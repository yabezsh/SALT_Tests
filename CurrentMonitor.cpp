#include "CurrentMonitor.h"

CurrentMonitor::CurrentMonitor(int8_t  bus_number = 1, int8_t device_address = 0b01000000) : I2C( bus_number, device_address){} 

CurrentMonitor::~CurrentMonitor(){}


void CurrentMonitor::define_setup()
{
    this->configure(); 
    this->calibrate();
}

void CurrentMonitor::configure()
{
    write_buffer(config_register, bit_15_8_conf, bit_7_0_conf);
}

void CurrentMonitor::calibrate()
{
    write_buffer(calib_register, bit_15_8_calib, bit_7_0_calib);
}

void CurrentMonitor::read_current(uint16_t *current_counts)
{
    read_buffer(0b00000100, current_counts);
    printf("Values: HEX 0x%02x\n  or  %i mA \n",*current_counts, (*current_counts)/20);
}

void CurrentMonitor::set_config_bits(uint16_t config_bits)
{
    bit_15_8_conf = (config_bits >> 8);
    bit_7_0_conf = config_bits & 0xff;
}

void CurrentMonitor::set_config_bits(uint8_t bit_7_0_conf,uint8_t bit_15_8_conf)
{
    bit_15_8_conf = bit_15_8_conf;
    bit_7_0_conf = bit_7_0_conf;
}

void CurrentMonitor::set_calib_bits(uint16_t calib_bits)
{
    
    bit_15_8_calib = (calib_bits >> 8);
    bit_7_0_calib = calib_bits & 0xff;
}

void CurrentMonitor::set_calib_bits(uint8_t bit_7_0_calib,uint8_t bit_15_8_calib)
{
    
    bit_15_8_calib = bit_15_8_calib;
    bit_7_0_calib = bit_7_0_calib;
}
