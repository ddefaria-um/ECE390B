/* Main code for 390B Mapping Device Project
v0.1 3/30/2026 - Initial version
v0.2 3/31/2026 - Added and finalized UART and HPS-166 libraries
v0.3 4/1/2026 - Added sensor reset and button trigger
v0.4 4/6/2026 - Changed to Single Range, added error handling for testing
v0.5 4/8/2026 - Added SSD1306 OLED code for testing, works!!
v0.6 4/9/2026 - Created buttonhandler library for buttons and laser, cleaned up main.c
v0.7 4/10/2026 - Mapped out menu states
v0.8 4/15/2026 - Measuring code while in correct menu states, button debouncing, and OLED code for some of menu states, ready for testing
Damon DeFaria 34305913  */

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "i2c.h"
#include "my_uart_lib.h"
#include "HPS166.h"
#include "SSD1306.h"
#include "buttonhandler.h"


void sensor_reset(void)
{
    DDRD  |=  (1 << RST_PIN);    // RST as output
    PORTD &= ~(1 << RST_PIN);    // Assert reset (active low)
    _delay_ms(10);
    PORTD |=  (1 << RST_PIN);    // Release reset
    _delay_ms(500);              // Wait for "Hypersen" init string
    uart_flush_rx();             // Discard the init string
}


