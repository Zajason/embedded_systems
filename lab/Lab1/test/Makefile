CC     ?= cc
CFLAGS ?= -std=c99 -O0 -g -Wall -Wextra -Wno-unused-function -fsanitize=undefined -fno-sanitize-recover=all

host_test: host_test.c $(wildcard ../src/*.h)
	$(CC) $(CFLAGS) -I../src -o $@ host_test.c

run: host_test
	./host_test

clean:
	rm -f host_test
	rm -rf host_test.dSYM

.PHONY: run clean
