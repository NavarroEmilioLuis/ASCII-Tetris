# ASCII Tetris

This is a recreation of Tetris, it was written entirely in C to comply with the final project requirement of Harvard's CS50 Introduction to CS.
It falls under the category of a command-line program using C.

The game runs and renders on the terminal using ASCII characters (# to be precise).
It starts immediately after running it and features score per game and next piece display.

## Dependencies

The game will compile and run in a POSIX environment. Libraries used:

- stdio.h
- stdlib.h
- time.h
- ncurses.h

## Usage

A makefile is needed to link ncurses library. After compiling, the game can be run using:

```
./tetris
```

## How to play

The game uses the classic keyboard controllers: Arrow keys (left, down, right) to move the piece and the keys 'z' and 'x' to rotate the piece.
It features fixed auto-scroll (no level), soft drop, piece rotation and lines clear.
