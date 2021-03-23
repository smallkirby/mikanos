SHELL=/bin/bash

all:
	clang++ -O2 -Wall -g \
		${CPPFLAGS} \
		--target=x86_64-elf \
		-ffreestanding \
		-mno-red-zone \
		-fno-exceptions \
		-fno-rtti \
		-std=c++17 \
		-c main.cpp
	ld.lld ${LDFLAGS} --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o

clean:
	rm -f kernel.elf
	rm -f main.o
