.. zephyr:board:: esp32c6_devkitc

Overview
********

ESP32-C6-DevKitC is an entry-level development board based on ESP32-C6-WROOM-1(U),
a general-purpose module with a 8 MB SPI flash. This board integrates complete Wi-Fi,
Bluetooth LE, Zigbee, and Thread functions. For more information, check `ESP32-C6-DevKitC`_.

Hardware
********

ESP32-C6 is Espressif's first Wi-Fi 6 SoC integrating 2.4 GHz Wi-Fi 6, Bluetooth 5.3 (LE) and the
802.15.4 protocol. ESP32-C6 achieves an industry-leading RF performance, with reliable security
features and multiple memory resources for IoT products.
It consists of a high-performance (HP) 32-bit RISC-V processor, which can be clocked up to 160 MHz,
and a low-power (LP) 32-bit RISC-V processor, which can be clocked up to 20 MHz.
It has a 320KB ROM, a 512KB SRAM, and works with external flash.

ESP32-C6-DevKitC is an entry-level development board based on ESP32-C6-WROOM-1(U),
a general-purpose module with a 8 MB SPI flash.

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing.
Developers can either connect peripherals with jumper wires or mount ESP32-C6-DevKitC on
a breadboard.

ESP32-C6 includes the following features:

- 32-bit core RISC-V microcontroller with a clock speed of up to 160 MHz
- 400 KB of internal RAM
- WiFi 802.11 ax 2.4GHz
- Fully compatible with IEEE 802.11b/g/n protocol
- Bluetooth LE: Bluetooth 5.3 certified
- Internal co-existence mechanism between Wi-Fi and Bluetooth to share the same antenna
- IEEE 802.15.4 (Zigbee and Thread)

Digital interfaces:

- 30x GPIOs (QFN40), or 22x GPIOs (QFN32)
- 2x UART
- 1x Low-power (LP) UART
- 1x General purpose SPI
- 1x I2C
- 1x Low-power (LP) I2C
- 1x I2S
- 1x Pulse counter
- 1x USB Serial/JTAG controller
- 1x TWAI® controller, compatible with ISO 11898-1 (CAN Specification 2.0)
- 1x SDIO 2.0 slave controller
- LED PWM controller, up to 6 channels
- 1x Motor control PWM (MCPWM)
- 1x Remote control peripehral
- 1x Parallel IO interface (PARLIO)
- General DMA controller (GDMA), with 3 transmit channels and 3 receive channels
- Event task matrix (ETM)

Analog interfaces:

- 1x 12-bit SAR ADCs, up to 7 channels
- 1x temperature sensor

Timers:

- 1x 52-bit system timer
- 1x 54-bit general-purpose timers
- 3x Watchdog timers
- 1x Analog watchdog timer

Low Power:

- Four power modes designed for typical scenarios: Active, Modem-sleep, Light-sleep, Deep-sleep

Security:

- Secure boot
- Flash encryption
- 4-Kbit OTP, up to 1792 bits for users
- Cryptographic hardware acceleration: (AES-128/256, ECC, HMAC, RSA, SHA, Digital signature, Hash)
- Random number generator (RNG)

For more information, check the datasheet at `ESP32-C6 Datasheet`_ or the technical reference
manual at `ESP32-C6 Technical Reference Manual`_.

Supported Features
==================

.. zephyr:board-supported-hw::

System requirements
*******************

Prerequisites
=============

Espressif HAL requires WiFi and Bluetooth binary blobs in order work. Run the command
below to retrieve those files.

.. code-block:: console

   west blobs fetch hal_espressif

.. note::

   It is recommended running the command above after :file:`west update`.

Building & Flashing
*******************

.. zephyr:board-supported-runners::

Simple boot
===========

The board could be loaded using the single binary image, without 2nd stage bootloader.
It is the default option when building the application without additional configuration.

.. note::

   Simple boot does not provide any security features nor OTA updates.

MCUboot bootloader
==================

User may choose to use MCUboot bootloader instead. In that case the bootloader
must be built (and flashed) at least once.

There are two options to be used when building an application:

1. Sysbuild
2. Manual build

.. note::

   User can select the MCUboot bootloader by adding the following line
   to the board default configuration file.

   .. code:: cfg

      CONFIG_BOOTLOADER_MCUBOOT=y

