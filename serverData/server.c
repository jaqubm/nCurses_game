#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server_player_info.h"
#include "server_startup.h"

pthread_mutex_t lock;

int serverPID; //PID servera

int server; //Opis socketu servera
struct sockaddr_in address; //Struktura danych socketu serveru

int roundNumber = 0;    //Zmienna przechowująca numer obecnej rundy

char **map; //Dwuwymiarowa tablica przechowująca informacje o mapie gry
void mapAdd(char a);    //Funkcja odpowiedzialna za dodanie nowego elementu w losowym miejscu mapy
void mapDraw(); //Funkcja odpowiedzialna za generowanie mapy na konsoli
void mapFree(); //Funkcja zwalniająca zaalokowaną pamięć na informacje o mapie gry

struct player p1;   //Struktura gracza 1
struct player p2;   //Struktura gracza 2
struct droppedTreasure d;   //Struktura upuszczonego skarbu
void playerSpawn(); //Funkcja do losowania pozycji startowych graczy
void *playerConnecting(); //Funkcja służąca do łączenia gracza z serverem
void *playerUpdate(struct player *p);   //Funkcja służąca do aktualizacji obecnej pozycji gracza
void *playerCollision();    //Funckja służąca do rozpatrywania zderzania się graczy
void playerMapUpdate(struct player *p); //Funkcja aktualizująca co rundę widoczną przez gracza mapę

struct beast b1;    //Struktura bestii 1
struct beast b2;    //Struktura bestii 2
void beastInitializing();   //Funkcja służąca do losowania startowej pozycji bestii oraz inicjalizacji mutexu bestii
void *beastUpdate(void *b_void); //Funckja odpowiedzialna za ruch bestii oraz pogoń za graczem

void statisticDraw();   //Funckja odpowiedzialna za generowanie statystyk na konsoli
void legendDraw();  //Funckja odpowiedzialna za wygenerowanie legendy gry

int pressedButton = 0;  //Zmienna mówiąca czy jakiś przycisk został wciśnięty
void *pressButton();    //Funkcja odpowiedzialna za oczekiwanie w tle na wciśnięcie przycisku

