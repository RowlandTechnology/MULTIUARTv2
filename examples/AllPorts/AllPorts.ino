/*
  MultiUART - All Ports

  Sends a message on all four UARTs every two seconds and forwards anything
  received on any UART to the USB serial port.

  Link a UART's TX to its own RX to see the messages loop back.
*/

#include <MultiUART.h>

MultiUART board(10);                      // chip select pin

void setup()
{
  Serial.begin(115200);
  board.begin();

  board.port(0).begin(9600);
  board.port(1).begin(9600);
  board.port(2).begin(115200);
  board.port(3).begin(115200);
}

void loop()
{
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= 2000)
  {
    lastSend = millis();
    for (uint8_t i = 0; i < MULTIUART_NUM_PORTS; i++)
    {
      board.port(i).print("UART ");
      board.port(i).print(i);
      board.port(i).println(" Test");
    }
  }

  for (uint8_t i = 0; i < MULTIUART_NUM_PORTS; i++)
  {
    MultiUARTPort &uart = board.port(i);
    if (uart.available())
    {
      Serial.print("UART ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(uart.readStringUntil('\n'));
    }
  }
}
