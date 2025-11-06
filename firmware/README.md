# TermDriver2 RP2040 Firmware

Firmware project for the TermDriver2, built using the official [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) and CMake.

---

## Overview

This repository contains source code and build configuration for firmware
targeting the RP2040 dual-core ARM Cortex-M0+ MCU. It is structured for
compatibility with the standard Pico SDK and can be built for both the
Raspberry Pi Pico board and custom RP2040-based designs.

---

## Requirements

- **Toolchain**
  - ARM GCC (`arm-none-eabi-gcc`)
  - CMake ≥ 3.13
  - Ninja or Make
  - OpenOCD (for flashing/debugging)

- **Dependencies**
  - [pico-sdk](https://github.com/raspberrypi/pico-sdk)
  - Optionally: [pico-extras](https://github.com/raspberrypi/pico-extras) for additional libraries

Ensure that `PICO_SDK_PATH` is set in your environment or provided to CMake.

```
export PICO_SDK_PATH=/path/to/pico-sdk

git clone --recursive https://github.com/yourname/rp2040-firmware.git
cd rp2040-firmware
mkdir build && cd build
cmake .. -DPICO_SDK_PATH=$PICO_SDK_PATH
make -j
```

The resulting `td2.uf2` and `td2.elf` binaries will appear in the build directory.

To load `td2.uf2` on the TermDriver2 hardware, the easiest way is to reset the TermDriver to UF2 bootloader mode,
as described in the [user Guide](https://termdriver.com/termdriver2.pdf).

You can do this by hand, or 
use the script in `demos/reset_to_uf2.py` that puts the TermDriver into UF2 bootloader mode, e.g.

    python ../demos/reset_to_uf2.py COM4:
