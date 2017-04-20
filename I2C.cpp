// I2C.cpp
#include "I2C.h"



#include <cerrno>
#include <cstring>

I2C::I2C(){}
I2C::I2C(int8_t bus_number, int8_t device_address)
{
    I2C::bus_number = bus_number;
    I2C::device_address = device_address;
}
I2C::~I2C(){ close(bus); }

void I2C::open_bus()
{   const char *bus_name = "/dev/i2c-0";// bus_n.c_str();
    if ((bus = open(bus_name, O_RDWR)) < 0) {
        printf("file %s and bus_number  %i\n",bus_name,bus_number);
        int errvsv = errno;
        printf("Failed to open the i2c bus of ADC and return errno %s\n",strerror(errvsv));
        exit(1);
    }
}

void I2C::set_bus_number( int8_t bus_number ) { I2C::bus_number = bus_number;}

void I2C::get_bus_access()
{
    if (ioctl(bus, I2C_SLAVE, device_address) < 0) {
        int errvsv = errno;
        printf("Failed to acquire bus access and/or talk to slave and ioctl returned errno %s .\n",strerror(errvsv));
        exit(1);
    }
}

void I2C::set_device_address(int8_t device_address) {I2C::device_address = device_address;}

void I2C::access_device()
{
    open_bus();
    get_bus_access();
}

void I2C::write_buffer(uint8_t register_address, uint16_t data){}
void I2C::write_buffer(uint8_t register_address, uint8_t bit_15_8, uint8_t bit_7_0)
{
    uint8_t buffer[3] = {register_address, bit_15_8, bit_7_0};
    printf("bufferr[0]    %i\nbufferr[1]    %i\nbufferr[2]    %i\n",buffer[0],buffer[1],buffer[2]);
    if (write(bus, buffer, 3) != 3) {
        perror("Write to register");
        exit(-1);
    }
}

void I2C::read_buffer(uint8_t register_address, uint16_t* data)
{
    uint8_t buffer[3] = {register_address, 0, 0};
    if (write(bus, buffer, 1) != 1) {
        perror("Write register select");
        exit(-1);
    }
    
    if (read(bus, buffer, 2) != 2) {
        perror("Read conversion");
        exit(-1);
    }
    printf("buffer[0]  %li    HEX 0x%02x \n",buffer[0],buffer[0]);
    printf("buffer[1]  %li    HEX 0x%02x \n",buffer[1],buffer[1]);
    
    *data = buffer[0] << 8 | buffer[1];
    printf("data  %li    HEX 0x%04x \n",*data,*data);
    
    if( *data<0 ) data = 0;
    printf("data2  %li    HEX 0x%04x \n",*data,*data);
  
    
}