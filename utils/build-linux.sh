# Builds linux for riscv32 with added cx runtime calls
# Based on https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html


pushd ../linux_cx

if [ $? -ne 0 ]; then
	echo "Linux subdirectory not found - remember to clone submodules for linux + QEMU."
    exit 1
fi

git checkout cx_table_rebase

if [ $? -ne 0 ]; then
	echo "Couldn't find cx_table branch for linux."
    exit 1
fi

make ARCH=riscv CROSS_COMPILE=riscv32-unknown-linux-gnu- rv32_defconfig
make ARCH=riscv CROSS_COMPILE=riscv32-unknown-linux-gnu- -j $(nproc)

popd
