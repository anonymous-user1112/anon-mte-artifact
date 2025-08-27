#!/usr/bin/env bash
# Usage: ./wrapper.sh clang a.c -o a.out
#        ./wrapper.sh clang++ test.cpp -O2
set -e

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <compiler> <args...>"
    exit 1
fi

REAL_CLANG=$(command -v "$1")  # first arg is compiler
shift                          # remove compiler from args

LFI_REWRITER=/home/XXXXXXXX/code/lfi-rewriter/build/lfi-rewrite/lfi-rewrite
ARGS=("$@")
OUTFILE=""
SRCFILES=()
NEWARGS=()

# Parse arguments to detect output file and source files
for ((i=0; i<${#ARGS[@]}; i++)); do
    arg="${ARGS[$i]}"
    if [[ "$arg" == "-o" ]]; then
        OUTFILE="${ARGS[$((i+1))]}"
        i=$((i+1))
    elif [[ "$arg" == *.c || "$arg" == *.cpp || "$arg" == *.cc || "$arg" == *.cxx || "$arg" == *.S ]]; then
        SRCFILES+=("$arg")
    else
        NEWARGS+=("$arg")
    fi
done

# Default output if not provided
if [[ -z "$OUTFILE" ]]; then
    OUTFILE="a.out"
fi

# We will generate one assembly per source
for SRC in "${SRCFILES[@]}"; do
    BASENAME="$SRC"
    ASMFILE="${BASENAME%.*}.s"
    OUT_ASMFILE="${BASENAME%.*}.lfi.s"
    OBJFILE="${BASENAME%.*}.o"

    # 1. Emit assembly
    # Need `-ffixed-x18 -mllvm -aarch64-enable-compress-jump-tables=0`, spec config also has these
    "$REAL_CLANG" -S "$SRC" "${NEWARGS[@]}" -ffixed-x18 -mllvm -aarch64-enable-compress-jump-tables=0 -o "$ASMFILE" || exit 1

    "$LFI_REWRITER" -a arm64 "$ASMFILE" > "$OUT_ASMFILE"

    # 2. Assemble to object
    "$REAL_CLANG" -c "$OUT_ASMFILE" -o "$OBJFILE" || exit 1

    OBJFILES+=("$OBJFILE")
done

# If output is an object file, just move it; otherwise link
if [[ "$OUTFILE" != *.o ]]; then
    # Linking
    "$REAL_CLANG" "${NEWARGS[@]}" -o "$OUTFILE" || exit 1
fi

# Optional: remove temporary .S files
# rm -f *.S
