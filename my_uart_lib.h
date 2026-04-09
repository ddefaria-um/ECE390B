/***************************************
 * my_uart_lib.h 
 * header file for my_uart_lib.c
 * user-contributed library for initializing UART
 * and transmitting characters and strings on ATmega328P MCU
 * Version Author           Date        Comment
 * 1.0      D. McLaughlin   4/16/24     
 * **************************************/


#ifndef MY_UART_LIB_H
#define MY_UART_LIB_H

#include <avr/io.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define UART_TIMEOUT_MS 500
#define F_CPU 16000000UL

/* Initialize the UART: Enables the UART transmitter; 
* Sets 8 bit character size
* Sets baud rate to 115200 for 16 MHz crystal
* Arguments: none
* Returns: none */
void uart_init(void);

/* Transmit a single character via UART
* Arguments: 
*       letter - ASCII character to be transmitted
* Returns: none */
void uart_send(unsigned char letter);

/* Transmit a character string via UART.
* Sends the string, char by char, to the UART
* via uart_send()
* Arguments: 
*       *stringAddress - pointer to the string
* Returns: none */
void send_string(const char *stringAddress);

/* Send a block of bytes over UART */
void send_bytes(const uint8_t *data, uint8_t len);

void uart_flush_rx(void);

bool uart_getc_timeout(uint8_t *out);

#endif