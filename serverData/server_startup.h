#include "server_player_info.h"

#ifndef SERVER_SERVER_STARTUP_H
#define SERVER_SERVER_STARTUP_H

#define PORT 8080
#define MAX_PLAYERS 2
#define MAX_BEASTS 2
#define MAP_SIZE 32

int startup_error();

void ncursesInitializing();

void *socketInitializing(int *server, struct sockaddr_in *address);

void *mapInitializing(char *filename, char ***map);

void playerInitializing(struct player *p);

#endif //SERVER_SERVER_STARTUP_H