int main()
{
    serverPID = getpid();   //Uzyskanie numeru PID servera

    ncursesInitializing();  //Inicjalizacja biblioteki ncurses.h

    socketInitializing(&server, &address); //Inicjalizacja socketu servera

    int sI_result = startup_error();    //Obsługa błędów inicjalizacji socketu servera

    if(sI_result == 1)
    {
        endwin();

        printf("Socket creation failed\n");

        return -1;
    }
    else if(sI_result == 2)
    {
        endwin();

        printf("Failed: setsockopt");

        return -1;
    }
    else if(sI_result == 3)
    {
        endwin();

        printf("Bind failed");

        return -1;
    }
    else if(sI_result == 4)
    {
        endwin();

        printf("Listen failed");

        return -1;
    }

    mapInitializing("map.txt", &map);   //Inicjalizacja mapy

    int mI_result = startup_error();    //Obsługa błędów inicjalizacji mapy servera

    if(mI_result == 1)
    {
        endwin();

        printf("Filename is NULL\n");

        return -1;
    }
    else if(mI_result == 2)
    {
        endwin();

        printf("Map file is missing");

        return -1;
    }
    else if(mI_result == 3)
    {
        endwin();

        printf("Map memory allocation fail");

        return -1;
    }

    playerSpawn();  //Losowanie pozycji startowych graczy

    playerInitializing(&p1);    //Inicjalizacja struktury gracza 1
    playerInitializing(&p2);    //Inicjalizacja struktury gracza 2

    beastInitializing(); //Inicjalizacja bestii

    //Inicjalizacja pierwszej pozycji upuszczonego skarbu poza mapą gry
    d.x = MAP_SIZE+1;
    d.y = MAP_SIZE+1;
    d.value = 0;

    legendDraw();   //Inicjalizacja legendy gry
    refresh();

    pthread_t press_button, connect;

    pthread_create(&press_button, NULL, pressButton, NULL); //Włączenie do działania wątku obsługującego wciskanie przycisku w tle
    pthread_create(&connect, NULL, playerConnecting, NULL); //Włączenie do działania wątku obsługującego akceptowanie nowych połączeń z graczami

    pthread_t player, beast1, beast2;

    pthread_mutex_init(&lock, NULL);

    while(1)
    {
        pthread_mutex_lock(&lock);

        if(pressedButton == 1) break;
        else if(pressedButton == 2) mapAdd('c');
        else if(pressedButton == 3) mapAdd('t');
        else if(pressedButton == 4) mapAdd('T');
        pressedButton = 0;

        pthread_mutex_unlock(&lock);

        playerUpdate(&p1);  //Aktualizacja położenia gracza 1
        playerUpdate(&p2);  //Aktualizacja położenia gracza 2

        pthread_create(&player, NULL, playerCollision, NULL);
        pthread_create(&beast1, NULL, beastUpdate, &b1);
        pthread_create(&beast2, NULL, beastUpdate, &b2);

        pthread_join(player, NULL); //Zderzanie się graczy
        pthread_join(beast1, NULL); //Ruch bestii 1
        pthread_join(beast2, NULL); //Ruch bestii 2

        if(p1.connected == 1)   //Wysyłanie informacji do gracza 1
        {
            playerMapUpdate(&p1);

            if(send(p1.socket, &p1, sizeof(struct player), 0) < 0)
            {
                close(p1.socket);

                playerInitializing(&p1);
            }
        }

        if(p2.connected == 1)   //Wysyłanie informacji do gracza 2
        {
            playerMapUpdate(&p2);

            if(send(p2.socket, &p2, sizeof(struct player), 0) < 0)
            {
                close(p2.socket);

                playerInitializing(&p2);
            }
        }

        roundNumber++;

        mapDraw();

        statisticDraw();

        refresh();

        usleep(500000);
    }

    pthread_mutex_destroy(&b1.beast_m);
    pthread_mutex_destroy(&b2.beast_m);
    pthread_mutex_destroy(&lock);

    if(p1.connected == 1) close(p1.socket); //Zamknięcie połączenia z socketem gracza 1
    if(p2.connected == 1) close(p2.socket); //Zamknięcie połączenia z socketem gracza 2

    shutdown(server, SHUT_RDWR); //Zamknięcie socketu servera

    endwin();   //Wyłączenie działania biblioteki ncurses.h

    mapFree();  //Zwalnianie pamięci mapy

    printf("Server has been closed successfully!\n");

    return 0;
}

void mapAdd(char a)
{
    srand(time(NULL));

    int x, y;

    while(1)    //Losowanie pozycji startowej gracza 1
    {
        x = rand()%MAP_SIZE;
        y = rand()%MAP_SIZE;

        if(*(*(map+y)+x) == ' ') break;
    }

    *(*(map+y)+x) = a;
}

