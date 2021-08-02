#include <netinet/in.h>

struct server_com {
  int mode; // 0 for start a game, 1 for server response, 2 for report result 
  in_addr_t addr; // When client->server, client sends its own server add, when server->client, server sents oponnent addr 
  in_port_t port;
  char play_piece; // X or O

  char player_name[25];
};