#!/bin/sh
set -x
$LOADER env GLIBC_TUNABLES=glibc.mem.tagging=$BENCH_MTE_MODE ./nginx_/sbin/nginx -g "worker_processes auto;"
sleep 5
