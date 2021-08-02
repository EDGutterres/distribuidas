#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


char game[3][3];

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
  if (argc < 1) {
    printf("Missing port input value");
  }

  int com_server_port = atoi(argv[0]);

  request_game();

  init_oponnent();
  printf("Player piece: %c; Oponnent piece: %c", player.piece, oponnent.piece);

  int op;
  struct player_t player;
  struct oponnent_t oponnent;

  init_game();
  play();

  exit(0);
}