TEAM_NAME=Harish-Navajit

CC=gcc
CFLAGS=-g

all: buddy

clean:
	rm buddy

tar:
	make clean
	mkdir $(TEAM_NAME)-project-buddy
	cp buddy.c list.h Makefile $(TEAM_NAME)-project-buddy/
	tar cvzf $(TEAM_NAME)-project-buddy.tar.gz $(TEAM_NAME)-project-buddy
	rm -rf $(TEAM_NAME)-project-buddy
