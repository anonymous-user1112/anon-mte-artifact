CC = aarch64-linux-gnu-g++
CFLAGS = -Wall -Wextra -O0 -march=armv8.5-a+memtag -static
INCLUDES = -I../util

# Directories
UTIL_DIR = ../util

# Source files
UTIL_SRC = $(UTIL_DIR)/util.cpp
micro_SRC = micro.cpp
micro_single_SRC = micro_single.cpp

# Output executables
micro_EXE = micro 
micro_single_EXE = micro_single


# Default target
all: $(micro_EXE) $(micro_single_EXE)

# Compile two_level executable
$(micro_EXE): $(micro_SRC) $(UTIL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

$(micro_single_EXE): $(micro_single_SRC) $(UTIL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^

# Clean build files
clean:
	rm -rf $(micro_EXE) $(micro_single_EXE)

upload: all
	adb -s 41290DLJG000LP push $(micro_EXE) /data/androdeb/debian/XXXXXXXX/microbench
	adb -s 41290DLJG000LP push $(micro_single_EXE) /data/androdeb/debian/XXXXXXXX/microbench
	adb -s 41290DLJG000LP push run.sh /data/androdeb/debian/XXXXXXXX/microbench
	adb -s 41290DLJG000LP push run_perf.sh /data/androdeb/debian/XXXXXXXX/microbench


download: all
	adb -s 41290DLJG000LP pull /data/androdeb/debian/XXXXXXXX/microbench/result_micro ./

.PHONY: all clean