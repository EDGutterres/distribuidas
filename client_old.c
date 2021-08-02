#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

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

struct game_t {
  int server_sockfd;
} game;

int open_server_socket(in_addr_t addr, in_port_t port) {
  int sockfd;
  struct sockaddr_in address;
  int len;
  int result;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = addr;
  address.sin_port = port;

  len = sizeof(address);

  result =
      bind(sockfd, (struct sockaddr *)&address, len);

  printf("Server binding to addr %d and port %d: %d\n", addr, port, result);

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
    printf("Client failed to connect to %d:%d.\n", serv_addr, port);
    exit(1);
  }

  return sockfd;
}

void request_game(int serv_port) {
  int sockfd;
  struct server_com start_request, start_response;
  // in_addr_t serv_addr = htonl(INADDR_ANY); 
  in_addr_t serv_addr = inet_addr("127.0.0.1"); 

  start_request.mode = 0;
  start_request.addr = serv_addr;
  start_request.port = serv_port;

  game.server_sockfd = open_server_socket(serv_addr, serv_port);

  // server addr and port
  sockfd = open_client_socket(inet_addr("127.0.0.1"), 9876);

  write(sockfd, &start_request, sizeof(start_request));
  read(sockfd, &start_response, sizeof(start_response));

  close(sockfd);

  player.piece = start_response.play_piece;
  
  oponnent.piece = (player.piece == 'X') ? 'O' : 'X';
  oponnent.addr = start_response.addr;
  oponnent.port = start_response.port;
}

void init_oponnent() {

  if (player.piece == 'X') {
    printf("Will connect to %d:%d\n", oponnent.addr, oponnent.port);
    oponnent.sockfd = open_client_socket(oponnent.addr, oponnent.port);
    write(oponnent.sockfd, "Hello", sizeof("Hello"));

  } else {
    int client_sockfd;
    struct sockaddr_in client_address;
    int client_len = sizeof(client_address); 

    client_sockfd = accept(game.server_sockfd, (struct sockaddr *)&client_address,
                           (unsigned int *)&client_len);
    
    if (client_sockfd == -1) {
      perror("Deu ruim\n");
      exit(1);
    } 

    printf("Accepted con from %d\n", client_sockfd);

    char *buffer[5];
    read(game.server_sockfd, &buffer, sizeof("Hello"));

    printf("Received message: %s", *buffer);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Mising port input value\n");
    exit(1);
  }

  request_game(atoi(argv[1]));

  printf("Player piece: %c; Oponnent piece: %c\n", player.piece, oponnent.piece);

  init_oponnent();
  
  exit(0);
}