#include "serial.h"
#include "io.h"

void serial_init(void) {
	io_out8(0x03fb, 0x80); /* setting MSB/LSB */
	io_out8(0x03f9, 0x00); /* MSB (115200bps) */
	io_out8(0x03f8, 0x01); /* LSB (115200bps) */
	io_out8(0x03fb, 0x03); /* no pality, 1-bit stop, 8-bit data */
	io_out8(0x03fc, 0x03); /* no interrupt, RTS=1, DTR=1 */
	io_out8(0x03f9, 0x00); /* no interrupt */
	io_out8(0x03fa, 0x01); /* use FIFO */
}

int serial_read(void) {
	while (!(io_in8(0x03fd) & 0x01)); /* wait until receive data */
	return io_in8(0x03f8);
}

void serial_write(int c) {
	while (!(io_in8(0x03fd) & 0x20)); /* wait until TX buffer available */
	io_out8(0x03f8, c);
}
