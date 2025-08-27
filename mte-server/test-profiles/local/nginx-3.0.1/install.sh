#!/bin/bash
set -e
INSTALL_DIR=$PWD
mkdir $INSTALL_DIR/nginx_
tar -xf http-test-files-1.tar.xz
tar -xf release-1.23.3.tar.gz
mv nginx-release-1.23.3 nginx-1.23.3
cd nginx-1.23.3

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
echo \$? > ~/test-exit-status" > nginx
chmod +x nginx
