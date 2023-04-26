all:
	gcc -o main *.c -lncurses -lpanel -pthread -std=c99