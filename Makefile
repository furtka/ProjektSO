CC = gcc
CFLAGS = -Wall -Wextra -g

make all: bin/hive bin/worker

bin:
	mkdir -p bin

bin/lib_logger.o: logger.c logger.h bin
	$(CC) $(CFLAGS) -c -o bin/lib_logger.o logger.c

bin/lib_hive_ipc.o: hive_ipc.c hive_ipc.h bin/lib_logger.o bin 
	$(CC) $(CFLAGS) -c -o bin/lib_hive_ipc.o hive_ipc.c

bin/hive: hive.c bin/lib_hive_ipc.o bin/lib_logger.o  bin 
	$(CC) $(CFLAGS) -o bin/hive hive.c bin/lib_hive_ipc.o bin/lib_logger.o

bin/worker: worker.c bin/lib_hive_ipc.o bin/lib_logger.o  bin
	$(CC) $(CFLAGS) -o bin/worker worker.c bin/lib_hive_ipc.o bin/lib_logger.o

.PHONY: clean
clean:
		rm -rf bin