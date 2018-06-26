#include "serial.h"
#include "io.h"
#include "interrupts.h"

#define TX_BUFFER_SIZE 256
#define RX_BUFFER_SIZE 256

unsigned char tx_buffer[TX_BUFFER_SIZE], rx_buffer[RX_BUFFER_SIZE];
volatile int tx_start, tx_end, rx_start, rx_end;

void serial_interrupt_handler(int ecode) {
	int status = io_in8(0x03fd);
	if (status & 0x01) { /* have data to read */
		int c = io_in8(0x03f8);
		int rx_next = (rx_end + 1) % RX_BUFFER_SIZE;
		if (rx_next != rx_start) {
			rx_buffer[rx_end] = c;
			rx_end = rx_next;
		}
	}
	if (tx_start != tx_end && (status & 0x20)) { /* have data to send && TX buffer available */
		io_out8(0x03f8, tx_buffer[tx_start]);
		tx_start = (tx_start+ 1) % TX_BUFFER_SIZE;
		if (tx_start == tx_end) {
			io_out8(0x03f9, 0x01); /* disable TX interrupt */
		}
	}
	(void)ecode;
}

void serial_init(void) {
	tx_start = tx_end =rx_start = rx_end = 0;
	set_interrupt_handler(0x24, serial_interrupt_handler);
	set_irq_enable(get_irq_enabled() | 0x10);

	io_out8(0x03fb, 0x80); /* setting MSB/LSB */
	io_out8(0x03f9, 0x00); /* MSB (115200bps) */
	io_out8(0x03f8, 0x01); /* LSB (115200bps) */
	io_out8(0x03fb, 0x03); /* no pality, 1-bit stop, 8-bit data */
	io_out8(0x03fa, 0x01); /* use FIFO */
	io_out8(0x03f9, 0x01); /* enable Rx interrupt */
	io_out8(0x03fc, 0x0b); /* enable interrupt, RTS=1, DTR=1 */
}

int serial_read(void) {
	int ret;
	/* wait until receive data */
	while (rx_start == rx_end);
	ret = rx_buffer[rx_start];
	rx_start = (rx_start + 1) % RX_BUFFER_SIZE;
	return ret;
}

void serial_write(int c) {
	int enable_interrupt;
	int tx_next;
	/* wait until TX buffer available */
	do {
		tx_next = (tx_end + 1) % TX_BUFFER_SIZE;
	} while (tx_next == tx_start);
	enable_interrupt = (tx_start == tx_end);
	tx_buffer[tx_end] = c;
	tx_end = tx_next;
	if (enable_interrupt) {
		io_out8(0x03f9, 0x03); /* enable TX interrupt */
	}
}
