%: %.c
	gcc $^  -Wall -o $@.out

%.o: %.c %.h
	gcc -Wall -c $< -o $@


Client: Client.c sockets.o globals.o packets.o
	gcc -Wall -pthread $^ -o $@.out

all: Client

clean:
	rm *.out *.o *.log temp/*

run:
	./childs.out