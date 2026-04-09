/*
HPS166.h
Header file for HPS-166 TOF Ranging Sensor
Damon DeFaria 3/30/2026 */

#ifndef HPS166_H
#define HPS166_H

#include <stdint.h>
#include <stdbool.h>

#define RANGING_FRAME_LEN   15   /* 0x0A + 0x0D(len) + 13 data bytes     */
#define STOP_ACK_LEN         5   /* 0x0A + 0x03(len) + 3 data bytes      */

extern const uint8_t CMD_SINGLE_RANGE[10];
extern const uint8_t CMD_CONTINUOUS_RANGING[10];
extern const uint8_t CMD_STOP_RANGING[10];

bool receive_ranging_frame(uint16_t *distance_mm);

bool sensor_stop_ranging(void);

#endif
