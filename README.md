File List:

main.c - Most of the testing code and menuing will be in this file

HPS116.c - Library made for the ToF sensor, includes commands to send to sensor and functions 
for receiving data and telling the sensor to stop ranging in continuous mode (if used)

my_uart_lib.c - Library for sending and receiving data over UART channel, used by ToF sensor

buttonhandler.c - Library for receiving states of pressed buttons, and handling the laser

i2c.c and SSD1306.c were libraries from ECE231 that were used to display data onto the SSD1306
OLED screen (not being used in this project), these were used for testing and could be replaced