void mapDraw()  //Funkcja rysująca mapę na konsoli
{
    for(int i=0; i<MAP_SIZE; i++)
    {
        for(int j=0; j<MAP_SIZE; j++)
        {
            if(*(*(map+i)+j) == ' ') mvprintw(i, j, " ");
            else if(*(*(map+i)+j) == 'W')   //Rysowanie ścian mapy
            {
                attron(A_REVERSE);
                mvprintw(i, j, " ");
                attroff(A_REVERSE);
            }
            else if(*(*(map+i)+j) == 'A')   //Rysowanie bazy
            {
                attron(COLOR_PAIR(2));
                mvprintw(i, j, "A");
                attroff(COLOR_PAIR(2));
            }
            else if(*(*(map+i)+j) == 'c')   //Rysowanie monety - c
            {
                attron(COLOR_PAIR(3));
                mvprintw(i, j, "c");
                attroff(COLOR_PAIR(3));
            }
            else if(*(*(map+i)+j) == 't')   //Rysowanie małego skarbu - t
            {
                attron(COLOR_PAIR(3));
                mvprintw(i, j, "t");
                attroff(COLOR_PAIR(3));
            }
            else if(*(*(map+i)+j) == 'T')   //Rysowanie dużego skarbu - T
            {
                attron(COLOR_PAIR(3));
                mvprintw(i, j, "T");
                attroff(COLOR_PAIR(3));
            }
            else if(*(*(map+i)+j) == '#')   //Rysowanie krzaków spowalniających
            {
                attron(COLOR_PAIR(4));
                mvprintw(i, j, "#");
                attroff(COLOR_PAIR(4));
            }

            if(i == d.y && j == d.x)
            {
                attron(COLOR_PAIR(3));
                mvprintw(i, j, "D");
                attroff(COLOR_PAIR(3));
            }

            if(p1.connected == 1 && i == p1.y && j == p1.x) //Rysowanie gracza 1 jeśli ten jest podłączony
            {
                attron(COLOR_PAIR(1));
                mvprintw(i, j, "1");
                attroff(COLOR_PAIR(1));
            }

            if(p2.connected == 1 && i == p2.y && j == p2.x) //Rysowanie gracza 2 jeśli ten jest podłączony
            {
                attron(COLOR_PAIR(1));
                mvprintw(i, j, "2");
                attroff(COLOR_PAIR(1));
            }

            if(i == b1.y && j == b1.x)
            {
                attron(COLOR_PAIR(5));
                mvprintw(i, j, "*");
                attroff(COLOR_PAIR(5));
            }

            if(i == b2.y && j == b2.x)
            {
                attron(COLOR_PAIR(5));
                mvprintw(i, j, "*");
                attroff(COLOR_PAIR(5));
            }
        }
    }
}

void mapFree()  //Funkcja zwalniająca pamięc mapy gry
{
    for(int i=0; i<MAP_SIZE; i++) free(*(map+i));
    free(map);
}

void playerSpawn()  //Funkcja służąca do losowania miejsca spawnu graczy
{
    srand(time(NULL));

    while(1)    //Losowanie pozycji startowej gracza 1
    {
        p1.xSpawn = rand()%MAP_SIZE;
        p1.ySpawn = rand()%MAP_SIZE;

        if(*(*(map+p1.ySpawn)+p1.xSpawn) != 'W') break;
    }

    while(1)    //Losowanie pozycji startowej gracza 2
    {
        p2.xSpawn = rand()%MAP_SIZE;
        p2.ySpawn = rand()%MAP_SIZE;

        if(p2.xSpawn != p1.xSpawn && p2.ySpawn != p1.ySpawn && *(*(map+p2.ySpawn)+p2.xSpawn) != 'W') break;
    }

    for(int i=0, l=p1.ySpawn-2; i<5; i++, l++)    //Pierwsza inicjalizacja widoku mapy gracza 1
    {
        for(int j=0, k=p1.xSpawn-2; j<5; j++, k++)
        {
            if(l >= 0 && l <= MAP_SIZE && k >= 0 && k <= MAP_SIZE) p1.mapView[i][j] = *(*(map+l)+k);
            else p1.mapView[i][j] = 'X';
        }
    }

    for(int i=0, l=p2.ySpawn-2; i<5; i++, l++)    //Pierwsza inicjalizacja widoku mapy gracza 2
    {
        for(int j=0, k=p2.xSpawn-2; j<5; j++, k++)
        {
            if(l >= 0 && l <= MAP_SIZE && k >= 0 && k <= MAP_SIZE) p2.mapView[i][j] = *(*(map+l)+k);
            else p2.mapView[i][j] = 'X';
        }
    }
}

