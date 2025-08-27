#!/bin/sh

# SERIAL=3A201FDJG003S2
SERIAL=41290DLJG000LP
OUTDIR=/data/misc/perfetto-traces

if [ -z "$1" ]; then
  echo "Usage: ./<script> <output_name>"
  exit
fi

NAME=$1
OUT=$OUTDIR/"$NAME".trace

adb -s $SERIAL shell -t \
  perfetto -c - --txt \
  --out "$OUT" \
<<EOF
duration_ms: 660000
buffers {
  size_kb: 65536
  fill_policy: DISCARD
}
data_sources: {
    config {
        name: "android.power"
        target_buffer: 0
        android_power_config {
            battery_poll_ms: 250
            collect_power_rails: true
        }
    }
}
EOF

adb -s $SERIAL pull "$OUT" ./
