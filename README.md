
RTDL
----------------------------------------

This software implements functionality which can be used to make
software with support for run-time updating and different kinds of
task migration.

  Usage
----------------------------------------

If you want to see if the software actually works there is a simple
way to do this.

1. Download and compile linaros qemu port of the VersaTile Express
   Cortex-A9. (clone from git://git.linaro.org/qemu/qemu-linaro.git)

2. Get the devkitARM toolchain r37 (this is the one we have used, so
   it should work).

3. Get and install the ninja build system
   (http://martine.github.io/ninja/)

4. Run python gen_build_ninja.py && ninja

5. Insert the correct path of your qemu binary into start_qemu.sh and
   run ./start_qemu.sh bin/rtudemo.bin

