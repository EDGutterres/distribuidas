#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


struct server_com {
  int mode; // 0 for start a game, 1 for server response, 2 for report result 
  in_addr_t addr; // When client->server, client sends its own server add, when server->client, server sents oponnent addr 
  in_port_t port;
  char play_piece; // X or O
};

struct client_t {
  struct sockaddr_in up_server;
  int sockfd;

  in_addr_t addr;
  in_port_t port;
};

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

int close_socket(int sockfd) {
    close(sockfd);
}


int main() {
  int server_sockfd, client_sockfd;
  int client_len;
  struct sockaddr_in client_address;
  int op1, op2;
  char op;
  int result;
  int bind_result;
  struct server_com curr_message, response_waiting_client, response_active_client;

  struct client_t waiting_client;
  int clients_waiting = 0;

  server_sockfd = open_server_socket();
  listen(server_sockfd, 5);

  client_len = sizeof(client_address);

  while (1) {
    printf("server waiting...\n");

    client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_address,
                           (unsigned int *)&client_len);

    read(client_sockfd, &curr_message, sizeof(struct server_com));

    if (clients_waiting == 0) {
      // Case no client is waiting for a game
      waiting_client.sockfd = client_sockfd;
      waiting_client.addr = curr_message.addr;
      waiting_client.port = curr_message.port;
      clients_waiting += 1;

      printf("Server queued a client\n");

    } else if (clients_waiting == 1) {
      // Case there's a client already waiting

      // setup response for the waiting client
      response_waiting_client.mode = 1;
      response_waiting_client.addr = curr_message.addr;
      response_waiting_client.port = curr_message.port;
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

      printf("Server matched two clients\n");
    }
  }
}