void *playerConnecting()    //Funkcja służąca do akceptowania połączeń graczy z serwerem
{
    int socket;

    int addrLen = sizeof(address);

    while(1)
    {
        socket = accept(server, (struct sockaddr*)&address, (socklen_t*)&addrLen);  //Akceptacja nowych połączeń graczy z serwerem

        if(socket < 0) break;   //Przerwanie działania pętli w przypadku błędu z połączeniem

        if(p1.connected == 0)   //Przypisanie nowego połączenia gracza 1, jeśli takie jeszcze nie istnieje
        {
            p1.socket = socket;
            p1.connected = 1;
            p1.playerNumber = 1;

            if(send(p1.socket, &serverPID, sizeof(int), 0) < 0)
            {
                close(p1.socket);

                playerInitializing(&p1);
            }

            if(recv(p1.socket, &(p1.PID), sizeof(int), 0) < 0)
            {
                close(p1.socket);

                playerInitializing(&p1);
            }
        }
        else if(p2.connected == 0)  //Przypisanie nowego połączenia gracza 2, jeśli takie jeszcze nie istnieje
        {
            p2.socket = socket;
            p2.connected = 1;
            p2.playerNumber = 2;

            if(send(p2.socket, &serverPID, sizeof(int), 0) < 0)
            {
                close(p2.socket);

                playerInitializing(&p2);
            }

            if(recv(p2.socket, &(p2.PID), sizeof(int), 0) < 0)
            {
                close(p2.socket);

                playerInitializing(&p2);
            }
        }
        else    //Zamknięcie nowo otwartego połączenia w przypadku jeśli maksymalna dopuszczalna liczba graczy została już podłączona
        {
            int connect_lost = 0;

            send(socket, &connect_lost, sizeof(int), 0);

            close(socket);
        }
    }

    return NULL;
}

void *playerUpdate(struct player *p)    //Funkcja wykonująca ruch gracza
{
    if(p == NULL) return (void *) ERR;

    if(p->connected == 0) return NULL;

    if(recv(p->socket, &p->buttonPressed, sizeof(int), 0) < 0)  //Odbieranie wciśniętego przycisku przez gracza 1
    {
        close(p->socket);

        playerInitializing(p);

        return NULL;
    }

    if(p->buttonPressed == 1)   //Zamknięcie połączenia z graczem 1 w przypadku wyłączenia przez niego gry
    {
        close(p->socket);

        playerInitializing(p);

        return NULL;
    }

    p->roundNumber = roundNumber;   //Aktualizacja numery rundy u gracza

    if(p->roundSkip == 1)   //Pominięcie rundy gracza jeśli ten został spowolniony przez krzak
    {
        p->roundSkip = 2;

        return NULL;
    }

    if(p->buttonPressed == 2)  //Ruch gracza w górę
    {
        if(*(*(map+p->y-1)+p->x) == 'W') return NULL;

        p->y--;
    }
    else if(p->buttonPressed == 3)  //Ruch gracza w dół
    {
        if(*(*(map+p->y+1)+p->x) == 'W') return NULL;

        p->y++;
    }
    else if(p->buttonPressed == 4)  //Ruch gracza w lewo
    {
        if(*(*(map+p->y)+p->x-1) == 'W') return NULL;

        p->x--;
    }
    else if(p->buttonPressed == 5)  //Ruch gracza w prawo
    {
        if(*(*(map+p->y)+p->x+1) == 'W') return NULL;

        p->x++;
    }

    if(p->roundSkip == 2) p->roundSkip = 0; //Warunek zapobiegający zapętlaniu się pomijania rundy jeśli gracz zdecydował się stać w miejscu

    if(*(*(map+p->y)+p->x) == 'A')  //Oddanie wszystkich zebranych monet przez gracza do obozu
    {
        p->bankOfCoins += p->carriedCoins;

        p->carriedCoins = 0;
    }
    else if(*(*(map+p->y)+p->x) == 'c') //Zebranie monety - c
    {
        *(*(map+p->y)+p->x) = ' ';

        p->carriedCoins++;
    }
    else if(*(*(map+p->y)+p->x) == 't') //Zebranie skarbu - t
    {
        *(*(map+p->y)+p->x) = ' ';

        p->carriedCoins += 10;
    }
    else if(*(*(map+p->y)+p->x) == 'T') //Zebranie skarbu - T
    {
        *(*(map+p->y)+p->x) = ' ';

        p->carriedCoins += 50;
    }
    else if(p->x == d.x && p->y == d.y) //Zebranie skarbu - D
    {
        d.x = MAP_SIZE+1;
        d.y = MAP_SIZE+1;

        p->carriedCoins += d.value;

        d.value = 0;
    }
    else if(*(*(map+p->y)+p->x) == '#' && p->roundSkip == 0) p->roundSkip = 1;  //Pominięcie rundy przez gracza

    return NULL;
}

