/*****************
MultiUART Arduino Library
Created By: Ben Rowland
Copyright: Rowland Technology
Released under the MIT License, see LICENSE

Compatible with the MULTIUART or SPI2UART module.
Uses a single SPI bus to control up to four buffered hardware UART channels.
*****************/

#include "MultiUART.h"

// SPI commands, lower two bits select the channel
#define CMD_CHECK_RX	0x10
#define CMD_GET_RX		0x20
#define CMD_CHECK_TX	0x30
#define CMD_PUT_TX		0x40
#define CMD_SET_BAUD	0x80

#define BAUD_FLASH_WRITE_MS	20		// time for the board to erase and write flash

static const unsigned long baudRates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 31250, 62500};


/*=----------------------------------------------------------------------=*\
   MultiUART - board
\*=----------------------------------------------------------------------=*/

MultiUART::MultiUART(uint8_t csPin, SPIClass &spi)
	: _csPin(csPin), _spi(&spi), _settings(MULTIUART_DEFAULT_SPI_SPEED, MSBFIRST, SPI_MODE0)
{
	for (uint8_t i = 0; i < MULTIUART_NUM_PORTS; i++)
	{
		_ports[i]._board = this;
		_ports[i]._index = i;
	}
}


void MultiUART::begin(uint32_t spiSpeed)
{
	pinMode(_csPin, OUTPUT);
	digitalWrite(_csPin, HIGH);
	_settings = SPISettings(spiSpeed, MSBFIRST, SPI_MODE0);
	_spi->begin();
}


MultiUARTPort &MultiUART::port(uint8_t index)
{
	if (index >= MULTIUART_NUM_PORTS)
		index = MULTIUART_NUM_PORTS - 1;
	return _ports[index];
}


int MultiUART::baudToCode(unsigned long baud)
{
	for (uint8_t i = 0; i < sizeof(baudRates) / sizeof(baudRates[0]); i++)
	{
		if (baudRates[i] == baud)
			return i;
	}
	return -1;
}


void MultiUART::select()
{
	_spi->beginTransaction(_settings);
	digitalWrite(_csPin, LOW);
}


void MultiUART::deselect()
{
	digitalWrite(_csPin, HIGH);
	_spi->endTransaction();
	delayMicroseconds(MULTIUART_BYTE_DELAY_US);
}


uint8_t MultiUART::query(uint8_t command)
{
	select();
	_spi->transfer(command);
	delayMicroseconds(MULTIUART_QUERY_DELAY_US);
	uint8_t result = _spi->transfer(0xFF);
	deselect();
	return result;
}


/*=----------------------------------------------------------------------=*\
   Use :Returns the number of received bytes waiting on the board (max 255).
\*=----------------------------------------------------------------------=*/
uint8_t MultiUART::rxCount(uint8_t channel)
{
	if (channel >= MULTIUART_NUM_PORTS)
		return 0;
	return query(CMD_CHECK_RX | channel);
}


/*=----------------------------------------------------------------------=*\
   Use :Returns the number of bytes waiting to be sent by the board (max 255).
\*=----------------------------------------------------------------------=*/
uint8_t MultiUART::txCount(uint8_t channel)
{
	if (channel >= MULTIUART_NUM_PORTS)
		return 0;
	return query(CMD_CHECK_TX | channel);
}


/*=----------------------------------------------------------------------=*\
   Use :Returns the guaranteed free space in the board's transmit queue.
       :As the firmware caps the count at 255 a full count is treated as 255 queued.
\*=----------------------------------------------------------------------=*/
uint16_t MultiUART::txFree(uint8_t channel)
{
	if (channel >= MULTIUART_NUM_PORTS)
		return 0;
	uint8_t queued = txCount(channel);
	if (queued >= _txQueueSize)
		return 0;
	return _txQueueSize - queued;
}


