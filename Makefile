CC=gcc

merge: main.c log.c log.h
	$(CC) -o audio main.c log.c popen2.c -g -lpthread
clean: 
	rm -f audio
