path = Trab2_Parte_2/

%: %.c
	gcc $^ -pthread -Wall -o $@.out

%.o: %.c %.h
	gcc -Wall -pthread -c $< -o $@


tp_server: tp_server.c $(path)sockets.o $(path)globals.o $(path)packets.o thread_pool.c
	gcc -Wall -pthread $^ -o $@.out

all: tp_server


clean:
	rm *.out *.o *.log 

run:
	./childs.out