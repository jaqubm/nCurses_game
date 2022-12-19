# nCurses_game

Game made as a project for the subject "Operating Systems 2"

## Build

To build this project:
```bash
  make
```

## Run

To run server:

```bash
  ./server
```

To run game:

```bash
  ./game
```
## Keybindings

### Server

- `q`/`Q` - close the server
- `b`/`B` - spawn a beast
- `c` - spawn a coin
- `t` - spawn a treasure
- `T` - spawn a large treasure

### Client

- `q`/`Q` - quit the game
- &uarr; - move up
- &darr; - move down
- &larr; - move left
- &rarr; - move right

## Tech stack

- ncurses - UNIX console graphics library for C
- Threads
- UNIX Sockets