Sysbuild
========

The sysbuild makes possible to build and flash all necessary images needed to
bootstrap the board with the EPS32 SoC.

To build the sample application using sysbuild use the command:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build
   :west-args: --sysbuild
   :compact:

By default, the ESP32 sysbuild creates bootloader (MCUboot) and application
images. But it can be configured to create other kind of images.

Build directory structure created by sysbuild is different from traditional
Zephyr build. Output is structured by the domain subdirectories:

.. code-block::

  build/
  ├── hello_world
  │   └── zephyr
  │       ├── zephyr.elf
  │       └── zephyr.bin
  ├── mcuboot
  │    └── zephyr
  │       ├── zephyr.elf
  │       └── zephyr.bin
  └── domains.yaml

.. note::

   With ``--sysbuild`` option the bootloader will be re-build and re-flash
   every time the pristine build is used.

For more information about the system build please read the :ref:`sysbuild` documentation.

Manual build
============

During the development cycle, it is intended to build & flash as quickly possible.
For that reason, images can be built one at a time using traditional build.

The instructions following are relevant for both manual build and sysbuild.
The only difference is the structure of the build directory.

.. note::

   Remember that bootloader (MCUboot) needs to be flash at least once.

Build and flash applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build

The usual ``flash`` target will work with the ``esp32c6_devkitc`` board
configuration. Here is an example for the :zephyr:code-sample:`hello_world`
application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: flash

Open the serial monitor using the following command:

.. code-block:: shell

   west espressif monitor

After the board has automatically reset and booted, you should see the following
message in the monitor:

.. code-block:: console

   ***** Booting Zephyr OS vx.x.x-xxx-gxxxxxxxxxxxx *****
   Hello World! esp32c6_devkitc/esp32c6/hpcore

.. include:: ../../../espressif/common/board-variants.rst
   :start-after: espressif-board-variants

Debugging
*********

As with much custom hardware, the ESP32-C6 modules require patches to
OpenOCD that are not upstreamed yet. Espressif maintains their own fork of
the project. The custom OpenOCD can be obtained at `OpenOCD ESP32`_.

The Zephyr SDK uses a bundled version of OpenOCD by default. You can overwrite that behavior by adding the
``-DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>``
parameter when building.

Here is an example for building the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: build flash
   :gen-args: -DOPENOCD=<path/to/bin/openocd> -DOPENOCD_DEFAULT_PATH=<path/to/openocd/share/openocd/scripts>

You can debug an application in the usual way. Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: esp32c6_devkitc/esp32c6/hpcore
   :goals: debug

Low-Power CPU (LP CORE)
***********************

The ESP32-C6 SoC has two RISC-V cores: the High-Performance Core (HP CORE) and the Low-Power Core (LP CORE).
The LP Core features ultra low power consumption, an interrupt controller, a debug module and a system bus
interface for memory and peripheral access.

The LP Core is in sleep mode by default. It has two application scenarios:

- Power insensitive scenario: When the High-Performance CPU (HP Core) is active, the LP Core can assist the HP CPU with some speed and efficiency-insensitive controls and computations.
- Power sensitive scenario: When the HP CPU is in the power-down state to save power, the LP Core can be woken up to handle some external wake-up events.

For more information, check the datasheet at `ESP32-C6 Datasheet`_ or the technical reference
manual at `ESP32-C6 Technical Reference Manual`_.

The LP Core support is fully integrated with :ref:`sysbuild`. The user can enable the LP Core by adding
the following configuration to the project:

.. code:: cfg

   CONFIG_ULP_COPROC_ENABLED=y

See :zephyr:code-sample-category:`lp-core` folder as code reference.

References
**********

.. target-notes::

.. _`ESP32-C6-DevKitC`: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32c6/esp32-c6-devkitc-1/user_guide.html
.. _`ESP32-C6 Datasheet`: https://www.espressif.com/sites/default/files/documentation/esp32-c6_datasheet_en.pdf
.. _`ESP32-C6 Technical Reference Manual`: https://espressif.com/sites/default/files/documentation/esp32-c6_technical_reference_manual_en.pdf
.. _`OpenOCD ESP32`: https://github.com/espressif/openocd-esp32/releases
