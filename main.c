/* 
main.c
Main code for 390B Mapping Device Project 
Damon DeFaria, Marzooq Jeje */

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "i2c.h"
#include "my_uart_lib.h"
#include "HPS166.h"
#include "SSD1306.h"
#include "buttonhandler.h"
#include "roomdatahandler.h"

// Define the new OLED reset pin (Connect Adafruit RST to PD0)
// #define OLED_RST_PIN PD0

void sensor_reset(void)
{
    DDRD  |=  (1 << RST_PIN);    // RST as output
    PORTD &= ~(1 << RST_PIN);    // Assert reset (active low)
    _delay_ms(10);
    PORTD |=  (1 << RST_PIN);    // Release reset
    _delay_ms(500);              // Wait for "Hypersen" init string
    uart_flush_rx();             // Discard the init string
}

const char* unit_label(uint8_t unit)
{
    switch (unit)
    {
        case UNIT_MM:   return "mm";
        case UNIT_CM:   return "cm";
        case UNIT_INCH: return "in";
        case UNIT_FT:   return "ft";
        case UNIT_M:    return "m";
        default:        return "mm";
    }
}

int main(void)
{
    int state = 0;
    int trigstate = 0;
    
    // 0 for mm, 1 for cm, 2 for in, 3 for ft, 4 for m
    uint8_t unit = read_unit();

    pin_init();
    uart_init();
    
    // Kickstart both devices before trying to talk to them
    sensor_reset();
    //oled_reset();
    
    OLED_Init();
    
    OLED_Clear();
    OLED_GoToLine(4);
    OLED_DisplayString("Startup...");

    _delay_ms(1000);

    uint16_t latest_dist_mm = 0;
    uint16_t first_saved_dist_mm = 0;
    uint16_t second_saved_dist_mm = 0;
    uint16_t roomid = 1;
    uint16_t roomcount = get_room_count();
    uint16_t offset = read_offset();
    char buffer[16];
    char roombuffer[32];
    char dist1_str[10];
    char dist2_str[10];
    char m1_str[10];
    char m2_str[10];
    
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
                roomcount = get_room_count();
                unit = read_unit();
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
                unit = read_unit();
                offset = read_offset();
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
            OLED_DisplayString("Press Trigger");
            OLED_GoToLine(2);
            OLED_DisplayString("Dwn to Cancel");
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
                        first_saved_dist_mm = latest_dist_mm - offset;
                    }
                    else
                    {
                        // Will grab the custom 999x error code from the function
                        first_saved_dist_mm = latest_dist_mm; 
                    }
                    dtostrf(convert_distance(first_saved_dist_mm, unit), 1, 1, dist1_str);
                    snprintf(buffer, sizeof(buffer), "%s %s", dist1_str, unit_label(unit));
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
            OLED_GoToLine(0);
            OLED_DisplayString("Dist: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm");
            OLED_GoToLine(4);
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
            OLED_GoToLine(0);
            OLED_DisplayString("Dist: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm");
            OLED_GoToLine(4);
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
            OLED_DisplayString("Press Trigger");
            OLED_GoToLine(2);
            OLED_DisplayString("Dwn to Cancel");
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
                        second_saved_dist_mm = latest_dist_mm - offset;
                    }
                    else
                    {
                        second_saved_dist_mm = latest_dist_mm;
                    }
                    laser_off();
                    dtostrf(convert_distance(second_saved_dist_mm, unit), 1, 1, dist2_str);
                    snprintf(buffer, sizeof(buffer), "%s %s", dist2_str, unit_label(unit));
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
            OLED_GoToLine(0);
            OLED_DisplayString("2nd Dist: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake");
            if (get_button_state() == 1)
            {
                state = 11;
                snprintf(buffer, sizeof(buffer), "%sx%s %s", dist1_str, dist2_str, unit_label(unit));
            }
            else if (get_button_state() == 3)
            {
                state = 10;
            }
        }
        else if (state == 10)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("2nd Dist: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm");
            OLED_GoToLine(4);
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
            OLED_DisplayString("Room: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake 1st");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");
            if (get_button_state() == 1)
            {
                state = 0;
                while (roomid < MAX_ROOMS && !room_is_empty(roomid))
                {
                    roomid++;
                }
                if (roomid < MAX_ROOMS)
                {
                    write_room(first_saved_dist_mm, second_saved_dist_mm, roomid);
                    roomcount++;
                    state = 0;
                }
                else
                {
                    state = 90; // No more room to save
                }
            }
            else if (get_button_state() == 3)
            {
                state = 12;
            }
        }
        else if (state == 12)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("Room: ");
            OLED_DisplayString(buffer);
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
                state = 11;
            }
            else if (get_button_state() == 3)
            {
                state = 13;
            }
        }
        else if (state == 13)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("Room: ");
            OLED_DisplayString(buffer);
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
            OLED_DisplayString("Press Trigger");
            OLED_GoToLine(2);
            OLED_DisplayString("Dwn to Return");
            // Pressing Dwn brings back to main menu
            while (trigstate == 1)
            {
                if (!(BUTTON_PIN & (1 << TRIGGER_PIN))) // FIXED: using BUTTON_PIN
                {
                    trigstate = 0;
                    state = 15;
                    
                    uart_flush_rx(); // Clear garbage out of buffer before firing
                    send_bytes(CMD_SINGLE_RANGE, 10);
                    
                    if (receive_ranging_frame(&latest_dist_mm))
                    {
                        first_saved_dist_mm = latest_dist_mm - offset;
                    }
                    else
                    {
                        // Will grab the custom 999x error code from the function
                        first_saved_dist_mm = latest_dist_mm; 
                    }
                    dtostrf(convert_distance(first_saved_dist_mm, unit), 1, 1, dist1_str);
                    snprintf(buffer, sizeof(buffer), "%s %s", dist1_str, unit_label(unit));
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
            OLED_GoToLine(0);
            OLED_DisplayString("Dist: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake");
            
            if (get_button_state() == 1)
            {
                state = 17;
                snprintf(buffer, sizeof(buffer), "%sx%s %s", dist1_str, dist2_str, unit_label(unit));
            }
            else if (get_button_state() == 3)
            {
                state = 16;
            }
        }
        else if (state == 16)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("Dist: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString("Confirm");
            OLED_GoToLine(4);
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
            OLED_DisplayString("Room: ");
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(">Confirm Room");
            OLED_GoToLine(4);
            OLED_DisplayString("Retake 2nd");
            OLED_GoToLine(6);
            OLED_DisplayString("Cancel");

            if (get_button_state() == 1)
            {
                state = 0;
                while (roomid < MAX_ROOMS && !room_is_empty(roomid))
                {
                    roomid++;
                }
                if (roomid < MAX_ROOMS)
                {
                    write_room(first_saved_dist_mm, second_saved_dist_mm, roomid);
                    roomcount++;
                    state = 0;
                }
                else
                {
                    state = 90; // No more room to save
                }
            }
            else if (get_button_state() == 3)
            {
                state = 18;
            }
        }
        else if (state == 18)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("Room: ");
            OLED_DisplayString(buffer);
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
            OLED_DisplayString("Room: ");
            OLED_DisplayString(buffer);
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
            snprintf(buffer, sizeof(buffer), ">Room %d/%d", roomid, roomcount);
            OLED_GoToLine(0);
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString("Up to Scroll Rooms");
            OLED_GoToLine(4);
            OLED_DisplayString("Dwn to Main Menu");
            if (get_button_state() == 1)
            {
                if (roomcount == 0)
                {
                    // No rooms saved
                    state = 91;
                }
                else
                {
                    state = 21;
                    RoomData room = read_room(roomid);
                    dtostrf(convert_distance(room.m1, unit), 1, 1, m1_str);
                    dtostrf(convert_distance(room.m2, unit), 1, 1, m2_str);
                    snprintf(buffer, sizeof(buffer), "Room %d Dim.", roomid);
                    snprintf(roombuffer, sizeof(roombuffer), "%sx%s %s", m1_str, m2_str, unit_label(unit));
                }
            }
            else if (get_button_state() == 2)
            {
                // Scroll through roomids
                if (!room_is_empty(roomid + 1))
                {
                    roomid = roomid + 1;
                }
                else
                {
                    roomid = 1;
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
            OLED_GoToLine(0);
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(roombuffer);
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
            OLED_GoToLine(0);
            OLED_DisplayString(buffer);
            OLED_GoToLine(2);
            OLED_DisplayString(roombuffer);
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
                delete_and_defragment(roomid);
                roomcount--;
                roomid = 1;

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
                snprintf(buffer, sizeof(buffer), ">Unit (mm)");
            }
            else if (unit == 1)
            {
                snprintf(buffer, sizeof(buffer), ">Unit (cm)");
            }
            else if (unit == 2)
            {
                snprintf(buffer, sizeof(buffer), ">Unit (in)");
            }
            else if (unit == 3)
            {
                snprintf(buffer, sizeof(buffer), ">Unit (ft)");
            }
            else
            {
                snprintf(buffer, sizeof(buffer), ">Unit (m)");
            }
            OLED_GoToLine(0);
            OLED_DisplayString("Options");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            snprintf(buffer, sizeof(buffer), "Offset: %d %s", convert_offset(offset, unit), unit_label(unit));
            OLED_DisplayString(buffer);
            OLED_GoToLine(6);
            OLED_DisplayString("Return");
            if (get_button_state() == 1)
            {
                if (unit < UNIT_M)
                {
                    unit++;
                }
                else
                {
                    unit = 0;
                }
                write_unit(unit);
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
                snprintf(buffer, sizeof(buffer), "Unit (mm)");
            }
            else if (unit == 1)
            {
                snprintf(buffer, sizeof(buffer), "Unit (cm)");
            }
            else if (unit == 2)
            {
                snprintf(buffer, sizeof(buffer), "Unit (in)");
            }
            else if (unit == 3)
            {
                snprintf(buffer, sizeof(buffer), "Unit (ft)");
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "Unit (m)");
            }
            OLED_GoToLine(0);
            OLED_DisplayString("Options");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            snprintf(buffer, sizeof(buffer), ">Offset: %d %s", convert_offset(offset, unit), unit_label(unit));
            OLED_DisplayString(buffer);
            OLED_GoToLine(6);
            OLED_DisplayString("Return");
            if (get_button_state() == 1)
            {
                adjust_offset(unit);
                offset = read_offset();
            }
            else if (get_button_state() == 2)
            {
                state = 25;
            }
            else if (get_button_state() == 3)
            {
                state = 27;
            }
        }
        else if (state == 27)
        {
            if (unit == 0)
            {
                snprintf(buffer, sizeof(buffer), "Unit (mm)");
            }
            else if (unit == 1)
            {
                snprintf(buffer, sizeof(buffer), "Unit (cm)");
            }
            else if (unit == 2)
            {
                snprintf(buffer, sizeof(buffer), "Unit (in)");
            }
            else if (unit == 3)
            {
                snprintf(buffer, sizeof(buffer), "Unit (ft)");
            }
            else
            {
                snprintf(buffer, sizeof(buffer), "Unit (m)");
            }
            OLED_GoToLine(0);
            OLED_DisplayString("Options");
            OLED_GoToLine(2);
            OLED_DisplayString(buffer);
            OLED_GoToLine(4);
            snprintf(buffer, sizeof(buffer), "Offset: %d %s", convert_offset(offset, unit), unit_label(unit));
            OLED_DisplayString(buffer);
            OLED_GoToLine(6);
            OLED_DisplayString(">Return");
            if (get_button_state() == 1)
            {
                state = 0;
            }
            else if (get_button_state() == 2)
            {
                state = 26;
            }
        }
        // ERROR NOT ENOUGH ROOM SPACE TO SAVE
        else if (state == 90)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("ERROR");
            OLED_GoToLine(2);
            OLED_DisplayString("NO SPACE");
            OLED_GoToLine(4);
            OLED_DisplayString(">Main Menu");
            if (get_button_state() == 1)
            {
                state = 0;
            }
        }
        // ERROR NO ROOMS TO DISPLAY
        else if (state == 91)
        {
            OLED_GoToLine(0);
            OLED_DisplayString("ERROR");
            OLED_GoToLine(2);
            OLED_DisplayString("NO SAVED ROOMS");
            OLED_GoToLine(4);
            OLED_DisplayString(">Main Menu");
            if (get_button_state() == 1)
            {
                state = 0;
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