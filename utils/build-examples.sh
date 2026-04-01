#!/bin/bash
# Compiles all C files in examples/ and tests/ and puts executables in qemu-share/

SHARE_DIR="$CX_ROOT/qemu-share"
LIB_DIR="$CX_ROOT/build/lib"
CC="riscv32-unknown-linux-gnu-gcc"

if [ -z "$CX_ROOT" ]; then
    echo "CX_ROOT not set. Run 'source settings.sh' first."
    exit 1
fi

# Delete existing executables from qemu-share
echo "Cleaning qemu-share executables..."
find "$SHARE_DIR" -maxdepth 1 -type f -executable -not -name "*.sh" -not -name "*.txt" -delete

compile() {
    local src=$1
    local out=$2
    local extra=$3
    echo "  Compiling $(basename $src) -> $(basename $out)"
    $CC -static "$src" -o "$out" -L "$LIB_DIR" -lci $extra
    if [ $? -ne 0 ]; then
        echo "  FAILED: $src"
    fi
}

echo "Compiling examples/..."
for src in "$CX_ROOT"/examples/*.c; do
    name=$(basename "$src" .c)
    compile "$src" "$SHARE_DIR/$name"
done

echo "Compiling tests/..."
for src in "$CX_ROOT"/tests/*.c; do
    name=$(basename "$src" .c)
    # thread tests need -pthread
    if grep -q "pthread" "$src"; then
        compile "$src" "$SHARE_DIR/$name" "-pthread"
    else
        compile "$src" "$SHARE_DIR/$name"
    fi
done

echo "Done."