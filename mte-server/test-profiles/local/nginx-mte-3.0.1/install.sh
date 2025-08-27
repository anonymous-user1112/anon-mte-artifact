#!/bin/bash
set -e
INSTALL_DIR=$PWD
mkdir $INSTALL_DIR/nginx_
tar -xf http-test-files-1.tar.xz
tar -xf release-1.23.3.tar.gz
mv nginx-release-1.23.3 nginx-1.23.3
cd nginx-1.23.3

cat > nginx-mte.patch  <<'EOF'
diff -Naur nginx-release-1.23.3/src/os/unix/ngx_files.c nginx-1.23.3/src/os/unix/ngx_files.c
--- nginx-release-1.23.3/src/os/unix/ngx_files.c	2022-12-13 07:53:53.000000000 -0800
+++ nginx-1.23.3/src/os/unix/ngx_files.c	2025-08-21 16:50:49.576624758 -0700
@@ -633,7 +633,7 @@
         goto failed;
     }
 
-    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
+    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE|PROT_MTE, MAP_SHARED,
                     fm->fd, 0);
     if (fm->addr != MAP_FAILED) {
         return NGX_OK;
diff -Naur nginx-release-1.23.3/src/os/unix/ngx_shmem.c nginx-1.23.3/src/os/unix/ngx_shmem.c
--- nginx-release-1.23.3/src/os/unix/ngx_shmem.c	2022-12-13 07:53:53.000000000 -0800
+++ nginx-1.23.3/src/os/unix/ngx_shmem.c	2025-08-21 16:50:23.505708982 -0700
@@ -15,7 +15,7 @@
 ngx_shm_alloc(ngx_shm_t *shm)
 {
     shm->addr = (u_char *) mmap(NULL, shm->size,
-                                PROT_READ|PROT_WRITE,
+                                PROT_READ|PROT_WRITE|PROT_MTE,
                                 MAP_ANON|MAP_SHARED, -1, 0);
 
     if (shm->addr == MAP_FAILED) {
EOF
patch -p1 < nginx-mte.patch

CFLAGS="-Wno-error -O3 -march=native $CFLAGS" CXXFLAGS="-Wno-error -O3 -march=native $CFLAGS" auto/configure --prefix=$INSTALL_DIR/nginx_ --without-http_rewrite_module --without-http-cache  --with-http_ssl_module
make -j $NUM_CPU_CORES
echo $? > ~/install-exit-status
make install
cd ..
# rm -rf nginx-1.23.3
openssl req -new -newkey rsa:4096 -days 365 -nodes -x509 -subj "/C=US/ST=Denial/L=Chicago/O=Dis/CN=127.0.0.1" -keyout localhost.key  -out localhost.cert
sed -i "s/worker_processes  1;/#worker_processes  auto;/g" nginx_/conf/nginx.conf
sed -i "s/        listen       80;/        listen       8089;/g" nginx_/conf/nginx.conf
sed -i "38 i ssl                  on;" nginx_/conf/nginx.conf
sed -i "38 i ssl_certificate      $INSTALL_DIR/localhost.cert;" nginx_/conf/nginx.conf
sed -i "38 i ssl_certificate_key   $INSTALL_DIR/localhost.key;" nginx_/conf/nginx.conf
sed -i "38 i ssl_ciphers          HIGH:!aNULL:!MD5;" nginx_/conf/nginx.conf
rm -rf wrk-4.2.0
tar -xf wrk-4.2.0.tar.gz
cd wrk-4.2.0
make -j $NUM_CPU_CORES
echo $? > ~/install-exit-status
cd ..
echo $PWD
mv -f http-test-files/* nginx_/html/
echo "#!/bin/sh
./wrk-4.2.0/wrk -t \$NUM_CPU_CORES \$@ > \$LOG_FILE 2>&1
echo \$? > ~/test-exit-status" > nginx-mte
chmod +x nginx-mte
