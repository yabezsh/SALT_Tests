#include "CurrentMonitor.h"

CurrentMonitor::CurrentMonitor(int8_t  bus_number = 1, int8_t device_address = 0b01000000) : I2C( bus_number, device_address){} 

CurrentMonitor::~CurrentMonitor(){}


void CurrentMonitor::configure()
{
    bit_15_8 = 0b00000100;
    bit_7_0 = 0b00011111;
    write_buffer(0, bit_15_8, bit_7_0);
    
    bit_15_8 = 0b00100000;
    bit_7_0 = 0b00000000;
    write_buffer(0b101, bit_15_8, bit_7_0);
}

void CurrentMonitor::read_current(uint16_t *current_counts)
{
    read_buffer(0b00000100, current_counts);
    printf("Values: HEX 0x%02x\n  or  %i mA \n",*current_counts, (*current_counts)/20);
}