/*=----------------------------------------------------------------------=*\
   Use :Reads bytes from the board's receive queue.
       :count must not exceed the value returned by rxCount, otherwise the
       :firmware pads with zeros.
       :Returns the number of bytes read.
\*=----------------------------------------------------------------------=*/
uint8_t MultiUART::readBytes(uint8_t channel, uint8_t *buffer, uint8_t count)
{
	// A zero length request would leave the firmware state machine waiting
	if (channel >= MULTIUART_NUM_PORTS || count == 0)
		return 0;

	select();
	_spi->transfer(CMD_GET_RX | channel);
	delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	_spi->transfer(count);
	delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	for (uint8_t i = 0; i < count; i++)
	{
		buffer[i] = _spi->transfer(0xFF);
		delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	}
	deselect();
	return count;
}


/*=----------------------------------------------------------------------=*\
   Use :Adds bytes to the board's transmit queue.
       :Bytes that do not fit in the queue are discarded by the firmware,
       :check txFree first.
       :Returns the number of bytes sent.
\*=----------------------------------------------------------------------=*/
uint8_t MultiUART::writeBytes(uint8_t channel, const uint8_t *buffer, uint8_t count)
{
	// A zero length request would leave the firmware state machine waiting
	if (channel >= MULTIUART_NUM_PORTS || count == 0)
		return 0;

	select();
	_spi->transfer(CMD_PUT_TX | channel);
	delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	_spi->transfer(count);
	delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	for (uint8_t i = 0; i < count; i++)
	{
		_spi->transfer(buffer[i]);
		delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	}
	deselect();
	return count;
}


/*=----------------------------------------------------------------------=*\
   Use :Configures the baud rate of the selected channel.
       :Code: 0=1200, 1=2400, 2=4800, 3=9600, 4=19200, 5=38400, 6=57600,
       :      7=115200, 8=31250, 9=62500
\*=----------------------------------------------------------------------=*/
bool MultiUART::setBaudCode(uint8_t channel, uint8_t code)
{
	if (channel >= MULTIUART_NUM_PORTS || code >= sizeof(baudRates) / sizeof(baudRates[0]))
		return false;

	select();
	_spi->transfer(CMD_SET_BAUD | channel);
	delayMicroseconds(MULTIUART_BYTE_DELAY_US);
	_spi->transfer(code);
	deselect();

	delay(BAUD_FLASH_WRITE_MS);
	return true;
}


bool MultiUART::setBaud(uint8_t channel, unsigned long baud)
{
	int code = baudToCode(baud);
	if (code < 0)
		return false;
	return setBaudCode(channel, code);
}


/*=----------------------------------------------------------------------=*\
   Original library API
\*=----------------------------------------------------------------------=*/

void MultiUART::initialise(int SPIDivider)
{
	uint32_t speed = MULTIUART_DEFAULT_SPI_SPEED;

#if defined(ARDUINO_ARCH_AVR)
	switch (SPIDivider)
	{
		case SPI_CLOCK_DIV2:   speed = F_CPU / 2;   break;
		case SPI_CLOCK_DIV4:   speed = F_CPU / 4;   break;
		case SPI_CLOCK_DIV8:   speed = F_CPU / 8;   break;
		case SPI_CLOCK_DIV16:  speed = F_CPU / 16;  break;
		case SPI_CLOCK_DIV32:  speed = F_CPU / 32;  break;
		case SPI_CLOCK_DIV64:  speed = F_CPU / 64;  break;
		case SPI_CLOCK_DIV128: speed = F_CPU / 128; break;
	}
#else
	(void)SPIDivider;
#endif

	begin(speed);
}


char MultiUART::CheckRx(char UART)
{
	return rxCount(UART);
}


char MultiUART::CheckTx(char UART)
{
	return txCount(UART);
}


char MultiUART::ReceiveByte(char UART)
{
	uint8_t data = 0;
	readBytes(UART, &data, 1);
	return data;
}


void MultiUART::ReceiveString(char *RETVAL, char UART, char NUMBYTES)
{
	uint8_t count = readBytes(UART, (uint8_t *)RETVAL, (uint8_t)NUMBYTES);
	RETVAL[count] = 0;
}


void MultiUART::TransmitByte(char UART, char DATA)
{
	uint8_t data = DATA;
	writeBytes(UART, &data, 1);
}


void MultiUART::TransmitString(char UART, const char *DATA, char NUMBYTES)
{
	writeBytes(UART, (const uint8_t *)DATA, (uint8_t)NUMBYTES);
}


void MultiUART::SetBaud(char UART, char BAUD)
{
	setBaudCode(UART, BAUD);
}


