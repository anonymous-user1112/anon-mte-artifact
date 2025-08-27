CC = gcc
CFLAGS = -march=armv8.5-a+memtag -O0
EXECS:=stg_singlepoint

%: %.c
	$(CC) $(CFLAGS) $^ -o $@

default: $(EXECS)

run: $(EXECS)
	bench_runner cxc $@ 0
	bench_runner cxc $@ 1
	bench_runner cxc $@ 2
	bench_runner cxc $@ 3

clean:
	rm $(EXECS)
