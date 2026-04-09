/* Main code for 390B Mapping Device Project
v0.1 3/30/2026 - Initial version
v0.2 3/31/2026 - Added and finalized UART and HPS-166 libraries
v0.3 4/1/2026 - Added sensor reset and button trigger
v0.4 4/6/2026 - Changed to Single Range, added error handling for testing
v0.5 4/8/2026 - Added SSD1306 OLED code for testing, works!!
v0.6 4/9/2026 - Created buttonhandler library for buttons and laser
Damon DeFaria 34305913  */
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#include "i2c.h"
#include "my_uart_lib.h"
#include "HPS166.h"
#include "SSD1306.h"
#include "buttonhandler.h"

#define RST_PIN PD2  // Reset pin for HPS-166 connected to PORTD2

static const uint8_t CMD_SINGLE_RANGE[10] = {
    0x0A, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAE, 0x57
};

static const uint8_t CMD_CONTINUOUS_RANGING[10] = {
    0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x72
};

static const uint8_t CMD_STOP_RANGING[10] = {
    0x0A, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBC, 0x6F
};


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
    int state;

    DDRD |= (1 << RST_PIN);
    PORTD |= (1 << RST_PIN);

    button_init();
    uart_init();
    OLED_Init();
    
    OLED_Clear();
    OLED_GoToLine(4);
    OLED_DisplayString("Startup...");

    _delay_ms(1000);

    uint16_t latest_dist_mm = 0;
    uint16_t saved_dist_mm = 0;
    
    
    // TESTING CODE
    while (1)
    {
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
        _delay_ms(100);

    }

    return(0);
}