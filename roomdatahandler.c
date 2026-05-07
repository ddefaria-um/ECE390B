/*
roomdatahandler.c
Code for handling room data
Damon DeFaria, Eric Bellavia 5/7/2026 */

#include "roomdatahandler.h"

RoomData EEMEM rooms[MAX_ROOMS];
uint8_t EEMEM stored_unit;

void write_room(uint16_t m1, uint16_t m2, uint16_t room_id)
{
   if (room_id >= MAX_ROOMS) return;

   RoomData temp;
   temp.m1 = m1;
   temp.m2 = m2;

   eeprom_update_block(
       (const void*)&temp,
       (void*)&rooms[room_id],
       sizeof(RoomData)
   );
}

RoomData read_room(uint16_t room_id)
{
   RoomData temp;


   if (room_id >= MAX_ROOMS) {
       temp.m1 = 0;
       temp.m2 = 0;
       return temp;
   }


   eeprom_read_block(
       (void*)&temp,
       (const void*)&rooms[room_id],
       sizeof(RoomData)
   );


   return temp;
}

void delete_room(uint16_t room_id)
{
    if (room_id >= MAX_ROOMS) return;

    RoomData empty;
    empty.m1 = 0xFFFF;
    empty.m2 = 0xFFFF;

    eeprom_update_block(
        (const void*)&empty,
        (void*)&rooms[room_id],
        sizeof(RoomData)
    );
}

void defragment_rooms(void)
{
    uint16_t write_id = 0;
    uint16_t read_id;

    for (read_id = 0; read_id < MAX_ROOMS; read_id++)
    {
        if (!room_is_empty(read_id))
        {
            if (read_id != write_id)
            {
                RoomData temp = read_room(read_id);
                write_room(temp.m1, temp.m2, write_id);
                delete_room(read_id);
            }
            write_id++;
        }
    }
}

void delete_and_defragment(uint16_t room_id)
{
    if (room_id >= MAX_ROOMS) return;
    if (room_is_empty(room_id)) return;

    delete_room(room_id);
    defragment_rooms();
}

uint8_t room_is_empty(uint16_t room_id)
{
    RoomData temp = read_room(room_id);
    return (temp.m1 == 0xFFFF && temp.m2 == 0xFFFF);
}

uint16_t get_room_count(void)
{
    uint16_t count = 0;
    uint16_t i;

    for (i = 0; i < MAX_ROOMS; i++)
    {
        if (!room_is_empty(i))
            count++;
    }

    return count;
}

void write_unit(uint8_t unit)
{
    if (unit > UNIT_M) return;

    eeprom_update_byte(&stored_unit, unit);
}

uint8_t read_unit(void)
{
    uint8_t unit = eeprom_read_byte(&stored_unit);

    if (unit > UNIT_M) return UNIT_MM;

    return unit;
}

float convert_distance(uint16_t distance_mm, uint8_t unit)
{
    float result;

    switch (unit)
    {
        case UNIT_MM:   result = (float)distance_mm;           break;
        case UNIT_CM:   result = (float)distance_mm / 10.0f;   break;
        case UNIT_INCH: result = (float)distance_mm / 25.4f;   break;
        case UNIT_FT:   result = (float)distance_mm / 304.8f;  break;
        case UNIT_M:    result = (float)distance_mm / 1000.0f; break;
        default:        result = (float)distance_mm;           break;
    }

    return ((float)(int32_t)(result * 10.0f + 0.5f)) / 10.0f;
}