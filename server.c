#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include "common.h"


int clients_waiting = 0;
struct client_t {
  struct sockaddr_in up_server;
  int sockfd;

  in_addr_t addr;
  in_port_t port;

  char name[25];
} waiting_client;

struct player_results {
  char name[25];
  int wins;
};
struct player_results results[30];
int next_result_slot = 0;

void register_user(char player_name[25]) {
  int i;
  for (i = 0; i < next_result_slot; i ++) {
    if (strcmp(player_name, results[i].name) == 0) {
      printf("Player %s already registered with %d wins\n", results[i].name, results[i].wins);
      return;
    }
  }

  printf("Registering %s\n", player_name);
  strcpy(results[next_result_slot].name, player_name);
  results[next_result_slot].wins = 0;

  next_result_slot++;
}

int open_server_socket() {
  int sockfd;
  struct sockaddr_in address;
  int len;
  int result;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  address.sin_family = AF_INET;
   
  address.sin_port = 9876;

  len = sizeof(address);

  result = bind(sockfd, (struct sockaddr *)&address, len);
  printf("Server binding: %d\n", result);

  if (result < 0) {
    exit(1);
  }

  return sockfd;
}

void close_socket(int sockfd) {
    close(sockfd);
}

void process_play_request(int client_sockfd, struct server_com * curr_message) {
  struct server_com response_waiting_client, response_active_client;

  register_user(curr_message->player_name);

  if (clients_waiting == 0) {
      // Case no client is waiting for a game
      waiting_client.sockfd = client_sockfd;
      waiting_client.addr = curr_message->addr;
      waiting_client.port = curr_message->port;
      strcpy(waiting_client.name, curr_message->player_name);
      clients_waiting += 1;

      printf("Server queued client %s\n", curr_message->player_name);

    } else if (clients_waiting == 1) {
      // Case there's a client already waiting

      // setup response for the waiting client
      response_waiting_client.mode = 1;
      response_waiting_client.addr = curr_message->addr;
      response_waiting_client.port = curr_message->port;
      response_waiting_client.play_piece = 'X';

      // Setup response for the current client
      response_active_client.mode = 1;
      response_active_client.addr = waiting_client.addr;
      response_active_client.port = waiting_client.port;
      response_active_client.play_piece = 'O';


      write(waiting_client.sockfd, &response_waiting_client, sizeof(response_waiting_client));
      write(client_sockfd, &response_active_client, sizeof(response_active_client));

      // Clean up
      close_socket(client_sockfd);
      close_socket(waiting_client.sockfd);
      clients_waiting = 0;

      printf("Server matched two clients: %s and %s\n", curr_message->player_name, waiting_client.name);
    }
}

int increment_player_wins(char player_name[25]) {
  int i;
  for (i = 0; i < next_result_slot; i++) {
    if (strcmp(player_name, results[i].name) == 0) {
      results[i].wins++;
      return results[i].wins;
    }
  }

  return -1;
}

void register_win(struct server_com * curr_message) {
  int current_wins = increment_player_wins(curr_message->player_name);

  if (current_wins == -1) {
    printf("Player %s not found!", curr_message->player_name);
  } else {
    printf("Player: %s\nWins:%d\n", curr_message->player_name, current_wins);
  }
}


int main() {
  int server_sockfd, client_sockfd;
  int client_len;
  struct sockaddr_in client_address;
  int op1, op2;
  char op;
  int result;
  int bind_result;
  struct server_com curr_message;

  server_sockfd = open_server_socket();
  listen(server_sockfd, 5);

  client_len = sizeof(client_address);

  while (1) {
    printf("server waiting...\n");

    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address,
                           (unsigned int *)&client_len);

    read(client_sockfd, &curr_message, sizeof(struct server_com));
    printf("Received message in mode: %d\n", curr_message.mode);
    switch (curr_message.mode) {
      case 0:
        process_play_request(client_sockfd, &curr_message);
        break;
      case 2:
        register_win(&curr_message);
        break;
      default:
        printf("Invalid mode: %d", curr_message.mode);
    }
  }
}