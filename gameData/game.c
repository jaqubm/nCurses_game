#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "game_player_info.h"
#include "game_startup.h"

int serverPID;  //PID servera

struct player p;    //Struktura gracza

pthread_mutex_t lock;

int game_socket;    //Opis socketu gry
struct sockaddr_in serv_addr;   //Struktura danych socketu serveru

void mapDraw(); //Funkcja rysująca skrawek mapy widocznej przez gracza
void statisticDraw(); //Funkcja generująca statystyki gry
void legendDraw();  //Funkcja generująca legendę gry

int pressedButton = 0;  //Zmienna mówiąca czy jakiś przycisk został wciśnięty
void *pressButton();    //Funkcja odpowiedzialna za oczekiwanie w tle na wciśnięcie przycisku

int main()
{
    ncursesInitializing();  //Inicjalizacja biblioteki ncurses.h

    socketInitializing(&game_socket, &serv_addr);   //Inicjalizacja socketu gry

    int sI_result = startup_error();    //Obsługa błędów inicjalizacji socketu gry

    if(sI_result == 1)
    {
        endwin();
        printf("Socket creation error\n");
        return -1;
    }

    int connection; //Nawiązywanie połączenia z serverem
    if((connection = connect(game_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0)
    {
        endwin();

        printf("Connection with server failed/ Server is offline\n");

        return -1;
    }

    int testConnection; //Testowanie połączenia z serverem
    if(recv(game_socket, &testConnection, sizeof(int), 0) < 0)
    {
        close(connection);

        endwin();

        printf("Server has been disconnected\n");

        return 0;
    }

    if(testConnection == 0) //Sprawdzenie czy server nie jest już pełny
    {
        close(connection);

        endwin();

        printf("Server is full\n");

        return 1;
    }
    else serverPID = testConnection;

    int PID = getpid(); //Przesłanie numeru PID gracza do servera
    if(send(game_socket, &PID, sizeof(int), 0) < 0)
    {
        close(connection);

        endwin();

        printf("Server has been disconnected\n");

        return 0;
    }

    pthread_t press_button;

    pthread_create(&press_button, NULL, pressButton, NULL); //Włączenie do działania wątku obsługującego wciskanie przycisku w tle

    pthread_mutex_init(&lock, NULL);

    while(1)
    {
        pthread_mutex_lock(&lock);

        int decision = pressedButton;
        pressedButton = 0;

        pthread_mutex_unlock(&lock);

        if(send(game_socket, &decision, sizeof(int), 0) < 0)  //Wysyłanie informacji o wciśniętym przycisku przez gracza
        {
            close(connection);

            endwin();

            printf("Server has been disconnected\n");

            return -1;
        }

        if(decision == 1) break;

        if(recv(game_socket, &p, sizeof(struct player), 0) < 0)
        {
            close(connection);

            endwin();

            printf("Server has been disconnected\n");

            return -1;
        }

        mapDraw();

        statisticDraw();

        legendDraw();

        refresh();  //Odświeżanie konsoli
    }

    pthread_mutex_destroy(&lock);

    close(connection);  //Zamykanie połączonego socketu

    endwin();   //Wyłączenie działania biblioteki ncurses.h

    printf("Game closed successfully!\n");

    return 0;
}

void mapDraw()
{
    for(int i=0; i<p.mapSize; i++) for(int j=0; j<p.mapSize; j++) mvprintw(i, j, " ");

    for(int i=0, l=p.y-2; i<5; i++, l++)
    {
        for(int j=0, k=p.x-2; j<5; j++, k++)
        {
            if(l >= 0 && k>= 0 && l <= p.mapSize && k <= p.mapSize)
            {
                if(p.mapView[i][j] == 'W')
                {
                    attron(A_REVERSE);
                    mvprintw(l, k, " ");
                    attroff(A_REVERSE);
                }
                else if(p.mapView[i][j] == '*')
                {
                    attron(COLOR_PAIR(5));
                    mvprintw(l, k, "*");
                    attroff(COLOR_PAIR(5));
                }
                else if(p.mapView[i][j] == 'A')
                {
                    attron(COLOR_PAIR(2));
                    mvprintw(l, k, "A");
                    attroff(COLOR_PAIR(2));
                }
                else if(p.mapView[i][j] == 'c')
                {
                    attron(COLOR_PAIR(3));
                    mvprintw(l, k, "c");
                    attroff(COLOR_PAIR(3));
                }
                else if(p.mapView[i][j] == 't')
                {
                    attron(COLOR_PAIR(3));
                    mvprintw(l, k, "t");
                    attroff(COLOR_PAIR(3));
                }
                else if(p.mapView[i][j] == 'T')
                {
                    attron(COLOR_PAIR(3));
                    mvprintw(l, k, "T");
                    attroff(COLOR_PAIR(3));
                }
                else if(p.mapView[i][j] == 'D')
                {
                    attron(COLOR_PAIR(3));
                    mvprintw(l, k, "D");
                    attroff(COLOR_PAIR(3));
                }
                else if(p.mapView[i][j] == '#')
                {
                    attron(COLOR_PAIR(4));
                    mvprintw(l, k, "#");
                    attroff(COLOR_PAIR(4));
                }
                else if(p.mapView[i][j] == '1')
                {
                    attron(COLOR_PAIR(1));
                    mvprintw(l, k, "1");
                    attroff(COLOR_PAIR(1));
                }
                else if(p.mapView[i][j] == '2')
                {
                    attron(COLOR_PAIR(1));
                    mvprintw(l, k, "2");
                    attroff(COLOR_PAIR(1));
                }
                else mvprintw(l, k, " ");
            }
        }
    }
}

void statisticDraw()
{
    mvprintw(1, p.mapSize+3, "Server's PID: %d", serverPID);
    mvprintw(2, p.mapSize+4, "Campsite X/Y: unknown");
    mvprintw(3, p.mapSize+4, "Round number: %d", p.roundNumber);

    mvprintw(5, p.mapSize+3, "Player:");
    mvprintw(6, p.mapSize+4, "Number      ");
    mvprintw(7, p.mapSize+4, "Type        ");
    mvprintw(8, p.mapSize+4, "Curr X/Y    ");
    mvprintw(9, p.mapSize+4, "Deaths      ");

    mvprintw(11, p.mapSize+4, "Coins found ");
    mvprintw(12, p.mapSize+4, "Coins brought ");

    mvprintw(6, p.mapSize+16, "%d", p.PID);
    mvprintw(7, p.mapSize+16, "HUMAN");
    mvprintw(8, p.mapSize+16, "%02d/%02d", p.x, p.y);
    mvprintw(9, p.mapSize+16, "%d", p.deathsNumber);

    mvprintw(11, p.mapSize+16, "%03d", p.carriedCoins);
    mvprintw(12, p.mapSize+18, "%03d", p.bankOfCoins);
}

void legendDraw()
{
    mvprintw(16, p.mapSize+3, "Legend:");

    attron(COLOR_PAIR(1));
    mvprintw(17, p.mapSize+4, "12");
    attroff(COLOR_PAIR(1));
    mvprintw(17, p.mapSize+6, " - players");

    attron(A_REVERSE);
    mvprintw(18, p.mapSize+4, " ");
    attroff(A_REVERSE);
    mvprintw(18, p.mapSize+5, "    - wall");

    attron(COLOR_PAIR(4));
    mvprintw(19, p.mapSize+4, "#");
    attroff(COLOR_PAIR(4));
    mvprintw(19, p.mapSize+5, "    - bushes (slow down)");

    attron(COLOR_PAIR(5));
    mvprintw(20, p.mapSize+4, "*");
    attroff(COLOR_PAIR(5));
    mvprintw(20, p.mapSize+5, "    - wild beast");

    attron(COLOR_PAIR(3));
    mvprintw(21, p.mapSize+4, "c");
    attroff(COLOR_PAIR(3));
    mvprintw(21, p.mapSize+5, "    - one coin");

    attron(COLOR_PAIR(3));
    mvprintw(22, p.mapSize+4, "t");
    attroff(COLOR_PAIR(3));
    mvprintw(22, p.mapSize+5, "    - treasure (10 coins)");

    attron(COLOR_PAIR(3));
    mvprintw(23, p.mapSize+4, "T");
    attroff(COLOR_PAIR(3));
    mvprintw(23, p.mapSize+5, "    - large treasure (50 coins)");

    attron(COLOR_PAIR(3));
    mvprintw(24, p.mapSize+4, "D");
    attroff(COLOR_PAIR(3));
    mvprintw(24, p.mapSize+5, "    - dropped treasure");

    attron(COLOR_PAIR(2));
    mvprintw(25, p.mapSize+4, "A");
    attroff(COLOR_PAIR(2));
    mvprintw(25, p.mapSize+5, "    - campsite");
}

void *pressButton()
{
    int button;

    pressedButton = 0;

    while(1)
    {
        button = getch();

        if(button == 'Q' || button == 'q')
        {
            pressedButton = 1;

            break;
        }
        else if(button == KEY_UP) pressedButton = 2;
        else if(button == KEY_DOWN) pressedButton = 3;
        else if(button == KEY_LEFT) pressedButton = 4;
        else if(button == KEY_RIGHT) pressedButton = 5;
    }

    return NULL;
}
