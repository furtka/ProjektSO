CC = gcc
CFLAGS = -Wall -Wextra -g

make all: bin/hive bin/bee bin/logger_server

bin:
	mkdir -p bin

bin/logger_internal.o: bin src/logger/logger_internal.c src/logger/logger_internal.h
	$(CC) $(CFLAGS) -c -o bin/logger_internal.o src/logger/logger_internal.c

bin/lib_logger.o: bin src/logger/logger.c src/logger/logger.h bin/logger_internal.o
	$(CC) $(CFLAGS) -c -o bin/lib_logger.o src/logger/logger.c bin/logger_internal.o

bin/lib_hive_ipc.o: bin src/hive_ipc.c src/hive_ipc.h bin/lib_logger.o bin/logger_server
	$(CC) $(CFLAGS) -c -o bin/lib_hive_ipc.o src/hive_ipc.c

bin/hive: bin src/hive.c bin/lib_hive_ipc.o bin/lib_logger.o bin/logger_server bin/logger_internal.o bin/queen
	$(CC) $(CFLAGS) -o bin/hive src/hive.c bin/lib_hive_ipc.o bin/lib_logger.o bin/logger_internal.o

bin/bee: bin src/bee.c bin/lib_hive_ipc.o bin/lib_logger.o bin/logger_internal.o
	$(CC) $(CFLAGS) -o bin/bee src/bee.c bin/lib_hive_ipc.o bin/lib_logger.o bin/logger_internal.o

bin/logger_server: bin src/logger/logger_server.c src/logger/logger_internal.c src/logger/logger_internal.h
	$(CC) $(CFLAGS) -o bin/logger_server src/logger/logger_internal.c src/logger/logger_server.c

bin/queen: bin src/queen.c bin/lib_hive_ipc.o bin/lib_logger.o bin/logger_internal.o
	$(CC) $(CFLAGS) -o bin/queen src/queen.c bin/lib_hive_ipc.o bin/lib_logger.o bin/logger_internal.o

.PHONY: clean
clean:
	rm -rf bin