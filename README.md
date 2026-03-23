# cx_runtime
Repository for cx_table

Whenever using this repo, make sure to run `source settings.sh` to configure your environment.

Make sure to install makeinfo before starting: `sudo apt-get install texinfo`

1. run `git submodule update --init`

**Run from utils directory.**

2. run `./build-riscv-toolchain.sh`

**Run from root directory**

3. run `make all`.

**Run all the follow commands from the utils directory.**

4. run `./build-qemu.sh`

5. run `./build-linux.sh`

6. run `./build-busybox.sh`

7. run `./build-files.sh`

Finally, to run QEMU, run `make qemu` from the root directory. 