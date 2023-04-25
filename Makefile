# autor:    Vit Pavlik
# login:    xpavli0a
# osobni    cislo: 251301
# fakulta:  FIT VUT
# Created:  2022-04-24
# Modified: 2022-04-24
# vyvijeno s GNU Make 4.3 (Built for x86_64-pc-linux-gnu)

# Makefile pro druhy projekt predmetu IOS LS 2022/2023

# prekladac
CC=gcc

#CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -g -DNDEBUG
CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -g 

LDFLAGS=-pthread

# executable filename
FILENAME=proj2

# make
.PHONY: all
all: $(FILENAME)

# make run
.PHONY: run
.ONESHELL: 
run: all
	./$(FILENAME)
	echo return code $$?

# make clean
.PHONY: clean
clean:
	rm -f *.o *.elf $(FILENAME)

# make remake
.PHONY: remake
remake: clean all

# compile main
$(FILENAME).o: proj2.c makra.h
	$(CC) $(CFLAGS) -c -o $(FILENAME).o proj2.c

# link main
$(FILENAME): $(FILENAME).o
	$(CC) $(LDFLAGS) -o $(FILENAME) $(FILENAME).o


# toto neodevzdavat
# ------------------------------------------------------------------------------

# compile demo
demo.o: demo.c makra.h
	$(CC) $(CFLAGS) -c -o demo.o demo.c

# link demo
demo.elf: demo.o
	$(CC) $(LDFLAGS) -o demo.elf demo.o

# make demo
.PHONY: demo
.ONESHELL: 
demo: demo.elf
	./demo.elf
	echo $$?
