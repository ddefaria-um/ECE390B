/*
HPS166.c
Code for HPS-166 TOF Ranging Sensor
Damon DeFaria 3/30/2026 */

#include "HPS166.h"
#include "my_uart_lib.h"

static const uint8_t CMD_SINGLE_RANGE[10] = {
    0x0A, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xAE, 0x57
};

static const uint8_t CMD_CONTINUOUS_RANGING[10] = {
    0x0A, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x72
};

static const uint8_t CMD_STOP_RANGING[10] = {
    0x0A, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBC, 0x6F
};

// Receive ranging data from sensor UART
bool receive_ranging_frame(uint16_t *distance_mm)
{
    uint8_t  buf[RANGING_FRAME_LEN];
    uint8_t  idx = 0;
 
    // Read bytes until we find the start byte 0x0A
    while (true) {
        if (!uart_getc_timeout(&buf[0])) return false;
        if (buf[0] == 0x0A) break;
    }
 
    // Read the remaining 14 bytes
    for (idx = 1; idx < RANGING_FRAME_LEN; idx++) {
        if (!uart_getc_timeout(&buf[idx])) return false;
    }
 
    // Verify data-length byte == 0x0D (13)
    if (buf[1] != 0x0D) return false;
 
    // Decode distance: bytes 5 (MSB) and 6 (LSB), unit mm
    *distance_mm = ((uint16_t)buf[5] << 8) | buf[6];
 
    // Check for over-range (65530 mm = 65.53 m)
    if (*distance_mm >= 65530) return false;
 
    return true;
}
 
// Send stop command and wait for ACK
bool sensor_stop_ranging(void)
{
    uint8_t buf[STOP_ACK_LEN];
    uint8_t idx;
 
    send_bytes(CMD_STOP_RANGING, 10);
 
    // Wait for start byte
    while (true) {
        if (!uart_getc_timeout(&buf[0])) return false;
        if (buf[0] == 0x0A) break;
    }
    for (idx = 1; idx < STOP_ACK_LEN; idx++) {
        if (!uart_getc_timeout(&buf[idx])) return false;
    }
 
    // ACK byte is at index 2; 0x01 = success
    return (buf[2] == 0x01);
}