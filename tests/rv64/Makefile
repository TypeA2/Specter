BINDIR := ../bin

CC := riscv64-linux-gnu-gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c17
CFLAGS_STATIC := -nostdlib -nodefaultlibs -static

TESTS := minimal
TARGETS := $(patsubst %,$(BINDIR)/%.rv64,$(TESTS))

.phony: all clean

all: $(TARGETS)
	echo $(TARGETS)

$(BINDIR)/minimal.rv64: minimal.s
	$(CC) $(CFLAGS) $(CFLAGS_STATIC) -o $@ $<
