CC = gcc
CFLAGS = -Wall -Wextra -g

make all: bin/hive bin/bee

bin:
	mkdir -p bin

bin/lib_logger.o: src/logger.c src/logger.h bin
	$(CC) $(CFLAGS) -c -o bin/lib_logger.o src/logger.c

bin/lib_hive_ipc.o: src/hive_ipc.c src/hive_ipc.h bin/lib_logger.o bin 
	$(CC) $(CFLAGS) -c -o bin/lib_hive_ipc.o src/hive_ipc.c

bin/hive: src/hive.c bin/lib_hive_ipc.o bin/lib_logger.o  bin 
	$(CC) $(CFLAGS) -o bin/hive src/hive.c bin/lib_hive_ipc.o bin/lib_logger.o

bin/bee: src/bee.c bin/lib_hive_ipc.o bin/lib_logger.o  bin
	$(CC) $(CFLAGS) -o bin/bee src/bee.c bin/lib_hive_ipc.o bin/lib_logger.o

.PHONY: clean
clean:
	rm -rf bin