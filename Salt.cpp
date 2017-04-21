//Salt.cpp
#include "Salt.h"
#include <iostream>

Salt::Salt()
{
    this->open_bus();
}
Salt::~Salt(){}

void Salt::find_device_address(int8_t high_reg_bits)
{
    this -> set_device_address(chip_id << 5 | high_reg_bits);
}

void Salt::read_salt(int8_t high_reg_bits, int8_t low_reg_bits, uint8_t* command)
{
    this -> find_device_address(high_reg_bits);
    this -> get_bus_access();
    this -> read_buffer(low_reg_bits,command);
}