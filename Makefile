tetris:
	clang -ggdb3 -O0 -std=c11 -Wall -Werror -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow    tetris.c -lncurses -lcrypt -lcs50 -lm -o tetris