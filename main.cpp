#include "I2C.h"
#include "ExternalADC.h"
#include <iostream> 
#include "Fpga.h"
#include "CurrentMonitor.h"



#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"

int main(int argc, char *argv[])
{
//    ExternalADC *adc1115 = new ExternalADC(0b01001000,0b11110011,0b11110011);
/*ExternalADC *adc1115 = new ExternalADC(0b01001000,1);
adc1115->access_device();
uint16_t adc_counts = 0;
double v = 0;
adc1115->read_adc(&adc_counts);
adc1115->inVolts(&adc_counts, &v);
*/
/*
uint32_t data = 0;
printf("DATA 0:  0x%08x\n",data);
Fpga *fpga = new Fpga();
fpga->read_fpga(0x00010040,&data);
uint32_t direction = 0x0f000000;
fpga->write_fpga((uint32_t)ALT_GPIO1_SWPORTA_DDR_ADDR,direction);

fpga->write_fpga((uint32_t)ALT_GPIO1_SWPORTA_DR_ADDR,direction);
printf("DATA 1:  0x%08x\n",data);
*/
/*
uint8_t dataW = strtoul( argv[ 1 ], NULL, 0 );
uint8_t register_add = strtoul( argv[ 2 ], NULL, 0 );
uint8_t r_w = 0;
r_w = strtoul( argv[ 3 ], NULL, 0 );

I2C *salt = new I2C(1,0x50);
salt->access_device();

uint8_t data = 0;
if(r_w==0){
salt->read_buffer(register_add, &data);

printf("DATA before write:  0x%02x in 0x%02x\n",data,register_add);



//uint16_t dataW = 0x05;
salt->write_buffer(register_add, dataW);

salt->read_buffer(register_add, &data);
printf("DATA after write:  0x%02x in 0x%02x\n",data,register_add);
}
if(r_w==1){
salt->read_buffer(register_add, &data);
printf("DATA:  0x%02x in 0x%02x\n",data,register_add);

}
if(r_w==2){
    salt->write_buffer(register_add, dataW);
}
*/

uint16_t cur_counts = 0;
CurrentMonitor *cur1 = new CurrentMonitor(1,0b01000000);
cur1->access_device();
cur1->configure();
cur1->read_current(&cur_counts);

CurrentMonitor *cur2 = new CurrentMonitor(1,0b01000001);
cur2->access_device();
cur2->configure();
cur2->read_current(&cur_counts);


    return 0;
}