void *playerCollision() //Zderzanie się graczy oraz przypadkowe wejście gracza w bestię
{
    pthread_mutex_lock(&lock);

    if(p1.connected == 1 && p2.connected == 1)
    {
        if(p1.x == p2.x && p1.y == p2.y)
        {
            d.x = p1.x;
            d.y = p1.y;

            d.value += p1.carriedCoins;
            d.value += p2.carriedCoins;

            p1.carriedCoins = 0;
            p2.carriedCoins = 0;

            p1.x = p1.xSpawn;
            p1.y = p1.ySpawn;

            p2.x = p2.xSpawn;
            p2.y = p2.ySpawn;

            p1.deathsNumber++;
            p2.deathsNumber++;
        }
    }

    if(p1.connected == 1)
    {
        if(p1.x == b1.x && p1.y == b1.y)
        {
            d.x = p1.x;
            d.y = p1.y;

            d.value += p1.carriedCoins;

            p1.carriedCoins = 0;

            p1.x = p1.xSpawn;
            p1.y = p1.ySpawn;

            p1.deathsNumber++;
        }
        else if(p1.x == b2.x && p1.y == b2.y)
        {
            d.x = p1.x;
            d.y = p1.y;

            d.value += p1.carriedCoins;

            p1.carriedCoins = 0;

            p1.x = p1.xSpawn;
            p1.y = p1.ySpawn;

            p1.deathsNumber++;
        }
    }

    if(p2.connected == 1)
    {
        if(p2.x == b1.x && p2.y == b1.y)
        {
            d.x = p2.x;
            d.y = p2.y;

            d.value += p2.carriedCoins;

            p2.carriedCoins = 0;

            p2.x = p2.xSpawn;
            p2.y = p2.ySpawn;

            p2.deathsNumber++;
        }
        else if(p2.x == b2.x && p2.y == b2.y)
        {
            d.x = p2.x;
            d.y = p2.y;

            d.value += p2.carriedCoins;

            p2.carriedCoins = 0;

            p2.x = p2.xSpawn;
            p2.y = p2.ySpawn;

            p2.deathsNumber++;
        }
    }

    pthread_mutex_unlock(&lock);

    return NULL;
}

void playerMapUpdate(struct player *p)  //Aktualizacja obecnie widocznej przez gracza mapy
{
    for(int i=0, l=p->y-2; i<5; i++, l++)
    {
        for(int j=0, k=p->x-2; j<5; j++, k++)
        {
            if(l >= 0 && l < MAP_SIZE && k >= 0 && k < MAP_SIZE) p->mapView[i][j] = *(*(map+l)+k);
            else p->mapView[i][j] = 'X';

            if(l == b1.y && k == b1.x) p->mapView[i][j] = '*';
            else if(l == b2.y && k == b2.x) p->mapView[i][j] = '*';
            else if(p1.connected == 1 && l == p1.y && k == p1.x) p->mapView[i][j] = '1';
            else if(p2.connected == 1 && l == p2.y && k == p2.x) p->mapView[i][j] = '2';
            else if(l == d.y && k == d.x) p->mapView[i][j] = 'D';
        }
    }
}

