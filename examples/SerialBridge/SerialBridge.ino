/*
  MultiUART - Serial Bridge

  Forwards data between the USB serial port and UART 0 of the MULTIUART board,
  so you can talk to a device connected to the board from the Serial Monitor.

  Wiring: MULTIUART SPI to the Arduino SPI pins, CS to pin 10.
*/

#include <MultiUART.h>

MultiUART board(10);                      // chip select pin
MultiUARTPort &deviceSerial = board.port(0);

void setup()
{
  Serial.begin(115200);
  board.begin();

  // Sets and stores the baud rate on the board. The board writes this to flash,
  // so once set you can use deviceSerial.begin() without a rate instead.
  deviceSerial.begin(9600);
}

void loop()
{
  while (Serial.available())
    deviceSerial.write(Serial.read());

  while (deviceSerial.available())
    Serial.write(deviceSerial.read());
}
