#ifndef GAME_GAME_STARTUP_H
#define GAME_GAME_STARTUP_H

#define PORT 8080

int startup_error();

void ncursesInitializing();

void *socketInitializing(int *game_socket, struct sockaddr_in *serv_addr);

#endif //GAME_GAME_STARTUP_H