void beastInitializing()    //Inicjalizacja pozycji startowej bestii oraz pierwsze wypełnienie jej mapy
{
    srand(time(NULL));

    while(1)    //Losowanie pozycji startowej bestii 1
    {
        b1.x = rand()%MAP_SIZE;
        b1.y = rand()%MAP_SIZE;

        if(*(*(map+b1.y)+b1.x) != 'W' && b1.x != p1.xSpawn && b1.x != p2.xSpawn && b1.y != p1.ySpawn && b1.y != p2.ySpawn) break;
    }

    for(int i=0, l=b1.y-2; i<5; i++, l++)    //Pierwsza inicjalizacja widoku mapy bestii 1
    {
        for(int j=0, k=b1.x-2; j<5; j++, k++)
        {
            if(l >= 0 && l <= MAP_SIZE && k >= 0 && k <= MAP_SIZE) b1.mapView[i][j] = *(*(map+l)+k);
            else b1.mapView[i][j] = 'X';
        }
    }

    while(1)    //Losowanie pozycji startowej bestii 2
    {
        b2.x = rand()%MAP_SIZE;
        b2.y = rand()%MAP_SIZE;

        if(*(*(map+b2.y)+b2.x) != 'W' && b2.x != b1.x && b2.y != b1.y && b2.x != p1.xSpawn && b2.x != p2.xSpawn && b2.y != p1.ySpawn && b2.y != p2.ySpawn) break;
    }

    for(int i=0, l=b2.y-2; i<5; i++, l++)    //Pierwsza inicjalizacja widoku mapy gracza 1
    {
        for(int j=0, k=b2.x-2; j<5; j++, k++)
        {
            if(l >= 0 && l <= MAP_SIZE && k >= 0 && k <= MAP_SIZE) b2.mapView[i][j] = *(*(map+l)+k);
            else b2.mapView[i][j] = 'X';
        }
    }

    pthread_mutex_init(&b1.beast_m, NULL);  //Inicjalizacja mutexu bestii 1
    pthread_mutex_init(&b2.beast_m, NULL);  //Inicjalizacja mutexu bestii 2

    b1.direction = 1;   //Pierwszy kierunek ruchu bestii 1
    b2.direction = 1;   //Pierwszy kierunek ruchu bestii 2

    b1.chase = 0;   //Wyłączenie pościgu na start bestii 1
    b2.chase = 0;   //Wyłączenie pościgu na start bestii 2
}

