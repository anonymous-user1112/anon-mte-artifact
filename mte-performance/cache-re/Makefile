# Compiler and flags
CC := clang++
CFLAGS := -v -target aarch64-linux-gnu -march=armv8.5-a+memtag+sve -static -O0 -I../util/

# Source files
SRC := cache.cpp 
UTIL_SRC := ../util/util.c 
LFSR_SRC := ../util/LFSR.cpp 
LFSR_util_SRC := ../util/LFSR_util.cpp 

# Object files
OBJS := $(SRC:.cpp=.o) $(UTIL_SRC:.c=.o) $(LFSR_SRC:.cpp=.o) $(LFSR_util_SRC:.cpp=.o)

# Target executable
TARGET := cache 

# Default target
all: $(TARGET)

# Linking step
cache: cache.o ../util/util.o ../util/LFSR.o ../util/LFSR_util.o
	$(CC) $(CFLAGS) -o $@ cache.o ../util/util.o ../util/LFSR.o ../util/LFSR_util.o

# Compilation step for each .c file
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
	
# Clean up build files
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)