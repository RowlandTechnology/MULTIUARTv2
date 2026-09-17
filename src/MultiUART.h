/*****************
MultiUART Arduino Library
Created By: Ben Rowland
Copyright: Rowland Technology
Released under the MIT License, see LICENSE

Compatible with the MULTIUART or SPI2UART module.
Uses a single SPI bus to control up to four buffered hardware UART channels.

Each channel is exposed as a MultiUARTPort, which derives from the Arduino
Stream class. A port can therefore be used anywhere a Stream is accepted,
exactly like Serial or SoftwareSerial:

    MultiUART board(10);

    void setup() {
      board.begin();
      board.port(0).begin(9600);
      board.port(0).println("Hello");
      someLibrary.begin(board.port(1));   // any library taking Stream&
    }
*****************/

#ifndef MULTIUART_H
#define MULTIUART_H

#include <Arduino.h>
#include <SPI.h>

#define MULTIUART_NUM_PORTS         4

// Size of the local receive buffer held for each port on the Arduino.
// Data is pulled from the board in bursts of up to this many bytes.
#ifndef MULTIUART_RX_BUFFER_SIZE
#define MULTIUART_RX_BUFFER_SIZE    64
#endif
#if MULTIUART_RX_BUFFER_SIZE < 1 || MULTIUART_RX_BUFFER_SIZE > 255
#error "MULTIUART_RX_BUFFER_SIZE must be between 1 and 255"
#endif

// Default SPI clock. 250kHz matches SPI_CLOCK_DIV64 on a 16MHz AVR,
// which is what the original library examples used.
#ifndef MULTIUART_DEFAULT_SPI_SPEED
#define MULTIUART_DEFAULT_SPI_SPEED 250000UL
#endif

// Usable transmit queue size on the board: 999 for v2 firmware (the default),
// 511 for older v1 firmware.
#define MULTIUART_TX_QUEUE_V1       511
#define MULTIUART_TX_QUEUE_V2       999

// Inter-byte timing required by the board firmware
#ifndef MULTIUART_QUERY_DELAY_US
#define MULTIUART_QUERY_DELAY_US    250
#endif
#ifndef MULTIUART_BYTE_DELAY_US
#define MULTIUART_BYTE_DELAY_US     50
#endif

class MultiUART;

class MultiUARTPort : public Stream
{
  public:
	// Sets the baud rate and stores it on the board.
	// Supported rates: 1200, 2400, 4800, 9600, 19200, 31250, 38400, 57600, 62500, 115200.
	// Returns false if the rate is not supported.
	// Note: the board writes the rate to flash on every call, so avoid calling
	// this repeatedly. Use begin() with no argument to keep the stored rate.
	bool begin(unsigned long baud);

	// Uses the baud rate already stored on the board.
	void begin();

	void end();

	int available() override;
	int read() override;
	int peek() override;
	void flush() override;

	size_t write(uint8_t data) override;
	size_t write(const uint8_t *buffer, size_t size) override;
	using Print::write;

	int availableForWrite();

	operator bool() { return true; }

	uint8_t index() const { return _index; }

  private:
	friend class MultiUART;
	MultiUARTPort() {}

	void fillRxBuffer();

	MultiUART *_board = nullptr;
	uint8_t _index = 0;
	unsigned long _baud = 0;

	uint8_t _rxBuffer[MULTIUART_RX_BUFFER_SIZE];
	uint8_t _rxHead = 0;
	uint8_t _rxCount = 0;
	uint8_t _rxRemote = 0;		// bytes known to be waiting on the board at last check
	uint16_t _txFree = 0;		// conservative estimate of space in the board's tx queue
};


class MultiUART
{
  public:
	explicit MultiUART(uint8_t csPin, SPIClass &spi = SPI);

	// Starts the SPI bus. Call once in setup() before using any port.
	void begin(uint32_t spiSpeed = MULTIUART_DEFAULT_SPI_SPEED);

	MultiUARTPort &port(uint8_t index);
	MultiUARTPort &operator[](uint8_t index) { return port(index); }

	// Set to MULTIUART_TX_QUEUE_V1 for boards running the older v1 firmware.
	void setTxQueueSize(uint16_t size) { _txQueueSize = size; }
	uint16_t txQueueSize() const { return _txQueueSize; }

	// Maps a baud rate to the board's baud code, or -1 if unsupported.
	static int baudToCode(unsigned long baud);

	// Low level access. Counts are capped at 255 by the firmware.
	// Reading directly bypasses the port receive buffers, so do not mix these
	// with MultiUARTPort reads on the same channel.
	uint8_t rxCount(uint8_t channel);
	uint8_t txCount(uint8_t channel);
	uint16_t txFree(uint8_t channel);
	uint8_t readBytes(uint8_t channel, uint8_t *buffer, uint8_t count);
	uint8_t writeBytes(uint8_t channel, const uint8_t *buffer, uint8_t count);
	bool setBaud(uint8_t channel, unsigned long baud);
	bool setBaudCode(uint8_t channel, uint8_t code);

	// Original library API, kept so existing sketches continue to compile
	void initialise(int SPIDivider);
	char CheckRx(char UART);
	char CheckTx(char UART);
	char ReceiveByte(char UART);
	void ReceiveString(char *RETVAL, char UART, char NUMBYTES);
	void TransmitByte(char UART, char DATA);
	void TransmitString(char UART, const char *DATA, char NUMBYTES);
	void SetBaud(char UART, char BAUD);

  private:
	uint8_t query(uint8_t command);
	void select();
	void deselect();

	uint8_t _csPin;
	SPIClass *_spi;
	SPISettings _settings;
	uint16_t _txQueueSize = MULTIUART_TX_QUEUE_V2;
	MultiUARTPort _ports[MULTIUART_NUM_PORTS];
};

// Original class name
typedef MultiUART MULTIUART;

#endif
