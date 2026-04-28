/* Main code for 390B Mapping Device Project */

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "i2c.h"
#include "my_uart_lib.h"
#include "HPS166.h"
#include "SSD1306.h"
#include "buttonhandler.h"

// Define the new OLED reset pin (Connect Adafruit RST to PD0)
#define OLED_RST_PIN PD0

void sensor_reset(void)
{
    DDRD  |=  (1 << RST_PIN);    // RST as output
    PORTD &= ~(1 << RST_PIN);    // Assert reset (active low)
    _delay_ms(10);
    PORTD |=  (1 << RST_PIN);    // Release reset
    _delay_ms(500);              // Wait for "Hypersen" init string
    uart_flush_rx();             // Discard the init string
}

void oled_reset(void)
{
    DDRD  |=  (1 << OLED_RST_PIN); // RST as output
    PORTD &= ~(1 << OLED_RST_PIN); // Pull low to reset
    _delay_ms(10);
    PORTD |=  (1 << OLED_RST_PIN); // Pull high to run
    _delay_ms(100);                // Wait for OLED to boot
}

int main(void)
{
    int state = 0;
    int roomid = 0;
    int trigstate = 0;
    int roomcount = 0;
    int unit = 0; // 0 for mm, 1 for inches

    pin_init();
    uart_init();
    
    // Kickstart both devices before trying to talk to them
    sensor_reset();
    oled_reset();
    
    OLED_Init();
    
    OLED_Clear();
    OLED_GoToLine(4);
    OLED_DisplayString("Startup...");

    _delay_ms(1000);

    uint16_t latest_dist_mm = 0;
    uint16_t first_saved_dist_mm = 0;
    uint16_t second_saved_dist_mm = 0;
    char buffer[16];
    
    while (1)
    {
        // Main Menu depending on button pressed
        OLED_Clear();
        if (state == 0)
        {
            OLED_GoToLine(0);
            OLED_DisplayString(">Measure");
            OLED_GoToLine(2);
            OLED_DisplayString("View Rooms");
            OLED_GoToLine(4);
            OLED_DisplayString("Options");
            
            // Turn laser off just in case of error
            if (laser_is_on() == 1)
            {
                laser_off();
            }

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
            OLED_GoToLine(0);
            OLED_DisplayString("Measure");
            OLED_GoToLine(2);
            OLED_DisplayString(">View Rooms");
            OLED_GoToLine(4);
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
            OLED_GoToLine(0);
            OLED_DisplayString("Measure");
            OLED_GoToLine(2);
            OLED_DisplayString("View Rooms");
            OLED_GoToLine(4);
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
            OLED_GoToLine(0);
            OLED_DisplayString("Hold and Release Trigger");
            OLED_GoToLine(2);
            OLED_DisplayString("Press Dwn to Cancel");
            // Pressing Dwn brings back to main menu
            while (trigstate == 1)
            {
                if (!(BUTTON_PIN & (1 << TRIGGER_PIN))) // FIXED: using BUTTON_PIN
                {
                    trigstate = 0;
                    state = 4;
                    
                    uart_flush_rx(); // Clear garbage out of buffer before firing
                    send_bytes(CMD_SINGLE_RANGE, 10);
                    
                    if (receive_ranging_frame(&latest_dist_mm))
                    {
                        first_saved_dist_mm = latest_dist_mm;
                    }
                    else
                    {
                        // Will grab the custom 999x error code from the function
                        first_saved_dist_mm = latest_dist_mm; 
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
            
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)first_saved_dist_mm);
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
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)first_saved_dist_mm);
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
                first_saved_dist_mm = 0;
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
            OLED_GoToLine(0);
            OLED_DisplayString(">Take 2nd Measure.");
            OLED_GoToLine(2);
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
            OLED_GoToLine(0);
            OLED_DisplayString("Take 2nd Measure.");
            OLED_GoToLine(2);
            OLED_DisplayString(">Cancel");
            if (get_button_state() == 1)
            {
                state = 0;
                first_saved_dist_mm = 0;
            }
            else if (get_button_state() == 2)
            {
                state = 6;
            }
        }
        // Take 2nd measurement
        else if (state == 8)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("Hold and Release Trigger");
            OLED_GoToLine(2);
            OLED_DisplayString("Press Dwn to Cancel");
            while (trigstate == 1)
            {
                if (!(BUTTON_PIN & (1 << TRIGGER_PIN))) // FIXED: using BUTTON_PIN
                {
                    trigstate = 0;
                    state = 9;
                    
                    uart_flush_rx(); // Clear garbage out of buffer before firing
                    send_bytes(CMD_SINGLE_RANGE, 10);
                    
                    if (receive_ranging_frame(&latest_dist_mm))
                    {
                        second_saved_dist_mm = latest_dist_mm;
                    }
                    else
                    {
                        second_saved_dist_mm = latest_dist_mm;
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
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)second_saved_dist_mm);
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
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)second_saved_dist_mm);
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
            OLED_GoToLine(0);
            // edit this to show both distances
            OLED_DisplayString("ROOM DISTANCE");
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake 1st");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");
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
            OLED_GoToLine(0);
            // edit this to show both distances
            OLED_DisplayString("ROOM DISTANCE");
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString(">Retake 1st");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");
            if (get_button_state() == 1)
            {
                state = 14;
                laser_on();
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
            OLED_GoToLine(0);
            // edit this to show both distances
            OLED_DisplayString("ROOM DISTANCE");
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake 1st");
            OLED_GoToLine(6);
            OLED_DisplayString(">Cancel");
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
            OLED_GoToLine(0);
            OLED_DisplayString("Hold and Release Trigger");
            OLED_GoToLine(2);
            OLED_DisplayString("Press Dwn to Return");
            // Pressing Dwn brings back to main menu
            while (trigstate == 1)
            {
                if (!(BUTTON_PIN & (1 << TRIGGER_PIN)))
                {
                    trigstate = 0;
                    state = 15;
                    
                    uart_flush_rx(); // Clear garbage out of buffer before firing
                    send_bytes(CMD_SINGLE_RANGE, 10);
                    
                    if (receive_ranging_frame(&latest_dist_mm))
                    {
                        first_saved_dist_mm = latest_dist_mm;
                    }
                    else
                    {
                        // Will grab the custom 999x error code from the function
                        first_saved_dist_mm = latest_dist_mm; 
                    }
                    laser_off();
                }
            }
            
            if (get_button_state() == 3)
            {
                state = 12;
                laser_off();
            }
            else if (get_button_state() == 1)
            {
                trigstate = 1;
            }

        }
        // Lock retake
        else if (state == 15)
        {
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)first_saved_dist_mm);
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
                state = 17;
            }
            else if (get_button_state() == 3)
            {
                state = 16;
            }
        }
        else if (state == 16)
        {
            snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)first_saved_dist_mm);
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
                first_saved_dist_mm = 0;
                state = 14;
                laser_on();
            }
            else if (get_button_state() == 2)
            {
                state = 15;
            }
        }
        // Confirm retake, ask for retake of 2nd, or cancel
        else if (state == 17)
        {
            OLED_GoToLine(0);
            // edit this to show both distances
            OLED_DisplayString("ROOM DISTANCE");
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake 2nd");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");

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
            OLED_GoToLine(0);
            // edit this to show both distances
            OLED_DisplayString("ROOM DISTANCE");
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString(">Retake 2nd");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");
            // Retake 2nd
            if (get_button_state() == 1)
            {
                state = 8;
                laser_on();
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
            OLED_GoToLine(0);
            // edit this to show both distances
            OLED_DisplayString("ROOM DISTANCE");
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake 2nd");
            OLED_GoToLine(6);
            OLED_DisplayString(">Cancel");
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
            snprintf(buffer, sizeof(buffer), ">View Room %d", roomid);
            OLED_GoToLine(0);
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString("Press Up to Scroll Rooms");
            OLED_GoToLine(4);
            OLED_DisplayString("Press Dwn to Main Menu");
            if (get_button_state() == 1)
            {
                state = 21;
            }
            else if (get_button_state() == 2)
            {
                // Scroll through roomids
                if (roomid < roomcount)
                {
                    roomid = roomid + 1;
                }
                else
                {
                    roomid = 0;
                }
            }
            else if (get_button_state() == 3)
            {
                state = 0;
            }
        }
        // Room n display
        else if (state == 21)
        {
            // Display dimensions of room n
            snprintf(buffer, sizeof(buffer), "Room %d Dimensions", roomid);
            OLED_GoToLine(0);
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            // edit this
            OLED_DisplayString("ROOM DIMENSIONS");
            OLED_GoToLine(4);
            OLED_DisplayString(">Return");
            OLED_GoToLine(6);
            OLED_DisplayString("Delete Room");

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
            snprintf(buffer, sizeof(buffer), "Room %d Dimensions", roomid);
            OLED_GoToLine(0);
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            // edit this
            OLED_DisplayString("ROOM DIMENSIONS");
            OLED_GoToLine(4);
            OLED_DisplayString("Return");
            OLED_GoToLine(6);
            OLED_DisplayString(">Delete Room");
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
            OLED_GoToLine(0);
            OLED_DisplayString("Delete Room?");
            OLED_GoToLine(2);
            OLED_DisplayString(">No");
            OLED_GoToLine(4);
            OLED_DisplayString("Yes");
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
            OLED_GoToLine(0);
            OLED_DisplayString("Delete Room?");
            OLED_GoToLine(2);
            OLED_DisplayString("No");
            OLED_GoToLine(4);
            OLED_DisplayString(">Yes");
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
            if (unit == 0)
            {
                snprintf(buffer, sizeof(buffer), ">Change Display Unit (mm)");
            }
            else
            {
                snprintf(buffer, sizeof(buffer), ">Change Display Unit (in)");
            }
            OLED_GoToLine(0);
            OLED_DisplayString("Options");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            OLED_DisplayString("Return");
            if (get_button_state() == 1)
            {
                if (unit == 0)
                {
                    unit = 1;
                }
                else
                {
                    unit = 0;
                }
            }
            else if (get_button_state() == 3)
            {
                state = 26;
            }
        }
        else if (state == 26)
        {
            if (unit == 0)
            {
                snprintf(buffer, sizeof(buffer), "Change Display Unit (mm)");
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "Change Display Unit (in)");
            }
            OLED_GoToLine(0);
            OLED_DisplayString("Options");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            OLED_DisplayString(">Return");
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

        _delay_ms(300);

    }

    return(0);
}