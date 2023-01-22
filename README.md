This is the firmware and PCB design for a low-latency USB controller for arcade controls - buttons, joysticks (digital and analog), and mouse-like things (spinners, trackballs).

It's based around the Raspberry Pi Pico board which provides all of the GPIO and USB functionality.

TinyUSB is used (from the pico SDK) to handle the USB communication.

The quadrature encoder is implemented using this code:
https://github.com/jamon/pi-pico-pio-quadrature-encoder

To build:

1) git clone https://github.com/garyjsweet/arcade_ctrl.git --recurse-submodules

2) Make an out-of-tree build folder and configure cmake
   Use "<PATH_TO_THIS_REPO>/pico-sdk/cmake/preload/toolchains/pico_arm_gcc.cmake" as the toolchain file.

3) Ensure you have an appropriate arm-none-eabi-gcc compiler toolchain installed for the pico.

4) make

5) Connect the pico board to your PC whilst holding down the "bootsel" button on the pico board.

6) Drag and drop the ArcadeCtrl.uf2 from the build folder onto the pico
   Note: The led on the pico should now be blinking roughly once per second.

7) Move to USB to your target host device and enjoy.