#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct server_com {
  int mode; // 0 for start a game, 1 for server response, 2 for report result 
  in_addr_t addr; // When client->server, client sends its own server add, when server->client, server sents oponnent addr 
  in_port_t port;
  char play_piece; // X or O
};

struct oponnent_t {
  in_addr_t addr; // Oponnent address
  in_port_t port;
  int sockfd; // Open socket for communicating oponnent
  char piece; // X or O
} oponnent;

struct player_t {
  char piece;
} player;

int open_server_socket(in_port_t port) {
  int sockfd;
  struct sockaddr_in address;
  int len;
  int result;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = port;

  len = sizeof(address);

  result =
      bind(sockfd, (struct sockaddr *)&address, len);

  printf("Server binding: %d\n", result);

  return sockfd;
}

int open_client_socket(in_addr_t serv_addr, in_port_t port) {
  int sockfd;
  struct sockaddr_in address;
  int len;
  int result;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = serv_addr;
  address.sin_port = port;

  len = sizeof(address);

  result = connect(sockfd, (struct sockaddr *)&address, len);

  if (result == -1) {
    perror("Client failed to bind.");
    exit(1);
  }

  return sockfd;
}

void request_game() {
  int sockfd;
  struct server_com start_request, start_response;

  start_request.mode = 0;
  // start_request.addr = TODO;

  // server addr and port
  sockfd = open_client_socket(inet_addr("127.0.0.1"), 9876);

  write(sockfd, &start_request, sizeof(start_request));
  read(sockfd, &start_response, sizeof(start_response));

  close(sockfd);

  player.piece = start_response.play_piece;
  
  oponnent.piece = (player.piece == 'X') ? 'O' : 'X';
  oponnent.addr = start_response.addr;
}

void init_oponnent() {
  oponnent.sockfd = open_client_socket(); // CONTINUE FROM HERE
}

int main(int argc, char *argv[]) {
  if (argc < 1) {
    printf("Mising port input value");
  }

  int com_server_port = atoi(argv[0]);

  request_game();

  init_oponnent();
  printf("Player piece: %c; Oponnent piece: %c", player.piece, oponnent.piece);
  
  exit(0);
}