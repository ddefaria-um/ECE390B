/*
roomdatahandler.h
Header file for handling room data
Damon DeFaria, Eric Bellavia 5/7/2026 */

#ifndef ROOMDATAHANDLER_H
#define ROOMDATAHANDLER_H

#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdint.h>

#define MAX_ROOMS 20

#define UNIT_MM     0
#define UNIT_CM     1
#define UNIT_INCH   2
#define UNIT_FT     3
#define UNIT_M      4

#define UNIT_UNSET  0xFF

typedef struct {
   uint16_t m1;
   uint16_t m2;
} RoomData;


void write_room(uint16_t m1, uint16_t m2, uint16_t room_id);
RoomData read_room(uint16_t room_id);
void delete_room(uint16_t room_id);
void defragment_rooms(void);
void delete_and_defragment(uint16_t room_id);
uint8_t room_is_empty(uint16_t room_id);
uint16_t get_room_count(void);
void write_unit(uint8_t unit);
uint8_t read_unit();
float convert_distance(uint16_t distance_mm, uint8_t unit);

#endif