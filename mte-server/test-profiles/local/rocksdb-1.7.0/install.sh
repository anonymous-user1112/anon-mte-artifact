#!/bin/sh
tar -xf rocksdb-10.0.1.tar.gz
cd rocksdb-10.0.1

# Build fix early 2025...
echo '#include <cstdint>' | cat - db/blob/blob_file_meta.h > temp && mv temp db/blob/blob_file_meta.h
echo '#include <cstdint>' | cat - include/rocksdb/write_batch_base.h > temp && mv temp include/rocksdb/write_batch_base.h
echo '#include <cstdint>' | cat - include/rocksdb/trace_record.h > temp && mv temp include/rocksdb/trace_record.h
mkdir build
cd build
export CFLAGS="-O3 -march=native -Wno-error=maybe-uninitialized -Wno-error=uninitialized -Wno-error=deprecated-copy -Wno-error=pessimizing-move $CFLAGS"
export CXXFLAGS="-O3 -march=native -Wno-error=maybe-uninitialized -Wno-error=uninitialized -Wno-error=deprecated-copy -Wno-error=pessimizing-move $CXXFLAGS"
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_SNAPPY=ON  ..
make -j $NUM_CPU_CORES
make db_bench
echo $? > ~/install-exit-status
cd ~
echo "#!/bin/bash
rm -rf /tmp/rocksdbtest-1000/dbbench/
cd rocksdb-10.0.1/build/
\$LOADER env GLIBC_TUNABLES=glibc.mem.tagging=0 ./db_bench \$@ --threads \$NUM_CPU_CORES --duration 60 > \$LOG_FILE 2>&1
echo \$? > ~/test-exit-status
rm -rf /tmp/rocksdbtest-1000/dbbench/" > rocksdb
chmod +x rocksdb
