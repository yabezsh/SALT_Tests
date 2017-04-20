#include "I2C.h"
#include "ExternalADC.h"
#include <iostream> 
#include "Fpga.h"
int main()
{
//    ExternalADC *adc1115 = new ExternalADC(0b01001000,0b11110011,0b11110011);
/*ExternalADC *adc1115 = new ExternalADC(0b01001000,1);
adc1115->access_device();
uint16_t adc_counts = 0;
double v = 0;
adc1115->read_adc(&adc_counts);
adc1115->inVolts(&adc_counts, &v);
*/
uint32_t data = 0;
printf("DATA 0:  0x%08x\n",data);
Fpga *fpga = new Fpga();
fpga->read_fpga(0x00010040,&data);
printf("DATA 1:  0x%08x\n",data);
    return 0;
}
