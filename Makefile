CFLAGS = -Wall -g
CC = gcc $(CFLAGS)
TARGETS= bl-server bl-client bl-showlog

all: $(TARGETS)

bl-server: server.o bl-server.o simpio.o util.o
	$(CC) -o bl-server server.o bl-server.o simpio.o util.o -lpthread
	@echo bl-server is ready

bl-client: bl-client.o simpio.o util.o
	$(CC) -o bl-client bl-client.o simpio.o util.o -lpthread
	@echo bl-client is ready

bl-showlog: bl-showlog.o
	$(CC) -o bl-showlog bl-showlog.o
	@echo bl-showlog is ready

bl-client.o: bl-client.c simpio.c util.c blather.h
	$(CC) -c $<

bl-server.o: bl-server.c simpio.c util.c blather.h
	$(CC) -c $<

bl-showlog.o: bl-showlog.c blather.h
	$(CC) -c $<

server.o: server.c blather.h
	$(CC) -c $<

util.o: util.c blather.h
	$(CC) -c $<

simpio.o: simpio.c blather.h
	$(CC) -c $<

shell-tests : shell_tests.sh shell_tests_data.sh clean-tests
	chmod u+rx shell_tests.sh shell_tests_data.sh normalize.awk filter-semopen-bug.awk
	./shell_tests.sh

clean-tests :
	rm -f *.log *.out *.diff *.expect *.valgrindout

clean:
	@echo Cleaning up object files
	rm -f *.o *.fifo *.log *.out *.diff *.expect *.valgrindout

realclean:
	@echo Removing objects and programs
	rm -f *.o *.fifo *.log bl-client bl-server bl-showlog *.log *.out *.diff *.expect *.valgrindout
