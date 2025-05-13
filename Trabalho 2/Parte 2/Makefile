%: %.c
	gcc $^  -Wall -o $@.out

%.o: %.c %.h
	gcc -Wall -c $< -o $@

Server: Server.c sockets.o globals.o packets.o
	gcc -Wall -pthread $^ -o $@.out

Client: Client.c sockets.o globals.o packets.o
	gcc -Wall -pthread $^ -o $@.out

all: ex2.c

clean:
	rm *.out *.o *.log

run:
	./childs.out