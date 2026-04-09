#include "my_uart_lib.h"

// Initialize the UART
void uart_init(void){
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);   //enable the UART transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); //set 8 bit character size
    UBRR0L = 8;                           //set baud rate to 115200 for 16 MHz crystal
}

// Send a single character
void uart_send(unsigned char ch){
    while (!(UCSR0A & (1 << UDRE0)));       //wait til tx data buffer empty
    UDR0 = ch;                              //write the character to the USART data register
}

// Send a string of characters using uart_send
void send_string(const char *stringAddress){
    unsigned char i;
    for (i = 0; i < strlen(stringAddress); i++)
        uart_send((unsigned char)stringAddress[i]);
}

// Send a block of bytes using uart_send
void send_bytes(const uint8_t *data, uint8_t len)
{
    uint8_t i;
    for (i = 0; i < len; i++) {
        uart_send(data[i]);
    }
}

// Flush the UART receive buffer by reading and discarding any received data
void uart_flush_rx(void)
{
    uint8_t dummy;
    while (UCSR0A & (1 << RXC0)) dummy = UDR0;
    (void)dummy;
}

bool uart_getc_timeout(uint8_t *out)
{
    /* Each loop iteration ≈ 1 ms at 16 MHz (rough, without timer)       */
    uint32_t count = (uint32_t)UART_TIMEOUT_MS * (F_CPU / 10000UL);
    while (count--) {
        if (UCSR0A & (1 << RXC0)) {
            *out = UDR0;
            return true;
        }
    }
    return false;
}