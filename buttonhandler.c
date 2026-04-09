/*
buttonhandler.c
Code for getting button states and handling laser
Damon DeFaria 4/9/2026 */

#include <avr/io.h>
#include "buttonhandler.h"

// Initialize buttons, reset pin for sensor, and laser pin
void pin_init(void)
{
    // Trigger
    BUTTON_DDR &= ~(1 << TRIGGER_PIN);
    BUTTON_PORT |= (1 << TRIGGER_PIN);
    
    // Up
    BUTTON_DDR &= ~(1 << UP_PIN);
    BUTTON_PORT |= (1 << UP_PIN);
    
    // Down
    BUTTON_DDR &= ~(1 << DWN_PIN);
    BUTTON_PORT |= (1 << DWN_PIN);

    // Reset pin for HPS-166, PD5
    DDRD |= (1 << RST_PIN);
    PORTD |= (1 << RST_PIN);

    // Laser, starts off
    LASER_DDR |= (1 << LASER_PIN);
    LASER_PORT &= ~(1 << LASER_PIN);
}

/*
Return an integer based on button pressed
Trigger = 1, Up = 2, Down = 3, Multiple/None = 0
*/
int get_button_state(void)
{
    int state;
    
    // Multiple buttons pressed
    if (((!(BUTTON_PORT & (1 << TRIGGER_PIN))) && !(BUTTON_PORT & (1 << UP_PIN))) 
        || (!(BUTTON_PORT & (1 << TRIGGER_PIN)) && !(BUTTON_PORT & (1 << DWN_PIN))) 
        || (!(BUTTON_PORT & (1 << UP_PIN)) && !(BUTTON_PORT & (1 << DWN_PIN))))
    {
        state = 0;
    }
    
    else if (!(BUTTON_PORT & (1 << TRIGGER_PIN))) 
    {
        state = 1;
    } 
    else if (!(BUTTON_PORT & (1 << UP_PIN))) 
    {
        state = 2;
    } 
    else if (!(BUTTON_PORT & (1 << DWN_PIN))) 
    {
        state = 3;
    } 
    else 
    {
        state = 0;
    }
    return state;
}

void laser_on(void)
{
    LASER_PORT |= (1 << LASER_PIN);
}

void laser_off(void)
{
    LASER_PORT &= ~(1 << LASER_PIN);
}

// Possibly useful for debugging, returns 1 if on, 0 if off
int laser_is_on(void)
{
    return (LASER_PORT & (1 << LASER_PIN)) != 0;
}