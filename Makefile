multirw:
	gcc -Wall -O3 -o multirw args.c multirw.c -lpthread

all: multirw

clean:
	rm multirw
