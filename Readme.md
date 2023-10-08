# use i2s microphone on esp32c6 with esp-idf

Just a quick test to see if i2s microphone works on esp32c6 with esp-idf.
I keep it simple, just as reference for myself, and anyone else who might need it.

# microphone
sen0526 from DFRobot, but any i2s mic with MSM261S4030H0R chip should work

# board
ESP32-C6-DevKitC-1-N8 from Espressif Systems. The board is relatively new, and not yet supported by arduino-esp32, so we use esp-idf 5.2 directly from git repo. 
On VSCode we use the Espressif IDF v1.6.4 extension.
Select the usbbodem port and esp32c6 as target board, as flash method select JTAG.
![Board Config](/doc/idfconfig.png)

This board has two usb type c ports, we use the one named "USB" on board for programming and console, and the other named "UART" on board for audio data.
So yes you have to connect two cables to the computer.

You sould select mith menuconfig the JTAG for console output, by default it is UART0, but we use it to stream audio data.
![Console Config](/doc/consoleOutput.png)

* at the time of this writing esp-idf have a bug that does not allow disable console for UART0, a siple patch is commented here: https://github.com/espressif/esp-idf/issues/10707#:~:text=KonstantinKondrashov%20commented%202%20days%20ago

# Listen to the microphone

I have tested on macos, but it should work on linux too.

Use the usb-c port named "UART" on the board, and connect it to the computer. 

- start a terminal and run:

`cat /dev/cu.usbserial-xxxx|ffmpeg -ar 44100 -acodec pcm_s24le -ac 1 -f s24le -i - -ac 1 -y test.mp3`

- on another terminal run:

`stty -f /dev/cu.usbserial-xxxx 921600 cs8 -parenb -cstopb`

* replace /dev/cu.usbserial-xxxx with the serial port of your board

# wiring
you can use the following table to connect the microphone to the board

```
#define MIC_STD_BCLK_IO2    GPIO_NUM_11    // I2S bit clock io number
#define MIC_STD_WS_IO2      GPIO_NUM_2     // I2S word select io number
#define MIC_STD_DIN_IO2     GPIO_NUM_10    // I2S data in io number
```

 if you want to use other pins change the defines in i2sMicToserial.c
