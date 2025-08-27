NOTPARALLEL:

.DEFAULT_GOAL := build

SHELL := /bin/bash

CURR_USER=${USER}
CURR_PATH=${PATH}
CURR_TIME=$(shell date --iso=seconds)
PARALLEL_COUNT=$(shell nproc)
ROOT_PATH=$(shell realpath .)

DIRS=mtetrap openssl-mte firefox-mte

ifeq ($(shell uname --hardware-platform), x86_64)
wasi-sdk-23.0-linux.tar.gz:
	wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-23/wasi-sdk-23.0-x86_64-linux.tar.gz -O $@
else
wasi-sdk-23.0-linux.tar.gz:
	wget https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-23/wasi-sdk-23.0-arm64-linux.tar.gz -O $@
endif

wasi-sdk: wasi-sdk-23.0-linux.tar.gz
	mkdir -p $@
	tar -zxf $< -C $@ --strip-components 1

get_source: $(addprefix fetch_,$(DIRS)) wasi-sdk
	touch ./get_source

fetch_%:
	if [ ! -e "./$*" ]; then \
		git clone --recursive git@github.com:XXXXXXXX/$*.git; \
	fi

bootstrap:
	echo "Bootstrapping"
	sudo apt install -y gcc g++ clang make cmake nasm \
		python3 python3-dev python-is-python3 python3-pip \
		cpuset cpufrequtils curl gnuplot \
		build-essential g++-multilib \
		jq
	$(MAKE) get_source
	pip3 install simplejson matplotlib
	cd mtetrap && $(MAKE) bootstrap
	touch ./$@
