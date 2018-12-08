CC ?=gcc
#CFLAGS ?= -std=gnu99 -Werror -Wfatal-errors -Wall -Wextra -pedantic -Og -g -ggdb -pthread -D DEBUG= -fsanitize=address -fsanitize=leak -fsanitize=undefined
CFLAGS ?= -O3 -std=gnu99 -pthread

.PHONY: all clean test test1

#target

list.o: list.c
	$(CC) $(CFLAGS) -c $< -o $@ -lm
parser.o: parser.c list.o
	$(CC) $(CFLAGS) -c $< -o $@ -lm
input.o: input.c
	$(CC) $(CFLAGS) -c $< -o $@ -lm
scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c $< -o $@ -lm
simulator: simulator.c scheduler.o list.o input.o parser.o
	$(CC) $(CFLAGS) $< -o $@ -lm

# Phony
all: input.o list.o parser.o scheduler.o simulator

test: test1 test2 test3 test4 test5

test1: all
	./simulator -i input_files/01_tasks.csv -on output-1-np.log -op output-1-pr.log
test2: all
	./simulator -i input_files/02_tasks.csv -on output-2-np.log -op output-2-pr.log
test3: all
	./simulator -i input_files/03_tasks.csv -on output-3-np.log -op output-3-pr.log
test4: all
	./simulator -i input_files/04_tasks.csv -on output-4-np.log -op output-4-pr.log
test5: all
	./simulator -i input_files/05_tasks.csv -on output-5-np.log -op output-5-pr.log

clean:
	rm -f *.o
	rm -f *.csv
	rm -f simulator