/*
  MultiUART - Legacy Demo

  The original MULTIUART library example. Existing sketches using the old
  function names still compile with this library. New sketches should use
  the Stream based ports shown in the other examples.
*/

#include <MultiUART.h>

MULTIUART multiuart(10);        // CS pin = D10

char LENGTH;
char Str1[256];

void setup()
{
  multiuart.initialise(SPI_CLOCK_DIV64);
  Serial.begin(9600);

  // 0=1200, 1=2400, 2=4800, 3=9600, 4=19200, 5=38400, 6=57600, 7=115200
  multiuart.SetBaud(0, 3);
  multiuart.SetBaud(1, 3);
  multiuart.SetBaud(2, 7);
  multiuart.SetBaud(3, 7);
}

void loop()
{
  delay(2000);

  multiuart.TransmitString(0, "UART 0 Test", 11);
  multiuart.TransmitString(1, "UART 1 Test", 11);
  multiuart.TransmitString(2, "UART 2 Test", 11);
  multiuart.TransmitString(3, "UART 3 Test", 11);

  delay(2000);

  for (char i = 0; i < 4; i++)
  {
    Serial.print("UART ");
    Serial.print((int)i);
    Serial.print(": ");
    LENGTH = multiuart.CheckRx(i);
    if (LENGTH != 0)
    {
      multiuart.ReceiveString(Str1, i, LENGTH);
      Serial.println(Str1);
    }
    else
      Serial.println();
  }
}
