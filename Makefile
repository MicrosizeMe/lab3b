CC=gcc
FLAGS=-pg -g --std=gnu99
LFLAGS=-pthread -lrt
DISTNAME=lab3b-604480880.tar.gz

default: lab3b 

lab3b: lab3b.c 
	$(CC) $(FLAGS) -o $@ $^ $(LFLAGS)
	
dist: $(DISTNAME)

$(DISTNAME) : Makefile lab3b.c README
	tar -cvzf $(DISTNAME) $^


run: 
	./lab3b