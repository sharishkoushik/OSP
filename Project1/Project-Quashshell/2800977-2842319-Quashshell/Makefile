# Defined constants for command base name and arguments for simple test


STUDENT_ID=2800977-2842319

build:
	gcc ./Quash.c -L/usr/local/lib -I/usr/local/include -lreadline -o Quash


test:
	./Quash

clean:
	rm -f Quash
	rm -rf $(STUDENT_ID)-Quashshell

tar:
	make clean
	mkdir $(STUDENT_ID)-Quashshell
	cp Quash.c Makefile $(STUDENT_ID)-Quashshell
	tar cvzf $(STUDENT_ID)-Quashshell.tar.gz $(STUDENT_ID)-Quashshell
	rm -rf $(STUDENT_ID)-Quashshell

