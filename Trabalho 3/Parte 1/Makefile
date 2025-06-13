%: %.c
	gcc $^  -Wall -o $@.out

%.o: %.c %.h
	gcc -Wall -c $< -o $@


all: ex3a ex3b 

clean:
	rm *.out *.o *.log

run:
	./childs.out