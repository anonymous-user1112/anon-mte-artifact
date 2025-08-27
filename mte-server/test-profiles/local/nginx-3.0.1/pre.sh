#!/bin/sh
set -x
$LOADER env GLIBC_TUNABLES=glibc.mem.tagging=0 ./nginx_/sbin/nginx -g "worker_processes auto;"
sleep 5
