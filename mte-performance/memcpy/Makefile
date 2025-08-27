# Compiler and flags
CC := aarch64-linux-gnu-gcc
CFLAGS := -march=armv8.5-a+memtag -static

# Source files
SRC := main.c

# Target executable
TARGET := main.o0.FLUSH.bin main.o0.bin main.o2.FLUSH.bin main.o2.bin

# Default target
all: $(TARGET)

main.o0.bin: $(SRC)
	$(CC) $(CFLAGS) -O0 $< -o $@

main.o0.FLUSH.bin: $(SRC)
	$(CC) $(CFLAGS) -O0 $< -o $@ -DDOFLUSH

main.o2.bin: $(SRC)
	$(CC) $(CFLAGS) -O2 $< -o $@

main.o2.FLUSH.bin: $(SRC)
	$(CC) $(CFLAGS) -O2 $< -o $@ -DDOFLUSH

run: $(TARGET)
	./run.sh main.o0.bin
	./run.sh main.o0.FLUSH.bin
	./run.sh main.o2.bin
	./run.sh main.o2.FLUSH.bin

# Clean up build files
.PHONY: clean
clean:
	rm -f $(TARGET)
