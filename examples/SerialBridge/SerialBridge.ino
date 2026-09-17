/*
  MultiUART - Serial Bridge

  Forwards data between the USB serial port and UART 0 of the MULTIUART board,
  so you can talk to a device connected to the board from the Serial Monitor.

  Wiring: MULTIUART SPI to the Arduino SPI pins, CS to pin 10.
*/

#include <MultiUART.h>

MultiUART board(10);                      // chip select pin
MultiUARTPort &uart0 = board.port(0);

void setup()
{
  Serial.begin(115200);
  board.begin();

  // Sets and stores the baud rate on the board. The board writes this to flash,
  // so once set you can use uart0.begin() without a rate instead.
  uart0.begin(9600);
}

void loop()
{
  while (Serial.available())
    uart0.write(Serial.read());

  while (uart0.available())
    Serial.write(uart0.read());
}
