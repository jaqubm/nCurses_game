#include <ncurses.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "game_startup.h"

int err = 0;

int startup_error()
{
    return err;
}

void ncursesInitializing()
{
    initscr();  //Włączenie do działania biblioteki ncurses.h
    start_color();  //Włączenie kolorów w konsoli
    curs_set(0);    //Usuwanie kursora
    noecho();   //Ustawienie nie pozastawiające echa po użyciu getch()
    cbreak();   //Zapobieganie kolejkowaniu się wczytywania znaków przez getch()
    keypad(stdscr, TRUE);   //Włączenie zczytywania strzałek do sterowania graczem

    //Inicjalizacja kolorów
    init_pair(1, COLOR_BLACK, COLOR_MAGENTA);   //Kolor gracza
    init_pair(2, COLOR_YELLOW, COLOR_GREEN);    //Kolor bazy
    init_pair(3, COLOR_BLACK, COLOR_YELLOW);    //Kolor monet i skarbów
    init_pair(4, COLOR_WHITE, COLOR_BLACK);     //Kolor krzaków
    init_pair(5, COLOR_WHITE, COLOR_RED);       //Kolor bestii
}

void *socketInitializing(int *game_socket, struct sockaddr_in *serv_addr)
{
    if((*game_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)    //Tworzenie socketu gry
    {
        err = 1;

        return NULL;
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(PORT);

    return NULL;
}
