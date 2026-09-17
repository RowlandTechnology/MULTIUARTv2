# MultiUART Arduino Library

Arduino library for the Rowland Technology **MULTIUART / SPI2UART** board, which adds four buffered hardware UARTs to any board with an SPI bus.

Each UART channel is a standard Arduino `Stream`, the same base class as `Serial` and `SoftwareSerial`. That means you can use `print`, `println`, `readStringUntil`, `parseInt` and so on, and pass a port to any library that accepts a `Stream`.

## Quick start

```cpp
#include <MultiUART.h>

MultiUART board(10);                     // SPI chip select pin

void setup() {
  board.begin();                         // start SPI
  board.port(0).begin(9600);             // set UART 0 baud rate
  board.port(0).println("Hello from UART 0");
}

void loop() {
  if (board.port(0).available()) {
    Serial.write(board.port(0).read());
  }
}
```

A port can be kept as a reference so it reads like any other serial port:

```cpp
MultiUARTPort &gpsSerial = board.port(2);
```

## Using ports with other libraries

Any library that takes a `Stream &` or `Stream *` works with a port:

```cpp
modbus.begin(1, board.port(0));          // ModbusMaster
mp3.begin(board.port(1));                // DFRobotDFPlayerMini
```

Libraries that require a `HardwareSerial` object specifically cannot accept a MultiUART port. Many of these also provide a `Stream` overload.

## Wiring

| MULTIUART | Arduino |
|-----------|---------|
| MOSI      | SPI MOSI (Uno: 11) |
| MISO      | SPI MISO (Uno: 12) |
| SCK       | SPI SCK (Uno: 13) |
| CS        | Any digital pin (examples use 10) |
| GND       | GND |

The board uses SPI mode 0, MSB first. The default SPI clock is 250kHz. It can be changed with `board.begin(speed)`.

Several boards can share one SPI bus by giving each its own chip select pin.

## Baud rates

Supported rates: 1200, 2400, 4800, 9600, 19200, 31250, 38400, 57600, 62500 and 115200. Data format is 8N1.

`port.begin(baud)` returns `false` if the rate is not supported.

**The board stores the baud rate in flash every time it is set.** The rate is remembered across power cycles, so once a channel is configured you can call `port.begin()` with no rate to leave the stored setting alone and avoid unnecessary flash writes.

## Firmware versions

The library is set up for v2 firmware, which queues up to 999 bytes per UART for transmit. Boards still running the original v1 firmware only queue 511 bytes, so tell the library:

```cpp
board.setTxQueueSize(MULTIUART_TX_QUEUE_V1);
```

## API

### `MultiUART`

| Function | Description |
|----------|-------------|
| `MultiUART(csPin, spi = SPI)` | Create a board on the given chip select pin and SPI bus |
| `begin(spiSpeed = 250000)` | Start the SPI bus |
| `port(index)` / `board[index]` | Get the `MultiUARTPort` for channel 0-3 |
| `setTxQueueSize(size)` | Set to `MULTIUART_TX_QUEUE_V1` for boards with v1 firmware |

### `MultiUARTPort`

All `Stream` and `Print` functions, plus:

| Function | Description |
|----------|-------------|
| `begin(baud)` | Set and store the baud rate, returns `false` if unsupported |
| `begin()` | Use the baud rate already stored on the board |
| `available()` | Bytes waiting to be read |
| `read()` / `peek()` | Read a byte, `-1` if none |
| `write(...)` | Send data, waits for space in the board's queue up to the stream timeout |
| `availableForWrite()` | Free space in the board's transmit queue |
| `flush()` | Wait until the board has sent everything queued |

### Low level

`rxCount`, `txCount`, `txFree`, `readBytes`, `writeBytes`, `setBaud` and `setBaudCode` on `MultiUART` talk to the board directly. Do not mix these with port reads on the same channel, as ports keep a local receive buffer.

### Original API

Sketches written for the original `MULTIUART` library still compile after changing the include to `#include <MultiUART.h>`. `initialise`, `CheckRx`, `CheckTx`, `ReceiveByte`, `ReceiveString`, `TransmitByte`, `TransmitString` and `SetBaud` are all still available. See the `LegacyDemo` example.

## Performance notes

Every check of the board is an SPI transaction taking roughly half a millisecond. Received data is pulled across in blocks of up to 64 bytes into a local buffer, and writes are sent as blocks, so `print` and `readBytes` are much more efficient than looping over single bytes. To change the local buffer size, edit `MULTIUART_RX_BUFFER_SIZE` in `MultiUART.h` (maximum 255).

## Examples

- **SerialBridge**: talk to a device on UART 0 from the Serial Monitor
- **AllPorts**: send and receive on all four UARTs
- **StreamParser**: pass a port to code that takes a generic `Stream`
- **LegacyDemo**: the original library example using the old API

## Flowcode

A Flowcode component and example program for the board are in [extras/flowcode](extras/flowcode).

## License

MIT License, see [LICENSE](LICENSE).
