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

#define LASER_DDR  DDRD
#define LASER_PORT PORTD

#define DWN_PIN PD3 // Down Button connected to PORTD3
#define UP_PIN PD4 // Up Button connected to PORTD4
#define TRIGGER_PIN PD5 // Trigger Button connected to PORTD5

#define RST_PIN PD2  // Reset pin for HPS-166 connected to PORTD2

#define LASER_PIN  PD6 // Laser connected to PORTB1

void pin_init(void);

int get_button_state(void);

void laser_on(void);

void laser_off(void);

int laser_is_on(void);

#endif