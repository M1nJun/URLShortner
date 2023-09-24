miniweb: miniweb.o queue.o
	gcc miniweb.o queue.o -pthread -o miniweb

miniweb.o: miniweb.c queue.h
	gcc miniweb.c -c -g -pthread

queue.o: queue.c queue.h
	gcc queue.c -c -g -pthread