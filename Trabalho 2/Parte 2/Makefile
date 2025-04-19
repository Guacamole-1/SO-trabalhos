%: %.c
	gcc $^  -Wall -o $@.out

%.o: %.c %.h
	gcc -Wall -c $< -o $@

ServerTCP: ServerTCP.c sockets.o globals.o 
	gcc -Wall -pthread $^ -o $@.out

all: ex2.c

clean:
	rm *.out

run:
	./childs.out