void *beastUpdate(void *b_void)  //Funkcja odpowiedzialna za poruszanie się bestii, pogoń za graczem oraz aktualizację obecnego widoku mapy przez bestię
{
    struct beast *b = (struct beast*)b_void;

    pthread_mutex_lock(&b->beast_m);

    b->chase = 0;

    if(p1.connected == 1)
    {
        if(b->mapView[1][2] == '1' || (b->mapView[1][2] != 'W' && b->mapView[0][2] == '1'))
        {
            b->chase = 1;
            b->y--;
        }
        else if(b->mapView[2][1] == '1' || (b->mapView[2][1] != 'W' && b->mapView[2][0] == '1'))
        {
            b->chase = 1;
            b->x--;
        }
        else if(b->mapView[3][2] == '1' || (b->mapView[3][2] != 'W' && b->mapView[4][2] == '1'))
        {
            b->chase = 1;
            b->y++;
        }
        else if(b->mapView[2][3] == '1' || (b->mapView[2][3] != 'W' && b->mapView[2][4] == '1'))
        {
            b->chase = 1;
            b->x++;
        }

        if(b->x == p1.x && b->y == p1.y)
        {
            d.x = p1.x;
            d.y = p1.y;

            d.value += p1.carriedCoins;

            p1.carriedCoins = 0;

            p1.x = p1.xSpawn;
            p1.y = p1.ySpawn;

            p1.deathsNumber++;
        }
    }

    if(p2.connected == 1 && b->chase == 0)
    {
        if(b->mapView[1][2] == '2' || (b->mapView[1][2] != 'W' && b->mapView[0][2] == '2'))
        {
            b->chase = 1;
            b->y--;
        }
        else if(b->mapView[2][1] == '2' || (b->mapView[2][1] != 'W' && b->mapView[2][0] == '2'))
        {
            b->chase = 1;
            b->x--;
        }
        else if(b->mapView[3][2] == '2' || (b->mapView[3][2] != 'W' && b->mapView[4][2] == '2'))
        {
            b->chase = 1;
            b->y++;
        }
        else if(b->mapView[2][3] == '2' || (b->mapView[2][3] != 'W' && b->mapView[2][4] == '2'))
        {
            b->chase = 1;
            b->x++;
        }

        if(b->x == p2.x && b->y == p2.y)
        {
            d.x = p2.x;
            d.y = p2.y;

            d.value += p2.carriedCoins;

            p2.carriedCoins = 0;

            p2.x = p2.xSpawn;
            p2.y = p2.ySpawn;

            p2.deathsNumber++;
        }
    }

    if(b->chase == 0)
    {
        if(b->direction == 1)
        {
            if(b->mapView[1][2] == 'W') b->direction = 2;
            else b->y--;
        }

        if(b->direction == 2)
        {
            if(b->mapView[2][1] == 'W') b->direction = 3;
            else b->x--;
        }

        if(b->direction == 3)
        {
            if(b->mapView[3][2] == 'W') b->direction = 4;
            else b->y++;
        }

        if(b->direction == 4)
        {
            if(b->mapView[2][3] == 'W') b->direction = 1;
            else b->x++;
        }
    }

    for(int i=0, l=b->y-2; i<5; i++, l++)
    {
        for(int j=0, k=b->x-2; j<5; j++, k++)
        {
            if(l >= 0 && l < MAP_SIZE && k >= 0 && k < MAP_SIZE) b->mapView[i][j] = *(*(map+l)+k);
            else b->mapView[i][j] = 'X';

            if(l == b1.y && k == b1.x) b->mapView[i][j] = '*';
            else if(l == b2.y && k == b2.x) b->mapView[i][j] = '*';
            else if(p1.connected == 1 && l == p1.y && k == p1.x) b->mapView[i][j] = '1';
            else if(p2.connected == 1 && l == p2.y && k == p2.x) b->mapView[i][j] = '2';
            else if(l == d.y && k == d.x) b->mapView[i][j] = 'D';
        }
    }

    pthread_mutex_unlock(&b->beast_m);

    return NULL;
}

void statisticDraw()
{
    mvprintw(1, MAP_SIZE+3, "Server's PID: %d", serverPID);
    mvprintw(2, MAP_SIZE+4, "Campsite X/Y: 17/16");
    mvprintw(3, MAP_SIZE+4, "Round number: %d", roundNumber);

    mvprintw(5, MAP_SIZE+3, "Parameter:   Player1  Player2");
    mvprintw(6, MAP_SIZE+4, "PID         ");
    mvprintw(7, MAP_SIZE+4, "Type        ");
    mvprintw(8, MAP_SIZE+4, "Curr X/Y    ");
    mvprintw(9, MAP_SIZE+4, "Deaths      ");

    mvprintw(11, MAP_SIZE+4, "Coins");
    mvprintw(12, MAP_SIZE+8, "carried");
    mvprintw(13, MAP_SIZE+8, "brought");

    if(p1.connected == 1)
    {
        mvprintw(6, MAP_SIZE+16, "%d", p1.PID);
        mvprintw(7, MAP_SIZE+16, "HUMAN");
        mvprintw(8, MAP_SIZE+16, "%02d/%02d", p1.x, p1.y);
        mvprintw(9, MAP_SIZE+16, "%d", p1.deathsNumber);

        mvprintw(12, MAP_SIZE+16, "%03d", p1.carriedCoins);
        mvprintw(13, MAP_SIZE+16, "%03d", p1.bankOfCoins);
    }
    else
    {
        mvprintw(6, MAP_SIZE+16, "-    ");
        mvprintw(7, MAP_SIZE+16, "-    ");
        mvprintw(8, MAP_SIZE+16, "--/--");
        mvprintw(9, MAP_SIZE+16, "-    ");

        mvprintw(12, MAP_SIZE+16, "     ");
        mvprintw(13, MAP_SIZE+16, "     ");
    }

    if(p2.connected == 1)
    {
        mvprintw(6, MAP_SIZE+25, "%d", p2.PID);
        mvprintw(7, MAP_SIZE+25, "HUMAN");
        mvprintw(8, MAP_SIZE+25, "%02d/%02d", p2.x, p2.y);
        mvprintw(9, MAP_SIZE+25, "%d", p2.deathsNumber);

        mvprintw(12, MAP_SIZE+25, "%03d", p2.carriedCoins);
        mvprintw(13, MAP_SIZE+25, "%03d", p2.bankOfCoins);
    }
    else
    {
        mvprintw(6, MAP_SIZE+25, "-    ");
        mvprintw(7, MAP_SIZE+25, "-    ");
        mvprintw(8, MAP_SIZE+25, "--/--");
        mvprintw(9, MAP_SIZE+25, "-    ");

        mvprintw(12, MAP_SIZE+25, "     ");
        mvprintw(13, MAP_SIZE+25, "     ");
    }
}

