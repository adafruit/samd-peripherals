Introduction
============

.. image:: https://img.shields.io/discord/327254708534116352.svg
    :target: https://adafru.it/discord
    :alt: Discord

.. image:: https://travis-ci.org/adafruit/samd-peripherals.svg?branch=master
    :target: https://travis-ci.org/adafruit/samd-peripherals
    :alt: Build Status

A thin unifying API atop the peripherals in the MicroChip SAMD series of microcontrollers.
It supports the SAMD21 and SAMD51.

It originated in the atmel-samd port of [CircuitPython](https://github.com/adafruit/circuitpython)
and will be strengthened as other people use it.

Using
======
First, add the samd-peripherals repo as a submodule within your own. For example:

.. code-block::

    git submodule add https://github.com/adafruit/samd-peripherals.git peripherals

This will place the files from the repo in a peripherals directory. When your repo is checked out
or updated from before people will need to:

.. code-block::

    git submodule update --init --recursive

The header files in `samd` define the common API between the two series. Logic with most code shared
lives in a corresponding .c file. Functions with mostly different implementations are in a .c file
of the same name under the series specific directory, such as `samd21`. Includes are relative to the
top of the repo so make sure to add the location of the library to your includes path with something
like:

.. code-block::

    -Iperipherals/

In your Makefile create a variable which stores the series such as `CHIP_FAMILY` and alter the
source files depending on it. For example (from [here](https://github.com/adafruit/circuitpython/blob/master/ports/atmel-samd/Makefile)):

.. code-block::

    SRC_C = \
        peripherals/samd/clocks.c \
        peripherals/samd/dma.c \
        peripherals/samd/events.c \
        peripherals/samd/external_interrupts.c \
        peripherals/samd/sercom.c \
        peripherals/samd/timers.c \
        peripherals/samd/$(CHIP_FAMILY)/adc.c \
        peripherals/$(CHIP_FAMILY)/cache.c

Contributing
============

Contributions are welcome! Please read our `Code of Conduct
<https://github.com/adafruit/samd-peripherals/blob/master/CODE_OF_CONDUCT.md>`_
before contributing to help this project stay welcoming.
