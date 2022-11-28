TESTS := $(wildcard *.s)
BINS  := $(patsubst %.s,%.rv64,$(TESTS))

all: $(BINS)

clean:
	rm -f $(BINS)

%.rv64: %.s %.toml
	riscv64-linux-gnu-gcc -Ttext=0x10000 -Tdata=0x20000 -Wl,-e,_start \
		-Wall -O0 -nostdlib -fno-builtin -nodefaultlibs -static -o $@ $<