void legendDraw()
{
    mvprintw(16, MAP_SIZE+3, "Legend:");

    attron(COLOR_PAIR(1));
    mvprintw(17, MAP_SIZE+4, "12");
    attroff(COLOR_PAIR(1));
    mvprintw(17, MAP_SIZE+6, " - players");

    attron(A_REVERSE);
    mvprintw(18, MAP_SIZE+4, " ");
    attroff(A_REVERSE);
    mvprintw(18, MAP_SIZE+5, "    - wall");

    attron(COLOR_PAIR(4));
    mvprintw(19, MAP_SIZE+4, "#");
    attroff(COLOR_PAIR(4));
    mvprintw(19, MAP_SIZE+5, "    - bushes (slow down)");

    attron(COLOR_PAIR(5));
    mvprintw(20, MAP_SIZE+4, "*");
    attroff(COLOR_PAIR(5));
    mvprintw(20, MAP_SIZE+5, "    - wild beast");

    attron(COLOR_PAIR(3));
    mvprintw(21, MAP_SIZE+4, "c");
    attroff(COLOR_PAIR(3));
    mvprintw(21, MAP_SIZE+5, "    - one coin");

    attron(COLOR_PAIR(3));
    mvprintw(22, MAP_SIZE+4, "t");
    attroff(COLOR_PAIR(3));
    mvprintw(22, MAP_SIZE+5, "    - treasure (10 coins)");

    attron(COLOR_PAIR(3));
    mvprintw(23, MAP_SIZE+4, "T");
    attroff(COLOR_PAIR(3));
    mvprintw(23, MAP_SIZE+5, "    - large treasure (50 coins)");

    attron(COLOR_PAIR(3));
    mvprintw(24, MAP_SIZE+4, "D");
    attroff(COLOR_PAIR(3));
    mvprintw(24, MAP_SIZE+5, "    - dropped treasure");

    attron(COLOR_PAIR(2));
    mvprintw(25, MAP_SIZE+4, "A");
    attroff(COLOR_PAIR(2));
    mvprintw(25, MAP_SIZE+5, "    - campsite");
}

void *pressButton() //Funkcja odczytująca wciskane klawisze na bieżąco
{
    int button;

    pressedButton = 0;

    while(1)
    {
        button = getch();

        if(button == 'Q' || button == 'q')  //Koniec działania programu
        {
            pressedButton = 1;

            break;
        }
        else if(button == 'c') pressedButton = 2;   //Generacja monety w losowym miejscu mapu
        else if(button == 't') pressedButton = 3;   //Generacja małego skarbu w losowym miejscu mapy
        else if(button == 'T') pressedButton = 4;   //Generacja dużego skarbu w losowym miejscu mapy
        else if(button == 'B' || button == 'b') pressedButton = 5;  //Dodanie nowej bestii na server w losowym miejscu
    }

    return NULL;
}
