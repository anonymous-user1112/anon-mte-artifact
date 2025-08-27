#!/bin/bash

declare -a suites=(default mtekmod mtesig mprotsig)
for suite in "${suites[@]}"
do
	out="$suite.txt"
  tail -n7 $out | awk -v var="$suite" '/rsa [ ]?[0-9]+ bits/ { sub(/s$/, "", $4); sub(/s$/, "", $5); printf "%s %s %s %s\n", var, $2, $4, $5; }'
done
