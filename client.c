#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>


char game[3][3];

struct server_com {
  int mode; // 0 for start a game, 1 for server response, 2 for report result
  in_addr_t addr; // When client->server, client sends its own server add, when
                  // server->client, server sents oponnent addr
  in_port_t port;
  char play_piece; // X or O
};

struct matching_server_client_t {
  in_addr_t addr;
  in_port_t port;

  int sockfd;
} matching_server_client;

struct oponnent_com_t {
  in_addr_t addr;
  in_port_t port;

  int sockfd;
} oponnent_com;

struct receive_server_t {
  in_addr_t addr;
  in_port_t port;

  int sockfd;
} receive_server;

struct player_t {
  char piece;
} player;


struct play_info {
  int test;
};

int open_client_socket(in_addr_t serv_addr, in_port_t port) {
  int sockfd, len, result;
  struct sockaddr_in address;

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

int open_server_socket(in_addr_t addr, in_port_t port) {
  int sockfd, len, result;
  struct sockaddr_in address;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = addr;
  address.sin_port = port;

  len = sizeof(address);

  result = bind(sockfd, (struct sockaddr *)&address, len);

  printf("Server binding to addr %d and port %d: %d\n", addr, port, result);

  return sockfd;
}

void setup_matching_server() {
  matching_server_client.addr = inet_addr("127.0.0.1");
  matching_server_client.port = 9876;

  matching_server_client.sockfd = open_client_socket(matching_server_client.addr, matching_server_client.port);
}

void setup_receiving_server(in_addr_t addr, in_port_t port) {
  receive_server.addr = addr;
  receive_server.port = port;

  receive_server.sockfd = open_server_socket(addr, port);

  listen(receive_server.sockfd, 5);
}

void setup_client_to_oponnent() {
  oponnent_com.sockfd = open_client_socket(oponnent_com.addr, oponnent_com.port);
}


void write_to_matching_server(struct server_com *payload) {
  int len = sizeof(*payload);
  write(matching_server_client.sockfd, payload, len);
}

void read_from_matching_server(struct server_com *payload) {
  int len = sizeof(*payload);

  read(matching_server_client.sockfd, payload, len);
}


void get_oponnent() {
  struct server_com start_request, start_response;

  start_request.mode = 0;
  start_request.addr = receive_server.addr;
  start_request.port = receive_server.port;

  write_to_matching_server(&start_request);
  read_from_matching_server(&start_response);

  player.piece = start_response.play_piece;

  oponnent_com.addr = start_response.addr;
  oponnent_com.port = start_response.port;
}

void send_to_oponnent(struct play_info * payload) {
  setup_client_to_oponnent();

  write(oponnent_com.sockfd, payload, sizeof(*payload));

  close(oponnent_com.sockfd);
}

void receive_from_oponnent(struct play_info * payload) {
  int client_sockfd;
  struct sockaddr_in client_address;
  int client_len = sizeof(client_address);

  client_sockfd = accept(receive_server.sockfd, (struct sockaddr *)&client_address,
                          (unsigned int *)&client_len);

  read(client_sockfd, payload, sizeof(*payload));
}

void init_game() {
  int i, j;

  for(i=0; i<3; i++) {
    for(j=0; j<3; j++) {
      game[i][j] = ' ';
    }
  }
}

int isValid(char letter) {
  if(letter == 'X' || letter == 'O')
    return 1;
  else
    return 0;
}

int coordIsValid(int x, int y) {
  if(x >= 0 && x < 3) {
    if(y >= 0 && y < 3)
      return 1;
  }
  return 0;
}

int emptyPos(int x, int y) {
  if(game[x][y] != 'X' && game[x][y] != 'O')
    return 1;
  return 0;
}

int winByLine() {
  int i, j, equal = 1;
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 2; j++) {
      if(isValid(game[i][j]) && game[i][j] == game[i][j+1])
        equal++;
    }
    if(equal == 3)
      return 1;
    equal = 1;
  }
  return 0;
}

int winByCol() {
  int i, j, equal = 1;
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 2; j++) {
      if(isValid(game[j][i]) && game[j][i] == game[j+1][i])
        equal++;
    }
    if(equal == 3)
      return 1;
    equal = 1;
  }
  return 0;
}

int winPrimmaryDiag() {
  int i, equal = 1;
  for(i = 0; i < 2; i++) {
    if(isValid(game[i][i]) && game[i][i] == game[i+1][i+1])
      equal++;
  }
  if(equal == 3)
    return 1;
  else
    return 0;
}

int winSecundaryDiag() {
  int i, equal = 1;
  for(i = 0; i < 2; i++) {
    if(isValid(game[i][3-i-1]) && game[i][3-i-1] == game[i+1][3-i-2])
      equal++;
  }
  if(equal == 3)
    return 1;
  else
    return 0;
}

void show() {
  int lin, col;

  printf("\n\t    0  1  2\n");
  for(lin = 0; lin < 3; lin++) {
    printf("\t%d ", lin);
    for(col = 0; col < 3; col++) {
      if(col < 2)
        printf(" %c |", game[lin][col]);
      else
        printf(" %c ", game[lin][col]);
    }
    if(lin < 2)
      printf("\n\t   ---------\n");
  }
}

void play() {
  int x, y, valid, plays = 0, win = 0, order = 1;

  do {
    do {
      show();
      printf("\nDigite a coordenada que deseja jogar: ");
      scanf("%d%d", &x, &y);
      valid = coordIsValid(x, y);
      if(valid == 1)
        valid += emptyPos(x, y);
    } while(valid != 2);
    if(order == 1) {
      game[x][y] = 'X';
    } else {
      game[x][y] = 'O';
    }
    plays++;
    order++;
    if(order == 3)
      order = 1;
    win += winByLine();
    win += winByCol();
    win += winPrimmaryDiag();
    win +- winSecundaryDiag();
  } while (win == 0 && plays < 9);
  if(win != 0) {
    show();
    if(order - 1 == 1)
      printf("\nParabens. Voce venceu %s\n", "jogador1");
    else
      printf("\nParabens. Voce venceu %s\n", "jogador2");
  } else
      printf("\nEmpatou!!\n\n");
}


int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Mising port input value\n");
    exit(1);
  }
  setup_matching_server();
  setup_receiving_server(inet_addr("127.0.0.1"), atoi(argv[1]));

  get_oponnent();

  printf("Player piece is %c\n", player.piece);
  printf("Oponent is at %d:%d\n", oponnent_com.addr, oponnent_com.port);

  if (player.piece == 'X') {
    sleep(2);
    struct play_info payload = {42};
    send_to_oponnent(&payload);

  } else {
    struct play_info payload;

    receive_from_oponnent(&payload);

    printf("Received from oponnent: %d", payload.test);
  }
}
