BINDIR := ../bin

CC ?= riscv64-linux-gnu-gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c17
CFLAGS_STATIC := -nostdlib -nodefaultlibs -static

TESTS := minimal basic basic_indirect main_static
TARGETS := $(patsubst %,$(BINDIR)/%.rv64,$(TESTS))

.phony: all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

$(BINDIR)/minimal.rv64: minimal.s
	$(CC) $(CFLAGS) $(CFLAGS_STATIC) -o $@ $<

$(BINDIR)/basic.rv64: basic.c _exit.s
	$(CC) $(CFLAGS) $(CFLAGS_STATIC) -march=rv64g -o $@ $^

$(BINDIR)/basic_indirect.rv64: basic.c _exit.s
	$(CC) $(CFLAGS) $(CFLAGS_STATIC) -march=rv64g -DINDIRECT=1 -o $@ $^

$(BINDIR)/main_static.rv64: main.c
	$(CC) $(CFLAGS) -static -march=rv64g -o $@ $^
