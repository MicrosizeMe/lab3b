CC=gcc
FLAGS=-pg -g --std=gnu99
LFLAGS=-pthread -lrt
DISTNAME=lab3b-604480880.tar.gz

default: lab3b 

lab3b: lab3b.o map.o
	$(CC) $(FLAGS) -o $@ $^ $(LFLAGS)
	
lab3b.o: lab3b.c map.h
	$(CC) $(FLAGS) -c lab3b.c map.o $(LFLAGS)

map.o: map.c map.h
	$(CC) $(FLAGS) -c map.c $(LFLAGS)

dist: $(DISTNAME)

$(DISTNAME) : Makefile lab3b.c README
	tar -cvzf $(DISTNAME) $^

run: 
	./lab3b