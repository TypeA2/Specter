BINDIR := ../bin

CC := riscv64-linux-gnu-gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c17
CFLAGS_STATIC := -nostdlib -nodefaultlibs -static

TESTS := minimal basic
TARGETS := $(patsubst %,$(BINDIR)/%.rv64,$(TESTS))

.phony: all clean

all: $(TARGETS)

clean:
	rm -f $(TARGETS)

$(BINDIR)/minimal.rv64: minimal.s
	$(CC) $(CFLAGS) $(CFLAGS_STATIC) -o $@ $<

$(BINDIR)/basic.rv64: basic.c
	$(CC) $(CFLAGS) -nostartfiles -static -o $@ $<
