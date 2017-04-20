#include "I2C.h"
#include "ExternalADC.h"
#include <iostream> 
int main()
{
//    ExternalADC *adc1115 = new ExternalADC(0b01001000,0b11110011,0b11110011);
std::cout<<" 0"<<std::endl;
ExternalADC *adc1115 = new ExternalADC(0b01001000,1);
std::cout<<" 1"<<std::endl;
adc1115->access_device();
std::cout<<" 2"<<std::endl;
adc1115->read_adc();
    return 0;
}
