CC = aarch64-linux-gnu-g++
CFLAGS = -Wall -Wextra -O0 -march=armv8.5-a+memtag -static
INCLUDES = -I../util

# Directories
UTIL_DIR = ../util

# Source files
UTIL_SRC = $(UTIL_DIR)/util.cpp
HEATMAP_SRC = two_level.cpp

# Output executables
HEATMAP_EXE = two_level


# Default target
all: $(HEATMAP_EXE)

# Compile two_level executable
$(HEATMAP_EXE): $(HEATMAP_SRC) $(UTIL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^


# Clean build files
clean:
	rm -rf $(HEATMAP_EXE)

upload: all
	adb -s 41290DLJG000LP push $(HEATMAP_EXE) /data/androdeb/debian/XXXXXXXX/heatmap
	adb -s 41290DLJG000LP push run.sh /data/androdeb/debian/XXXXXXXX/heatmap

download: all
	adb -s 41290DLJG000LP pull /data/androdeb/debian/XXXXXXXX/heatmap/result_warm ./
	adb -s 41290DLJG000LP pull /data/androdeb/debian/XXXXXXXX/heatmap/result_cold ./

.PHONY: all clean