/*=----------------------------------------------------------------------=*\
   MultiUARTPort - Stream interface for a single channel
\*=----------------------------------------------------------------------=*/

bool MultiUARTPort::begin(unsigned long baud)
{
	if (!_board->setBaud(_index, baud))
		return false;
	_baud = baud;
	begin();
	return true;
}


void MultiUARTPort::begin()
{
	_rxHead = 0;
	_rxCount = 0;
	_rxRemote = 0;
	_txFree = 0;
}


void MultiUARTPort::end()
{
	flush();
}


/*=----------------------------------------------------------------------=*\
   Use :Pulls waiting bytes from the board into the local receive buffer.
\*=----------------------------------------------------------------------=*/
void MultiUARTPort::fillRxBuffer()
{
	uint8_t space = MULTIUART_RX_BUFFER_SIZE - _rxCount;
	if (space == 0)
		return;

	_rxRemote = _board->rxCount(_index);
	if (_rxRemote == 0)
		return;

	uint8_t count = (_rxRemote < space) ? _rxRemote : space;
	uint8_t data[MULTIUART_RX_BUFFER_SIZE];
	_board->readBytes(_index, data, count);
	_rxRemote -= count;

	for (uint8_t i = 0; i < count; i++)
	{
		_rxBuffer[(_rxHead + _rxCount) % MULTIUART_RX_BUFFER_SIZE] = data[i];
		_rxCount++;
	}
}


int MultiUARTPort::available()
{
	fillRxBuffer();
	return (int)_rxCount + _rxRemote;
}


int MultiUARTPort::read()
{
	if (_rxCount == 0)
		fillRxBuffer();
	if (_rxCount == 0)
		return -1;

	uint8_t data = _rxBuffer[_rxHead];
	_rxHead = (_rxHead + 1) % MULTIUART_RX_BUFFER_SIZE;
	_rxCount--;
	return data;
}


int MultiUARTPort::peek()
{
	if (_rxCount == 0)
		fillRxBuffer();
	if (_rxCount == 0)
		return -1;

	return _rxBuffer[_rxHead];
}


size_t MultiUARTPort::write(uint8_t data)
{
	return write(&data, 1);
}


/*=----------------------------------------------------------------------=*\
   Use :Sends data to the board, waiting for space in its transmit queue.
       :Gives up if no space becomes available within the Stream timeout
       :(setTimeout, default 1000ms) and returns the number of bytes sent.
\*=----------------------------------------------------------------------=*/
size_t MultiUARTPort::write(const uint8_t *buffer, size_t size)
{
	size_t written = 0;
	unsigned long lastProgress = millis();

	while (written < size)
	{
		if (_txFree == 0)
		{
			_txFree = _board->txFree(_index);
			if (_txFree == 0)
			{
				if (millis() - lastProgress >= _timeout)
					break;
				delay(1);
				continue;
			}
		}

		size_t count = size - written;
		if (count > _txFree)
			count = _txFree;
		if (count > 255)
			count = 255;

		_board->writeBytes(_index, buffer + written, count);
		written += count;
		_txFree -= count;
		lastProgress = millis();
	}

	return written;
}


int MultiUARTPort::availableForWrite()
{
	_txFree = _board->txFree(_index);
	return _txFree;
}


/*=----------------------------------------------------------------------=*\
   Use :Waits for the board to finish sending its transmit queue.
\*=----------------------------------------------------------------------=*/
void MultiUARTPort::flush()
{
	// The reported count is capped at 255, so at slow baud rates it can appear
	// stuck while the rest of the queue drains. Allow time for a full queue.
	unsigned long timeout = _timeout + (_baud ? (10000UL * _board->txQueueSize()) / _baud : 10000UL);
	uint8_t lastCount = 255;
	unsigned long lastProgress = millis();

	while (true)
	{
		uint8_t count = _board->txCount(_index);
		if (count == 0)
			break;

		if (count < lastCount)
			lastProgress = millis();
		else if (millis() - lastProgress >= timeout)
			return;

		lastCount = count;
		delay(1);
	}

	// Allow the final character to leave the UART shift register
	if (_baud)
		delay(10000UL / _baud + 1);
}
