/*
  MultiUART - Stream Parser

  Shows that a MultiUART port can be passed to any code that takes a Stream,
  the same as Serial or SoftwareSerial. Most device libraries (Modbus, GPS,
  MP3 players, fingerprint sensors, AT modems) work the same way.

  Send lines such as "LED 1" or "ADD 5 7" to UART 1 and replies are sent back.
*/

#include <MultiUART.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

MultiUART board(10);                      // chip select pin

// Works with any Stream: Serial, SoftwareSerial or a MultiUART port
void handleCommands(Stream &stream)
{
  if (!stream.available())
    return;

  String command = stream.readStringUntil(' ');
  command.trim();

  if (command == "LED")
  {
    int state = stream.parseInt();
    digitalWrite(LED_BUILTIN, state ? HIGH : LOW);
    stream.print("LED is ");
    stream.println(state ? "on" : "off");
  }
  else if (command == "ADD")
  {
    long a = stream.parseInt();
    long b = stream.parseInt();
    stream.print("Result: ");
    stream.println(a + b);
  }
  else if (command.length())
  {
    stream.print("Unknown command: ");
    stream.println(command);
  }

  while (stream.available() && stream.peek() != '\n')
    stream.read();
  if (stream.peek() == '\n')
    stream.read();
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  board.begin();
  board.port(1).begin(9600);
}

void loop()
{
  handleCommands(Serial);           // commands from the Serial Monitor
  handleCommands(board.port(1));    // commands from UART 1 on the board
}
