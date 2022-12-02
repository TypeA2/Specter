TESTS := $(wildcard *.s)
BINS  := $(patsubst %.s,%.rv64,$(TESTS))

CC := riscv64-linux-gnu-gcc
LINKER := -Ttext=0x10000 -Tdata=0x20000 -Wl,-e,_start
CFLAGS := -Wall -O0 -nostdlib -fno-builtin -nodefaultlibs -static

all: $(BINS)

clean:
	rm -f $(BINS)

c.%.rv64: c.%.s c.%.toml
	$(CC) $(LINKER) $(CFLAGS) -march=rv64gc -o $@ $<

%.rv64: %.s %.toml
	$(CC) $(LINKER) $(CFLAGS) -march=rv64g -o $@ $<
