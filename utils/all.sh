pushd ..
source ./settings.sh
make clean && make
popd
./build-qemu.sh && ./build-linux.sh && \
./build-busybox.sh && ./build-files.sh && ./build-examples.sh
