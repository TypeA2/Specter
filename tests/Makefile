CC32 := gcc
CC64 := $(CC32)
CFLAGS := -Wall -Wextra -Wpedantic -std=c17
CFLAGS32 := -m32
CFLAGS64 := -m64

CC32_RV := riscv32-linux-gnu-$(CC32)
CC64_RV := riscv64-linux-gnu-$(CC64)
CFLAGS_RV := $(CFLAGS)
CFLAGS32_RV := 
CFLAGS64_RV := 

SRC := $(wildcard src/*.c)

# TARGETS_X86 := $(patsubst src/%.c,bin/%.x86,$(SRC))
# TARGETS_RV32 := $(patsubst src/%.c,bin/%.rv32,$(SRC))
TARGETS_AMD64 := $(patsubst src/%.c,bin/%.amd64,$(SRC))
TARGETS_RV64 := $(patsubst src/%.c,bin/%.rv64,$(SRC))

TARGETS := $(TARGETS_X86) $(TARGETS_RV32) $(TARGETS_AMD64) $(TARGETS_RV64)

define testname
	$(notdir $(basname $(1)))
endef

.phony: all clean

all: $(TARGETS)

bin/%.x86: src/%.c
	$(CC32) $(CFLAGS) $(CFLAGS32) -o $@ $<

bin/%.rv32: src/%.c
	$(CC32_RV) $(CFLAGS_RV) $(CLFAGS32_RV) -o $@ $<

bin/%.amd64: src/%.c
	$(CC64) $(CFLAGS) $(CFLAGS64) -o $@ $<

bin/%.rv64: src/%.c
	$(CC64_RV) $(CFLAGS_RV) $(CFLAGS64_RV) -o $@ $<

clean:
	rm -f $(TARGETS)
