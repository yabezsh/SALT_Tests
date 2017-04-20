//ExternalADC.h
#include "ExternalADC.h"

ExternalADC::ExternalADC(int8_t device_address, int8_t bus_number) : I2C( bus_number, device_address) {} 

ExternalADC::~ExternalADC(){}

void ExternalADC::read_adc()
{   
    uint16_t adc_counts = 0;
    bit_15_8 = 0b11110011;
    bit_7_0 = 0b10000101;
    config_register = 1;
    convers_register = 0;
    write_buffer(config_register, bit_15_8, bit_7_0);
    if(isConversed())
    {
       read_buffer(convers_register, &adc_counts); 
    }
    printf("Values: HEX 0x%02x\n",adc_counts);
}

bool ExternalADC::isConversed()
{
    uint16_t data = 0;
    do{
        read_buffer(config_register,&data);
        printf("Data: HEX 0x%04x\n\n",data);
    } while((data & 0x8000) == 0);
    return true;
}