#ifndef GAME_PLAYER_INFO_H
#define GAME_PLAYER_INFO_H

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

#endif //GAME_PLAYER_INFO_H
