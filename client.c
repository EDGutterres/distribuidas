#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

void run_wait();
void run_play();

struct game_info_t {
  char table[3][3];
  int plays_count;
  char winner;
} game_info;

struct server_info_t {
  in_addr_t addr;
  in_port_t port;

  int sockfd;
} matching_server_client, oponnent_com, receive_server;

struct player_t {
  char piece;
  char name[25];
} player;


struct play_info {
  char table[3][3];
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
  strcpy(start_request.player_name, player.name);

  matching_server_client.sockfd = open_client_socket(matching_server_client.addr, matching_server_client.port);
  write_to_matching_server(&start_request);
  read_from_matching_server(&start_response);
  close(matching_server_client.sockfd);

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
      game_info.table[i][j] = ' ';
    }
  }
}

int is_valid(char letter) {
  if(letter == 'X' || letter == 'O')
    return 1;
  else
    return 0;
}

int coord_is_valid(int x, int y) {
  if(x >= 0 && x < 3) {
    if(y >= 0 && y < 3)
      return 1;
  }
  return 0;
}

int empty_pos(int x, int y) {
  if(game_info.table[x][y] != 'X' && game_info.table[x][y] != 'O')
    return 1;
  return 0;
}

int win_by_line() {
  int i, j, equal = 1;
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 2; j++) {
      if(is_valid(game_info.table[i][j]) && game_info.table[i][j] == game_info.table[i][j+1])
        equal++;
    }
    if(equal == 3)
      return 1;
    equal = 1;
  }
  return 0;
}

int win_by_col() {
  int i, j, equal = 1;
  for(i = 0; i < 3; i++) {
    for(j = 0; j < 2; j++) {
      if(is_valid(game_info.table[j][i]) && game_info.table[j][i] == game_info.table[j+1][i])
        equal++;
    }
    if(equal == 3)
      return 1;
    equal = 1;
  }
  return 0;
}

int win_primary_diag() {
  int i, equal = 1;
  for(i = 0; i < 2; i++) {
    if(is_valid(game_info.table[i][i]) && game_info.table[i][i] == game_info.table[i+1][i+1])
      equal++;
  }
  if(equal == 3)
    return 1;
  else
    return 0;
}

int win_secondary_diag() {
  int i, equal = 1;
  for(i = 0; i < 2; i++) {
    if(is_valid(game_info.table[i][3-i-1]) && game_info.table[i][3-i-1] == game_info.table[i+1][3-i-2])
      equal++;
  }
  if(equal == 3)
    return 1;
  else
    return 0;
}

void show_table() {
  int lin, col;

  printf("\n\t    0  1  2\n");
  for(lin = 0; lin < 3; lin++) {
    printf("\t%d ", lin);
    for(col = 0; col < 3; col++) {
      if(col < 2)
        printf(" %c |", game_info.table[lin][col]);
      else
        printf(" %c ", game_info.table[lin][col]);
    }
    if(lin < 2)
      printf("\n\t   ---------\n");
  }
  printf("\n");
}


void receive_table(struct play_info op_play) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      game_info.table[i][j] = op_play.table[i][j];
    }
  }
}

void fill_table_payload(struct play_info * play) {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      play->table[i][j] = game_info.table[i][j];
    }
  }
}

void get_play_from_user() {
  int valid, x, y;

  do {
    printf("\nEnter the coordinate you want to play (format x,y): ");
    scanf("%d,%d", &x, &y);
    valid = coord_is_valid(x, y);
    if(valid == 1)
      valid += empty_pos(x, y);

    if (valid != 2) {
      printf("Invalid position.\n");
      int c;
      while ((c = getchar()) != '\n' && c != EOF) { }
    }
  } while(valid != 2);

  game_info.table[x][y] = player.piece;
}

int check_result() {
  int win = 0;

  win += win_by_line();
  win += win_by_col();
  win += win_primary_diag();
  win += win_secondary_diag();

  if (win == 0 && game_info.plays_count == 9) {
    return -1;
  } 

  return win;
}

void finish_game() {
  close(receive_server.sockfd);

  close(oponnent_com.sockfd);

  if (game_info.winner == player.piece) {
    printf("Notifying server that you have won...\n");
    struct server_com payload;
    payload.mode = 2;
    payload.play_piece = player.piece;
    strcpy(payload.player_name, player.name);
    matching_server_client.sockfd = open_client_socket(matching_server_client.addr, matching_server_client.port);
    write_to_matching_server(&payload);
    close(matching_server_client.sockfd);
  } else if (game_info.winner == 'E'){
      printf("Notifying server that the game drawn...\n");
      struct server_com payload;
      payload.mode = 3;
      payload.play_piece = player.piece;
      strcpy(payload.player_name, player.name);
      matching_server_client.sockfd = open_client_socket(matching_server_client.addr, matching_server_client.port);
      write_to_matching_server(&payload);
      close(matching_server_client.sockfd);
  }

  close(matching_server_client.sockfd);

  exit(0);
}

void run_wait() {
  printf("--------------------\n");
  printf("Waiting oponnent to make a play...\n");

  struct play_info op_play;
  receive_from_oponnent(&op_play);

  receive_table(op_play);
  game_info.plays_count += 1;

  int result =  check_result();

  if (result == -1) {
    printf("Game Draw!\n");
    game_info.winner = 'E';
  } else if (result > 0) {
    show_table();
    printf("You Lost.\n");
    game_info.winner = (player.piece == 'X' ? 'O' : 'X');
  }

  if (result != 0) { 
    finish_game();
  }

  run_play();
}

void run_play() {
  printf("--------------------\n");
  printf("Your turn!\n");
  show_table();

  get_play_from_user();
  printf("Current board:\n");
  show_table();

  struct play_info play;
  fill_table_payload(&play);
  send_to_oponnent(&play);
  game_info.plays_count += 1;

  int result = check_result();
  if (result > 0) {
    printf("You won!\n");
    game_info.winner = player.piece;
  } else if (result == -1) {
    printf("Game Draw.\n");
    game_info.winner = 'E';
  }

  if (result != 0) {
    finish_game();
  }

  run_wait();
}

void run_game() {
  if (player.piece == 'X') {
    printf("You start!\n");
    run_play();
  } else {
    printf("Your oponnent starts.\n");
    run_wait();
  }
}


int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Missing port input value\n");
    printf("Usade: ./client.o <port:int> <name:char[25]> <server_ip:char[]");
    exit(1);
  }

  strcpy(player.name, argv[2]);

  setup_matching_server();
  setup_receiving_server(inet_addr(argv[3]), atoi(argv[1]));

  get_oponnent();

  printf("Player piece is %c\n", player.piece);
  printf("Oponnent is at %d:%d\n", oponnent_com.addr, oponnent_com.port);

  init_game();
  run_game();

  return 0;
}