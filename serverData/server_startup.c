#include <stdlib.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server_startup.h"

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

void *socketInitializing(int *server, struct sockaddr_in *address)
{
    if((*server = socket(AF_INET, SOCK_STREAM, 0)) < 0) //Tworzenie socketu servera
    {
        err = 1;

        return NULL;
    }

    int opt = 1;    //Zapobieganie błędom typu "address already in use"

    if(setsockopt(*server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        err = 2;

        return NULL;
    }
    if(setsockopt(*server, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        err = 2;

        return NULL;
    }

    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    if(bind(*server, (struct sockaddr*)&(*address), sizeof(*address)) < 0)  //Bindowanie socketu servera do adresu oraz portu
    {
        err = 3;

        return NULL;
    }

    if(listen(*server, MAX_PLAYERS) < 0)  //Włączenie nasłuchiwania servera
    {
        err = 4;

        return NULL;
    }

    return NULL;
}

void *mapInitializing(char *filename, char ***map)
{
    if(filename == NULL)    //Błąd w przypadku przekazania pustej nazwy pliku mapy
    {
        err = 1;

        return NULL;
    }

    FILE *f;    //Otworzenie pliku mapy
    f = fopen(filename, "r");

    if(f == NULL)   //Błąd braku pliku lub niemożliwości jego otworzenia
    {
        err = 2;

        return NULL;
    }

    *map = calloc(MAP_SIZE, sizeof(char));  //Alokacja pamięci przechowującej informacje o mapie wraz z obsługą błędów

    if(*map == NULL)
    {
        fclose(f);

        err = 3;

        return NULL;
    }

    for(int i=0; i<MAP_SIZE; i++)
    {
        *(*map+i) = calloc(MAP_SIZE+1, sizeof(char*));

        if(*(*map+i) == NULL)
        {
            fclose(f);

            for(int j=0; j<i; j++) free(*(*map+j));

            free(*map);

            err = 3;

            return NULL;
        }
    }

    for(int i=0; i<MAP_SIZE; i++)   //Wczytanie mapy z pliku do zaalokowanej pamięci
    {
        for(int j=0; j<MAP_SIZE; j++) *(*(*map+i)+j) = fgetc(f);

        fgetc(f);
    }

    fclose(f);

    return NULL;
}

void playerInitializing(struct player *p)
{
    p->connected = 0;   //Stan połączenia - brak połączenia
    p->buttonPressed = 0;   //Wyzerowanie wciśniętego przycisku przez gracza
    p->roundSkip = 0;   //Wyzerowanie pominięcia rundy przez gracza
    p->PID = 0; //Wyzerowanie PID gracza
    p->roundNumber = 0; //Wyzerowanie rundy gracza
    p->mapSize = MAP_SIZE;    //Ustawienie odpowiedniego rozmiaru mapy
    p->playerNumber = 0;    //Wyzerowanie numeru gracza
    p->deathsNumber = 0;    //Wyzerowanie ilości śmierci

    p->x = p->xSpawn;   //Przypisanie obecnej pozycji gracza jego pozycji startowej
    p->y = p->ySpawn;   //Przypisanie obecnej pozycji gracza jego pozycji startowej

    p->carriedCoins = 0;    //Wyzerowanie ilości niesionych monet przez gracza
    p->bankOfCoins = 0;     //Wyzerowanie ilości monet przyniesionych przez gracza do obozu 'A'
}
