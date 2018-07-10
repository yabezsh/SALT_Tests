#include "I2C.h"
#include "ExternalADC.h"
#include <iostream> 
#include "Fpga.h"
#include "CurrentMonitor.h"
#include "Salt.h"
#include "fastComm.h"
#include "Dig_Clk_test.h"
#include "Ana_tests.h"

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



//Example how to define different configuration
// but this config is default one as well
/*
uint16_t cur_counts = 0;
float amp = 0;
CurrentMonitor *cur1 = new CurrentMonitor(1,0b01000000);
cur1->access_device();

cur1->set_config_bits(0b00011111,0b00000100);
cur1->set_calib_bits(0b00000000,0b00100000);

cur1->define_setup();
cur1->read_current(&cur_counts);
cur1->convert_to_amp(&cur_counts,&amp);

printf("Monitor 1: HEX 0x%02x\n  or  %f mA \n",cur_counts, amp);

CurrentMonitor *cur2 = new CurrentMonitor(1,0b01000001);
cur2->access_device();
cur2->set_config_bits(0b00011111,0b00000100);
cur2->set_calib_bits(0b00000000,0b00100000);
 
cur2->define_setup();
cur2->read_current(&cur_counts);
cur1->convert_to_amp(&cur_counts,&amp);

printf("Monitor 2: HEX 0x%02x\n  or  %f mA \n",cur_counts, amp);
*/
// test digital c

  
 cout << "Defining FPGA" << endl;
 Fpga *fpga = new Fpga();
 cout << "SALT" << endl;
 Salt *st = new Salt(1,5);
 cout << "Defining Fast comm" << endl;
 FastComm *fastComm = new FastComm(fpga);
 cout << "Defining Dig comm" << endl;
 Dig_Clk_test *dig_com = new Dig_Clk_test(fpga,st,fastComm);

 Ana_tests *ana_func = new Ana_tests(fpga,st,fastComm);

 // st->write_salt(0x601,(uint8_t) 0x01);
 //st->write_salt(0x600,(uint8_t) 0x01);
 //st->write_salt(0x604,(uint8_t) 0x01);
 // I2C test
 cout << "I2C test starting" << endl;
 if(dig_com->I2C_check())
   cout << "I2C check OK" << endl;
 else
   cout << "I2C check FAILED" << endl;
 /*
cout << "PLL configuration starting" << endl;
 if(dig_com->PLL_Check())
   cout << "PLL configuration OK" << endl;
 else
   cout << "PLL configuration FAILED" <<endl;
 
 cout << "DLL configuration starting" << endl;
 if(dig_com->DLL_Check())
   cout << "DLL configuration OK" << endl;
 else
   cout << "DLL configuration FAILED" <<endl;
 
 
cout << "PLL configuration starting" << endl;
 if(dig_com->PLL_Check())
   cout << "PLL configuration OK" << endl;
 else
   cout << "PLL configuration FAILED" <<endl;
 */
 cout<< "Clk synch starting" << endl;
 dig_com->DAQ_Sync();
 
 //cout << "Clk synch finished" <<endl;
 
 cout << "TFC check starting" << endl;
 
 if(dig_com->TFC_check())
   cout << "TFC OK" << endl;
 else
   cout << "TFC failed" << endl;


 
 
 cout << "Baseline correction starting" << endl;
 ana_func->Baseline_corr();
 cout << "Baseline correction finished" << endl;


 ana_func->Check_NZS();
 
 ana_func->Check_noise();
 
//
 
  cout << "Checking Pedestal subtraction" << endl;
  if(ana_func->Check_PedS())
    cout << "Ped substraction OK" << endl;
  else
    cout << "Ped substraction failed" << endl;


  
  if(ana_func->Check_MCMS())
    cout << "MCM sub4traction OK" << endl;
  else
    cout << "MCM failed" << endl;
  
 //ana_func->Check_noise();
 
 //ana_func->Check_NZS();
 
 //cout << "Calibration run" << endl;
 //ana_func->Check_Gain();
 cout << "done " << endl;

 
/*
Salt *st = new Salt();
uint8_t data = 0;
st->read_salt(0b101,0, &data);
printf(" 0x%02x \n",data);
*/
    return 0;
}
