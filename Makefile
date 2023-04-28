# autor:    Vit Pavlik
# login:    xpavli0a
# osobni    cislo: 251301
# fakulta:  FIT VUT
# Created:  2022-04-24
# Modified: 2022-04-28
# vyvijeno s GNU Make 4.3 (Built for x86_64-pc-linux-gnu)

# Makefile pro druhy projekt predmetu IOS LS 2022/2023

# prekladac
CC=gcc

CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -g -DNDEBUG
#CFLAGS=-std=gnu99 -Wall -Wextra -pedantic -g 

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
	./$(FILENAME) 48 24 1000 100 1500
	echo return code $$?

# make clean
.PHONY: clean
clean:
	rm -f *.o *.elf $(FILENAME)

# make remake
.PHONY: remake
remake: clean all

# compile main
$(FILENAME).o: proj2.c makra.h proj2.h fronta.h control_struct.h
	$(CC) $(CFLAGS) -c -o $(FILENAME).o proj2.c

# compile shm
shm.o: shm.c proj2.h makra.h
	$(CC) $(CFLAGS) -c -o shm.o shm.c

# compile fronta
fronta.o: fronta.c fronta.h proj2.h makra.h
	$(CC) $(CFLAGS) -c -o fronta.o fronta.c

# compile zakaznik
zakaznik.o: zakaznik.c makra.h proj2.h fronta.h control_struct.h
	$(CC) $(CFLAGS) -c -o zakaznik.o zakaznik.c

# compile urednik
urednik.o: urednik.c makra.h proj2.h fronta.h control_struct.h
	$(CC) $(CFLAGS) -c -o urednik.o urednik.c

# link main
$(FILENAME): $(FILENAME).o shm.o fronta.o zakaznik.o urednik.o
	$(CC) $(LDFLAGS) -o $(FILENAME) $(FILENAME).o shm.o fronta.o zakaznik.o \
	urednik.o


# toto neodevzdavat
# ------------------------------------------------------------------------------

.PHONY: run_no_break
run_no_break: run
	python3 skripty/skript_jedna.py

.PHONY: submit
submit:
	rm -f xpavli0a.zip
	zip xpavli0a.zip *.c *.h Makefile

.PHONY: copy_home
copy_home: submit
	rm -r -f /home/vita/ios_proj2/*
	cp xpavli0a.zip /home/vita/ios_proj2/xpavli0a.zip
	cp /home/vita/ios_testy/tester.sh /home/vita/ios_proj2/tester.sh
	cp /home/vita/ios_proj2/xpavli0a.zip \
/home/vita/ios_testy/IOS_tester_2023/xpavli0a.zip

# scp the archive to eva (scp prompts for password!)
.PHONY: scp_eva
scp_eva: submit
	scp xpavli0a.zip xpavli0a@eva.fit.vutbr.cz:~/ios/proj2/xpavli0a.zip

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

.PHONY: test
test: remake test1 test2 test3 test4

.PHONY: test1
test1:
	rm -rf ./OUTPUT
	./testy/tester.sh

.PHONY: test2
test2:
	./testy/deadlock.sh 30 10 500 50 750

.PHONY: test3
test3:
	cp ./testy/IOS_tester_2023/kontrola-vystupu.py ./kontrola-vystupu.py
	./testy/IOS_tester_2023/test.sh
	rm -f ./kontrola-vystupu.py

.PHONY: test4
test4: all
	./proj2 80 10 500 50 750
	cat proj2.out | ./kontrola-vystupu.sh