int main(void)
{
    int state = 0;
    int roomid = 0;
    int trigstate = 0;

    pin_init();
    uart_init();
    OLED_Init();
    
    OLED_Clear();
    OLED_GoToLine(4);
    OLED_DisplayString("Startup...");

    _delay_ms(1000);

    uint16_t latest_dist_mm = 0;
    uint16_t 1st_saved_dist_mm = 0;
    uint16_t 2nd_saved_dist_mm = 0;
    char buffer[16];
    
    while (1)
    {
        // Main Menu depending on button pressed
        OLED_Clear();
        if (state == 0)
        {
            OLED_GoToLine(2);
            OLED_DisplayString(">Measure");
            OLED_GoToLine(4);
            OLED_DisplayString("View Rooms");
            OLED_GoToLine(6);
            OLED_DisplayString("Options");
            if (get_button_state() == 1)
            {
                state = 3;
                laser_on();
            }
            else if (get_button_state() == 3)
            {
                state = 1;
            }

        }
        else if (state == 1)
        {
            OLED_GoToLine(2);
            OLED_DisplayString("Measure");
            OLED_GoToLine(4);
            OLED_DisplayString(">View Rooms");
            OLED_GoToLine(6);
            OLED_DisplayString("Options");
            if (get_button_state() == 1)
            {
                state = 20;
            }
            else if (get_button_state() == 2)
            {
                state = 0;
            }
            else if (get_button_state() == 3)
            {
                state = 2;
            }
        }
        else if (state == 2)
        {
            OLED_GoToLine(2);
            OLED_DisplayString("Measure");
            OLED_GoToLine(4);
            OLED_DisplayString("View Rooms");
            OLED_GoToLine(6);
            OLED_DisplayString(">Options");
            if (get_button_state() == 1)
            {
                state = 25;
            }
            else if (get_button_state() == 2)
            {
                state = 1;
            }
        }
        // First measurement
        else if (state == 3)
        {
            OLED_GoToLine(2);
            OLED_DisplayString("Hold and Release Trigger");
            OLED_GoToLine(4);
            OLED_DisplayString("Press Dwn to Cancel");
            // Pressing Dwn brings back to main menu
            while (trigstate == 1)
            {
                if (!(BUTTON_PORT & (1 << TRIGGER_PIN)))
                {
                    trigstate = 0;
                    state = 4;
                    send_bytes(CMD_SINGLE_RANGE, 10);
                    if (receive_ranging_frame(&latest_dist_mm))
                    {
                        1st_saved_dist_mm = latest_dist_mm;
                    }
                    else
                    {
                        // Handle error
                        1st_saved_dist_mm = 9999;
                    }
                    laser_off();
                }
            }
            
            if (get_button_state() == 3)
            {
                state = 0;
                laser_off();
            }
            else if (get_button_state() == 1)
            {
                trigstate = 1;
            }

        }
        // Lock in measurement
        else if (state == 4)
        {
            
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)saved_dist_mm);
            OLED_GoToLine(0);
            OLED_DisplayString("Dist: ");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            OLED_DisplayString(">Confirm");
            OLED_GoToLine(6);
            OLED_DisplayString("Retake");
            if (get_button_state() == 1)
            {
                state = 6;
            }
            else if (get_button_state() == 3)
            {
                state = 5;
            }
        }
        else if (state == 5)
        {
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)1st_saved_dist_mm);
            OLED_GoToLine(0);
            OLED_DisplayString("Dist: ");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            OLED_DisplayString("Confirm");
            OLED_GoToLine(6);
            OLED_DisplayString(">Retake");
            if (get_button_state() == 1)
            {
                1st_saved_dist_mm = 0;
                state = 3;
                laser_on();
            }
            else if (get_button_state() == 2)
            {
                state = 4;
            }
        }
        // Confirm 1st measurement, ask for 2nd
        else if (state == 6)
        {
            OLED_GoToLine(4);
            OLED_DisplayString(">Take 2nd Measure.");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");
            if (get_button_state() == 1)
            {
                state = 8;
                laser_on();
            }
            else if (get_button_state() == 3)
            {
                state = 7;
            }
        }
        else if (state == 7)
        {
            OLED_GoToLine(4);
            OLED_DisplayString("Take 2nd Measure.");
            OLED_GoToLine(6);
            OLED_DisplayString(">Cancel");
            if (get_button_state() == 1)
            {
                state = 0;
                1st_saved_dist_mm = 0;
            }
            else if (get_button_state() == 2)
            {
                state = 6;
            }
        }
        // Take 2nd measurement
        else if (state == 8)
        {
            OLED_GoToLine(2);
            OLED_DisplayString("Hold and Release Trigger");
            OLED_GoToLine(4);
            OLED_DisplayString("Press Dwn to Cancel");
            while (trigstate == 1)
            {
                if (!(BUTTON_PORT & (1 << TRIGGER_PIN)))
                {
                    trigstate = 0;
                    state = 9;
                    send_bytes(CMD_SINGLE_RANGE, 10);
                    if (receive_ranging_frame(&latest_dist_mm))
                    {
                        2nd_saved_dist_mm = latest_dist_mm;
                    }
                    else
                    {
                        // Handle error
                        2nd_saved_dist_mm = 9999;
                    }
                    laser_off();
                }
            }

            // Pressing Dwn brings back to main menu
            if (get_button_state() == 3)
            {
                state = 0;
                laser_off();
            }
            else if (get_button_state() == 1)
            {
                trigstate = 1;
            }
        }
        // Lock 2nd measurement
        else if (state == 9)
        {
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)2nd_saved_dist_mm);
            OLED_GoToLine(0);
            OLED_DisplayString("2nd Dist: ");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            OLED_DisplayString(">Confirm");
            OLED_GoToLine(6);
            OLED_DisplayString("Retake");
            if (get_button_state() == 1)
            {
                state = 11;
            }
            else if (get_button_state() == 3)
            {
                state = 10;
            }
        }
        else if (state == 10)
        {
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)2nd_saved_dist_mm);
            OLED_GoToLine(0);
            OLED_DisplayString("2nd Dist: ");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            OLED_DisplayString("Confirm");
            OLED_GoToLine(6);
            OLED_DisplayString(">Retake");
            if (get_button_state() == 1)
            {
                laser_on();
                state = 8;
            }
            else if (get_button_state() == 2)
            {
                state = 9;
            }
        }
        // Confirm room, ask for retake of 1st, or cancel
        else if (state == 11)
        {
            if (get_button_state() == 1)
            {
                state = 0;
            }
            else if (get_button_state() == 3)
            {
                state = 12;
            }
        }
        else if (state == 12)
        {
            if (get_button_state() == 1)
            {
                state = 14;
            }
            else if (get_button_state() == 2)
            {
                state = 12;
            }
            else if (get_button_state() == 3)
            {
                state = 13;
            }
        }
        else if (state == 13)
        {
            if (get_button_state() == 1)
            {
                state = 0;
            }
            else if (get_button_state() == 2)
            {
                state = 12;
            }
        }
        // Retake 1st measurement
        else if (state == 14)
        {
            // MAKE CODE FOR TRIGGER DEBOUNCE

            // Pressing Dwn brings back to main menu
            else if (get_button_state() == 3)
            {
                state = 0;
            }
        }
        // Lock retake
        else if (state == 15)
        {
            if (get_button_state() == 1)
            {
                state = 17;
            }
            else if (get_button_state() == 3)
            {
                state = 16;
            }
        }
        else if (state == 16)
        {
            if (get_button_state() == 1)
            {
                state = 14;
            }
            else if (get_button_state() == 2)
            {
                state = 15;
            }
        }
        // Confirm retake, ask for retake of 2nd, or cancel
        else if (state == 17)
        {
            if (get_button_state() == 1)
            {
                state = 0;
            }
            else if (get_button_state() == 3)
            {
                state = 18;
            }
        }
        else if (state == 18)
        {
            // Retake 2nd
            if (get_button_state() == 1)
            {
                state = 8;
            }
            else if (get_button_state() == 2)
            {
                state = 17;
            }
            else if (get_button_state() == 3)
            {
                state = 18;
            }
        }
        else if (state == 19)
        {
            if (get_button_state() == 1)
            {
                state = 0;
            }
            else if (get_button_state() == 2)
            {
                state = 18;
            }
        }
        // Saved room menu
        else if (state == 20)
        {
            if (get_button_state() == 1)
            {
                state = 21;
            }
            else if (get_button_state() == 2)
            {
                // Scroll through roomids
                roomid = roomid + 1;
            }
            else if (get_button_state() == 3)
            {
                state = 0;
            }
        }
        // Room n display
        else if (state == 21)
        {
            if (get_button_state() == 1)
            {
                state = 20;
            }
            else if (get_button_state() == 3)
            {
                state = 22;
            }
        }
        else if (state == 22)
        {
            if (get_button_state() == 1)
            {
                state = 23;
            }
            else if (get_button_state() == 2)
            {
                state = 21;
            }
        }
        // Delete room conf. menu
        else if (state == 23)
        {
            // Deny delete
            if (get_button_state() == 1)
            {
                state = 21;
            }
            else if (get_button_state() == 3)
            {
                state = 24;
            }
        }
        else if (state == 24)
        {
            // Confirm delete
            if (get_button_state() == 1)
            {
                state = 20;
            }
            else if (get_button_state() == 2)
            {
                state = 23;
            }
        }
        // Options menu
        else if (state == 25)
        {
            if (get_button_state() == 1)
            {
                // CHANGE DISPLAY UNIT
            }
            else if (get_button_state() == 3)
            {
                state = 26;
            }
        }
        else if (state == 26)
        {
            if (get_button_state() == 1)
            {
                state = 0;
            }
            else if (get_button_state() == 2)
            {
                state = 25;
            }
        }
        else
        {
            // If any errors, set back to main menu
            state = 0;
        }

        /*
        // TESTING CODE
        OLED_Clear();
        OLED_GoToLine(4);
        OLED_DisplayString("Trigger...");
        if (get_button_state() == 1)
        {
            
            OLED_Clear();
            OLED_GoToLine(4);
            OLED_DisplayString("Pressed!");

            send_bytes(CMD_SINGLE_RANGE, 10);
            if (receive_ranging_frame(&latest_dist_mm))
            {
                saved_dist_mm = latest_dist_mm;
            }
            else
            {
                // Handle error
                saved_dist_mm = 9999;
            }

            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)saved_dist_mm);

            OLED_Clear();
            OLED_GoToLine(4);
            OLED_DisplayString("Dist: ");
            OLED_GoToLine(6);
            OLED_DisplayString(buffer);
            _delay_ms(10000);
        }
        */

        _delay_ms(300);

    }

    return(0);
}