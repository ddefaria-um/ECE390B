/* 
buttonhandler.h
Header file for button handler 
Damon DeFaria 4/9/2026
*/

#include <stdint.h>

#ifndef BUTTONHANDLER_H
#define BUTTONHANDLER_H

#define BUTTON_DDR DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PIN PIND

#define LASER_DDR  DDRB
#define LASER_PORT PORTB

#define DWN_PIN PD3 // Down Button connected to PORTD3
#define UP_PIN PD4 // Up Button connected to PORTD4
#define TRIGGER_PIN PD5 // Trigger Button connected to PORTD5

#define LASER_PIN  PB1 // Laser connected to PORTB1

void button_init(void);

int get_button_state(void);

void laser_init(void);

void laser_on(void);

void laser_off(void);

int laser_is_on(void);

#endif