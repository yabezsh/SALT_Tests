#include "I2C.h"
#include "ExternalADC.h"
#include <iostream> 
#include "Fpga.h"
#include "CurrentMonitor.h"
#include "Salt.h"
#include "fastComm.h"
#include "Dig_Clk_test.h"
#include "Ana_tests.h"
#include <time.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include <cstring>
#include <vector>
#include <fstream>

int main(int argc, char *argv[])
{
 clock_t start_bp;
clock_t end_bp;

	clock_t start;
	clock_t finish;

	start = clock();

	ofstream outfile;
	outfile.open("RunLog.txt");
	uint16_t cur_counts_a = 0;
    int bus_voltage_a = 0;
    uint16_t cur_counts_d = 0;
    int bus_voltage_d = 0;
    float amp_a = 0;
    float amp_d = 0;
 CurrentMonitor *cur1 = new CurrentMonitor(2,0x41);
 CurrentMonitor *cur2 = new CurrentMonitor(0,0x40);


	// initial definitions
	ExternalADC *adc1115 = new ExternalADC(0x49,2);
	adc1115->access_device();
//	uint16_t adc_counts = 0;
//	double v = 0;
//	adc1115->read_adc(&adc_counts);
//	adc1115->inVolts(&adc_counts, &v);
	Fpga *fpga = new Fpga();
	Salt *st = new Salt(1,5);
	FastComm *fastComm = new FastComm(fpga);
	Dig_Clk_test *dig_com = new Dig_Clk_test(fpga,st,fastComm);
	Ana_tests *ana_func = new Ana_tests(fpga,st,fastComm);
	vector<string> arg;

  // soft reset of SALT
  //st->write_salt(0x601,(uint8_t) 1);
  //st->write_salt(0x600,(uint8_t) 1);
  
	if(argc == 1) {

		cout << "ERROR::MUST PROVIDE AN ARGUMENT!!!" << endl;
		cout << "Example: ./main i2c dll_pll fpga_daq_sync dsr_tfc_sync" << endl;
		return 0; 
	}
  
  for(int i=1; i < argc; i++)
    arg.push_back(argv[i]);

  for(int i=0; i < arg.size(); i++) {
    if( (arg.at(i) == "power") || (arg.at(i) == "all") ) {
//	    uint16_t adc_counts = 0;
//	    double v = 0;
//	    adc1115->read_adc(&adc_counts);
//	    adc1115->inVolts(&adc_counts, &v);
/*uint16_t cur_counts_a = 0;
    int bus_voltage_a = 0;
    uint16_t cur_counts_d = 0;
    int bus_voltage_d = 0;
    float amp_a = 0;
    float amp_d = 0;*/
   // CurrentMonitor *cur1 = new CurrentMonitor(2,0x41);
    cur1->access_device();
    
    cur1->set_config_bits(0b00011111,0b00000100);
    cur1->set_calib_bits(0b00000000,0b00100000);
    
    cur1->define_setup();
    cur1->read_current(&cur_counts_d);
    cur1->read_BusVoltage_mV(&bus_voltage_d);
    cur1->convert_to_amp(&cur_counts_d,&amp_d);
    
   // printf("Digi: HEX 0x%02x\n  or  %f mA \n",cur_counts, amp);
    cout << "Digital Power Consumption:" << endl; 
    cout << "Current[mA] = " << dec << amp_d << endl;
    cout << "Voltage[mV] = " << dec << bus_voltage_d << endl;

    //CurrentMonitor *cur2 = new CurrentMonitor(0,0x40);
    cur2->access_device();
    cur2->set_config_bits(0b00011111,0b00000100);
    cur2->set_calib_bits(0b00000000,0b00100000);
    
    cur2->define_setup();
    cur2->read_current(&cur_counts_a);
    cur2->convert_to_amp(&cur_counts_a,&amp_a);
    cur2->read_BusVoltage_mV(&bus_voltage_a);

    //printf("Monitor 2: HEX 0x%02x\n  or  %f mA \n",cur_counts, amp);
    cout << "Analogue Power Consumption:" << endl; 
    cout << "Current[mA] = " << dec << amp_a << endl;
    cout << "Voltage[mV] = " << dec << bus_voltage_a << endl;

    cout << "Total Power Consumption:" << endl;
    cout << "Current[mA] = " << dec << amp_a+amp_d << endl;
    cout << "Voltage[mV] = " << dec << (bus_voltage_a+bus_voltage_d)/2 << endl;

    outfile << "Digital Power Consumption:" << endl; 
    outfile << "Current[mA] = " << dec << amp_d << endl;
    outfile << "Voltage[mV] = " << dec << bus_voltage_d << endl;


    outfile << "Analogue Power Consumption:" << endl; 
    outfile << "Current[mA] = " << dec << amp_a << endl;
    outfile << "Voltage[mV] = " << dec << bus_voltage_a << endl;

    outfile << "Total Power Consumption:" << endl;
    outfile << "Current[mA] = " << dec << amp_a+amp_d << endl;
    outfile << "Voltage[mV] = " << dec << (bus_voltage_a+bus_voltage_d)/2 << endl;


    if((amp_a+amp_d) < 500 ) {

	cout << "SUCCESS!" << endl << "PASSED!" << endl;
	outfile << "POWER CONSUMPTION: OK" << endl;
    }
    else {

	    cout << "FAIL" << endl << "FAILED" << endl;
	outfile << "POWER CONSUMPTION: FAIL" << endl;	    

    }
	

    }
	  
	  if( (arg.at(i)== "i2c") || (arg.at(i) == "all")) {
      cout << "I2C check:" << endl;
      if(dig_com->I2C_check()) {

      	      cout << "SUCCESS!" << endl << "PASSED!" << endl;
	      outfile << "I2C: OK" << endl;
      }
	else
	{

	  cout << "FAIL" << endl << "FAILED" << endl;
	 outfile << "I2C: FAIL" << endl;
	  exit(-1);
	}
    }
    
    if( (arg.at(i)== "dll_pll") || (arg.at(i) == "all")) {
      cout << "DLL and PLL configuration:" << endl;
      if(dig_com->DLL_Check() && dig_com->PLL_Check()) {
	
	      cout << "SUCCESS!" << endl << "PASSED!" << endl;
		outfile << "DLL/PLL CONFIG: OK" << endl;
      }
	else
	{
	  cout << "FAIL" << endl << "FAILED" << endl;
	 outfile << "DLL/PLL CONFIG: FAIL" << endl;
	  // exit(-1);
	}
    }
    
    if( (arg.at(i) == "fpga_daq_sync") || (arg.at(i) == "all")) {
      cout << "FPGA-DAQ sync:" << endl;
	bool pass_bp = false;
	start_bp = clock();
	while( (clock() - start_bp)/CLOCKS_PER_SEC < 5) {
      		if(dig_com->DAQ_Sync()) {
			pass_bp = true;
			break;
		}
	}
	if(pass_bp) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
     outfile << "FPGA/DAQ SYNC: OK" << endl;
	}
	else {
	cout << "FAIL" << endl << "FAILED" << endl; 
	outfile << "FPGA/DAQ SYNC: FAIL" << endl;
	}
	} 
    if( (arg.at(i) == "dsr_tfc_sync") || (arg.at(i) == "all")) {
      // reset TFC
      dig_com->TFC_Reset();
      
      // Synch between DSR and TFC
      cout << "DSR and TFC synch:" << endl;
      if(dig_com->TFC_DAQ_sync()) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
      outfile << "DSR/TFC SYNC: OK" << endl;
      }
	else {
	cout << "FAIL" << endl << "FAILED" << endl;
	outfile << "DSR/TFC SYNC: FAIL" << endl;
	}
	}
    if( (arg.at(i) == "tfc_cmd") || (arg.at(i) == "all")) {
      cout << "TFC commands check:" << endl;
      if(dig_com->TFC_Command_Check()) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
      outfile << "TFC CMD CHECK: OK" << endl;
      }
	else {
	cout << "FAIL" << endl << "FAILED" << endl;
   outfile << "TFC CMD CHECK: FAIL" << endl;

	}       
	}
    // Do baseline corr
    if( (arg.at(i) == "baseline_corr") || (arg.at(i) == "all")) {
      cout << "Baseline corrections:" << endl;
      if(ana_func->Baseline_corr()) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
	ana_func->baseline_output();
      outfile << "BASELINE CORRECTIONS: OK" << endl;
      }
      else {
	cout << "FAIL" << endl << "FAILED" << endl;
    outfile << "BASELINE CORRECTIONS: FAIL" << endl;
      }
      }
    if( (arg.at(i) == "zs") || (arg.at(i) == "all")) {
      cout << "Zero supression:" << endl; 
      if(ana_func->Check_NZS()) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
      outfile << "ZERO SUPPRESSION CHECK: OK" << endl;
      }
	else {
	
		cout << "FAIL" << endl << "FAILED" << endl;
    outfile << "ZERO SUPPRESSION CHECK: FAIL" << endl;
	}
	}



    if( (arg.at(i) == "pedestal") || (arg.at(i) == "all")) {
      cout << "Pedestal substraction:" << endl; 
      if(ana_func->Check_PedS()) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
      outfile << "PEDESTAL SUBSTRACTION: OK" << endl;
      }
	else {
	cout << "FAIL" << endl << "FAILED" << endl;
    outfile << "PEDESTAL SUBTRACTION: FAIL" << endl;
	}
	}
    
    if( (arg.at(i) == "mcms") || (arg.at(i) == "all")) {
      cout << "Mean Common Mode Subtraction:" << endl;
      if(ana_func->Check_MCMS()) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
      outfile << "MCMS: OK" << endl;
      }
	else {
	cout << "FAIL" << endl << "FAILED" << endl;
    outfile << "MCMS: FAIL" << endl;
	}
	}
    
    // Get noise
    if( (arg.at(i) == "noise_run") || (arg.at(i) == "all")) {
      cout << "Noise MCMS run:" << endl;
      if(ana_func->Get_noise(100,"MCMS","NZS")) {
	cout << "SUCCESS!" << endl << "PASSED!" << endl;
	outfile << "NOISE CHECK: OK" << endl;
	ana_func->adc_output(-32,64);
      }
      else {
	cout << "FAIL" << endl << "FAILED" << endl;
    outfile << "NOISE CHECK: FAIL" << endl;
      }
      }
    if((arg.at(i) == "calib_fifo") || (arg.at(i) == "all")) {
	    cout << "CALIB FIFO and ADC clk delay:" << endl;	
	    if(ana_func->set_calib_fifo()) {
		    cout << "SUCCESS!" << endl << "PASSED!" << endl;
		outfile << "CALIB FIFO/ADC CLK DELAY: OK" << endl;
	    }
	    else {
		    outfile << "CALIB FIFO/ADC CLK DELAY: FAIL" << endl;
		    cout << "FAIL" << endl << "FAILED" << endl;
    
		    
	    }
	    }

    if( (arg.at(i) == "gain") || (arg.at(i) == "all")) {
	    cout << "Gain test:" << endl;
	    if(ana_func->Check_Gain()) {
		    cout << "SUCCESS!" << endl << "PASSED!" << endl;
		    ana_func->gain_output();
	    	outfile << "GAIN CHECK: OK" << endl;
	    }
	    else {
		    cout << "FAIL" << endl << "FAILED" << endl;

    outfile << "GAIN CHECK: FAIL" << endl;
	    }
	    }

    if ((arg.at(i) == "xtalk") || (arg.at(i) == "all") ) {
	    if(ana_func->xtalk_test()) {
		    cout << "SUCCESS!" << endl << "PASSED!" << endl;
		    ana_func->xtalk_output();
		    outfile << "CROSSTALK CHECK: OK" << endl;
	    }
	    else {
		    cout << "FAIL" << endl << "FAILED" << endl;

	    outfile << "CROSSTALK CHECK: FAIL" << endl;
	    }
	    }

  if(arg.at(i) == "all") {
//CurrentMonitor *cur1 = new CurrentMonitor(2,0x41);
  //  cur1->access_device();
    
   // cur1->set_config_bits(0b00011111,0b00000100);
   // cur1->set_calib_bits(0b00000000,0b00100000);
    
    //cur1->define_setup();
    cur1->read_current(&cur_counts_d);
    cur1->read_BusVoltage_mV(&bus_voltage_d);
    cur1->convert_to_amp(&cur_counts_d,&amp_d);
    
   // printf("Digi: HEX 0x%02x\n  or  %f mA \n",cur_counts, amp);
    cout << "Digital Power Consumption (FINAL):" << endl; 
    cout << "Current[mA] = " << dec << amp_d << endl;
    cout << "Voltage[mV] = " << dec << bus_voltage_d << endl;

    //CurrentMonitor *cur2 = new CurrentMonitor(0,0x40);
    //cur2->access_device();
    //cur2->set_config_bits(0b00011111,0b00000100);
    //cur2->set_calib_bits(0b00000000,0b00100000);
    
    //cur2->define_setup();
    cur2->read_current(&cur_counts_a);
    cur2->convert_to_amp(&cur_counts_a,&amp_a);
    cur2->read_BusVoltage_mV(&bus_voltage_a);

cout << "Analogue Power Consumption (FINAL):" << endl; 
    cout << "Current[mA] = " << dec << amp_a << endl;
    cout << "Voltage[mV] = " << dec << bus_voltage_a << endl;

    cout << "Total Power Consumption (FINAL):" << endl;
    cout << "Current[mA] = " << dec << amp_a+amp_d << endl;
    cout << "Voltage[mV] = " << dec << (bus_voltage_a+bus_voltage_d)/2 << endl;


 outfile << "Digital Power Consumption (FINAL):" << endl; 
    outfile << "Current[mA] = " << dec << amp_d << endl;
    outfile << "Voltage[mV] = " << dec << bus_voltage_d << endl;


    outfile << "Analogue Power Consumption (FINAL):" << endl; 
    outfile << "Current[mA] = " << dec << amp_a << endl;
    outfile << "Voltage[mV] = " << dec << bus_voltage_a << endl;

    outfile << "Total Power Consumption (FINAL):" << endl;
    outfile << "Current[mA] = " << dec << amp_a+amp_d << endl;
    outfile << "Voltage[mV] = " << dec << (bus_voltage_a+bus_voltage_d)/2 << endl;



  }


if(arg.at(i) == "reset_fpga") {

fpga->write_fpga(registers::RESET, (uint8_t) 0x01);
fpga->write_fpga(registers::RESET, (uint8_t) 0x00);

}

if(arg.at(i) == "reset" ) {
fpga->write_fpga(registers::RESET, (uint8_t) 0x03);
fpga->write_fpga(registers::I2C_ADD, (uint8_t) 0x05);
}
}
    finish = clock();

    cout << "Total time = " << (float) (finish-start)/CLOCKS_PER_SEC << " seconds" << endl;

   return 0;
  
}
