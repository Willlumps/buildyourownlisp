# TODO FIX ME
CC = gcc
CFLAGS = -g -Wall

parsing:
	$(CC) $(CFLAGS) parsing.c mpc.c -ledit -lm -o parsing

#parsing.o: parsing.c parsing.h mpc.h
#	$(CC) $(CFLAGS) -c -g parsing.c mpc.c -ledit -lm
#
#mpc.o: mpc.c mpc.h
#	$(CC) $(CFLAGS) -c -ledit -lm mpc.c
#
clean:
	rm -rf *.o *.dSYM/ parsing
