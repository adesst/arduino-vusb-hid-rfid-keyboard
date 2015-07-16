# arduino-vusb-hid-rfid-keyboard
arduino vusb hid rfid keyboard

This project is to send RFID readUID to host computer as HID keyboard input.
The firmware currently must co-work with https://github.com/adesst/arduino-rfid-serial.
And the other requirement is https://github.com/adesst/arduino-vusb.

Drop the Library of arduino-usb to your arduino library folder.
Compile and flash https://github.com/adesst/arduino-rfid-serial into a chip, i use atmega328p.
And then compile and flash this project into another chip

You need 2 chips of atmega328p,
1st one for the USB communication. (with https://github.com/adesst/usbasploader-v2 as my bootloader)
2nd one for the SPI to MFCRC522 (https://github.com/miguelbalboa/rfid)

You wire chip1 Rx/Tx to chip2. The chips will communicate via serial.
Drop me a message if you have any question.
