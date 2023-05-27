# nCurses_game

It is 2D console game for maximum of 2 players.

## Gameplay

Every player spawns in random place on map. The only goal is to get as many coins as possible and bring them back to the camp, where they can be stored. There are also 2 beasts which moves randomly. They will follow the player as soon as they will be in theirs field of view.

## Tech stack

- ncurses - UNIX console graphics library for C
- Threads
- UNIX Sockets

## Build

To build this project:
```bash
  make
```

## Run

To run server:

```bash
  ./server.o
```

To run game:

```bash
  ./game.o
```
## Keybindings

### Server

- `q`/`Q` - close the server
- `c` - spawn a coin
- `t` - spawn a treasure
- `T` - spawn a large treasure

### Client

- `q`/`Q` - quit the game
- &uarr; - move up
- &darr; - move down
- &larr; - move left
- &rarr; - move right
