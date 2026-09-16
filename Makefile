# Target kernel module name
obj-m := nox.o

# Source object files linked into nox.ko
nox-y := main.o BloomFilter/BloomFilter.o Hash/murmur3.o

CFLAGS_Hash/murmur3.o += -Wno-implicit-fallthrough

