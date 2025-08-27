#!/bin/sh
INSTALL_DIR=$PWD
tar -xzvf memcached-1.6.19.tar.gz
cd memcached-1.6.19
# Allow run as root
patch -p0 <<'EOF'
--- memcached.c.orig	2022-08-19 13:51:28.705427565 -0400
+++ memcached.c	2022-08-19 13:52:03.426871004 -0400
@@ -5810,7 +5810,7 @@
     }
 
     /* lose root privileges if we have them */
-    if (getuid() == 0 || geteuid() == 0) {
+    if (false) {
         if (username == 0 || *username == '\0') {
             fprintf(stderr, "can't run as root without the -u switch\n");
             exit(EX_USAGE);
EOF

./configure
make
retVal=$?
if [ $retVal -ne 0 ]; then
	echo $retVal > ~/install-exit-status
	exit $retVal
fi
cd ..
tar -xf memtier_benchmark-1.3.0.tar.gz
cd memtier_benchmark-1.3.0/
autoreconf -ivf
./configure
make -j $NUM_CPU_CORES
echo $? > ~/install-exit-status

cd ..
echo "#!/bin/sh
set -e
cd memcached-1.6.19
PORT=7777
\$LOADER env GLIBC_TUNABLES=glibc.mem.tagging=0 ./memcached --port=\$PORT -c 4096 -t \$NUM_CPU_CORES &
MEMCACHED_PID=\$!
sleep 6
cd ${INSTALL_DIR}/memtier_benchmark-1.3.0/
./memtier_benchmark --hide-histogram -t \$NUM_CPU_CORES -p \$PORT \$@ > \$LOG_FILE
kill \$MEMCACHED_PID" > memcached
chmod +x memcached

