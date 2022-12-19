#ifndef SERVER_PLAYER_INFO_H
#define SERVER_PLAYER_INFO_H

struct player{  //Struktura gracza
    int connected;  //Zmienna mówiąca czy gracz jest podłączony do servera

    int socket; //Opis socketu gracza

    int buttonPressed;  //Zmienna przechowująca obecną decyzję o ruchu gracza
    int roundSkip;  //Zmienna informująca o pominięciu rundy przez gracza

    int PID;    //PID gracza
    int roundNumber;    //Obecny numer rundy
    int mapSize;    //Rozmiar mapy gry

    int playerNumber;   //Numer gracza

    int deathsNumber;   //Ilość śmierci gracza

    int xSpawn; //Pozycja startowa gracza x
    int ySpawn; //Pozycja startowa gracza y

    int x;  //Obecna pozycja gracza x
    int y;  //Obecna pozycja gracza y

    char mapView[5][5]; //Tablica przechowująca obecnie widoczny skrawek mapy przez gracza

    int carriedCoins;   //Ilość obecnie niesionych przez gracza monet
    int bankOfCoins;    //Ilość monet przyniesionych przez gracza do obozu 'A'
};

struct droppedTreasure{ //Struktura upuszczonego skarbu
    int value;  //Wartość jaką ten skarb posiada

    int x;  //Obecna pozycja skarbu x
    int y;  //Obecna pozycja skarbu y
};

struct beast{   //Struktura bestii
    pthread_mutex_t beast_m;    //Mutex bestii

    int x;  //Obecna pozycja bestii x
    int y;  //Obecna pozycja bestii y

    int chase;  //Zmienna określająca czy nastąpiła pogoń za graczem

    int direction;  //Obecny kierunek w którym bestia się porusza

    char mapView[5][5]; //Tablica przechowująca obecnie widoczny skrawek mapy przez bestie
};

#endif //SERVER_PLAYER_INFO_H
