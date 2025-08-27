#!/bin/sh
tar -xf rocksdb-10.0.1.tar.gz
cd rocksdb-10.0.1

cat > rocksdb-mte.patch <<'EOF'
--- rocksdb-10.0.1-orig/port/mmap.cc	2025-03-06 17:32:20.000000000 -0800
+++ rocksdb-10.0.1/port/mmap.cc	2025-08-21 10:48:11.044853378 -0700
@@ -78,7 +78,7 @@
     huge_flag = MAP_HUGETLB;
 #endif  // MAP_HUGE_TLB
   }
-  mm.addr_ = mmap(nullptr, length, PROT_READ | PROT_WRITE,
+  mm.addr_ = mmap(nullptr, length, PROT_READ | PROT_WRITE | PROT_MTE,
                   MAP_PRIVATE | MAP_ANONYMOUS | huge_flag, -1, 0);
   if (mm.addr_ == MAP_FAILED) {
     mm.addr_ = nullptr;
--- rocksdb-10.0.1-orig/env/io_posix.cc	2025-03-06 17:32:20.000000000 -0800
+++ rocksdb-10.0.1/env/io_posix.cc	2025-08-21 10:58:03.143238845 -0700
@@ -1104,7 +1104,7 @@
   }

   TEST_KILL_RANDOM("PosixMmapFile::Append:1");
-  void* ptr = mmap(nullptr, map_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
+  void* ptr = mmap(nullptr, map_size_, PROT_READ | PROT_WRITE | PROT_MTE, MAP_SHARED, fd_,
                    file_offset_);
   if (ptr == MAP_FAILED) {
     return IOStatus::IOError("MMap failed on " + filename_);
--- rocksdb-10.0.1-orig/env/fs_posix.cc	2025-03-06 17:32:20.000000000 -0800
+++ rocksdb-10.0.1/env/fs_posix.cc	2025-08-21 11:02:48.280913348 -0700
@@ -245,7 +245,7 @@
       IOOptions opts;
       s = GetFileSize(fname, opts, &size, nullptr);
       if (s.ok()) {
-        void* base = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
+        void* base = mmap(nullptr, size, PROT_READ | PROT_MTE, MAP_SHARED, fd, 0);
         if (base != MAP_FAILED) {
           result->reset(
               new PosixMmapReadableFile(fd, fname, base, size, options));
@@ -503,7 +503,7 @@
     }
     void* base = nullptr;
     if (status.ok()) {
-      base = mmap(nullptr, static_cast<size_t>(size), PROT_READ | PROT_WRITE,
+      base = mmap(nullptr, static_cast<size_t>(size), PROT_READ | PROT_WRITE | PROT_MTE,
                   MAP_SHARED, fd, 0);
       if (base == MAP_FAILED) {
         status = IOError("while mmap file for read", fname, errno);
EOF
patch -p1 < rocksdb-mte.patch

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
\$LOADER env GLIBC_TUNABLES=glibc.mem.tagging=\$BENCH_MTE_MODE ./db_bench \$@ --threads \$NUM_CPU_CORES --duration 60 > \$LOG_FILE 2>&1
echo \$? > ~/test-exit-status
rm -rf /tmp/rocksdbtest-1000/dbbench/" > rocksdb-mte
chmod +x rocksdb-mte
