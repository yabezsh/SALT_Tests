#include "I2C.h"
#include "ExternalADC.h"
#include <iostream> 
int main()
{
//    ExternalADC *adc1115 = new ExternalADC(0b01001000,0b11110011,0b11110011);
ExternalADC *adc1115 = new ExternalADC(0b01001000,1);
adc1115->access_device();
uint16_t adc_counts = 0;
double v = 0;
adc1115->read_adc(&adc_counts);
adc1115->inVolts(&adc_counts, &v);

    return 0;
}
