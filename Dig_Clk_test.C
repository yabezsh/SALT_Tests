#include <time.h>
#include "Dig_Clk_test.h"
#include "Salt.h"
#include <unistd.h>
#include "Fpga.h"
#include "registers_config.h"
#include "fastComm.h"
#include <iostream>
#include <ctime>
#include <fstream>

using namespace std;

// CONSTRUCTOR
Dig_Clk_test::Dig_Clk_test(Fpga *fpga, Salt *salt, FastComm *fastComm) {
   fpga_=fpga;
  salt_=salt;
  fastComm_=fastComm;
  
}

// Synch output of DAQ to clock
bool Dig_Clk_test::DAQ_Sync() {

  uint32_t data[5120];
  const int length=100;
  int e[8][8] = {0};
  int bs_p[2];
  bool found_opt = false;
  bool tfc_trig = false; // Do not set tfc trigger for pattern readout
  
  // DAQ Reset & Clear FIFO
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x11);
  usleep(100);
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);

  // configure DAQ  
  fastComm_->config_daq(length, 0, tfc_trig);
  
  // Set correct pattern
  salt_->write_salt(registers::pattern_cfg, (uint8_t) 0xAB); // Set pattern for synch, in this case hAB
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x22); // Reset ser_source_cfg (count up, pattern register output)

  // Loop over phases
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      for(int k=0; k<10; k++) {

	fastComm_->read_daq(length,data,tfc_trig);

	e[i][j]+=Check_Ber(data,length,0xAB);   

	if(k==9)
	  
	if(k==9 && (e[i][j]==0)) {
	  
	  bs_p[0] = i;
	  bs_p[1] = j;
	 	 
	  found_opt = true;
	  
	  break;
	  
	}
      }
      if(found_opt) break;
      FPGA_PLL_shift(1);
    }
    
    if(found_opt) break;
    
    //bit slip
    fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x02);
    usleep(100);
    fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);
    usleep(100);
    FPGA_PLL_shift(-8);
    
  }
   
  if(!found_opt) cout << "CLK synch failed: Could not find optimal bit slip/phase" << endl;
  else cout << "CLK synch finished. Optimal values = " <<bs_p[0]  << ", " << bs_p[1] << endl; 

  return found_opt;
  
}


void Dig_Clk_test::FPGA_PLL_shift(int16_t phase) {

  if(phase>0)  fpga_->write_fpga(registers::PLL_DFS_3,(uint8_t) 0x21);
  else fpga_->write_fpga(registers::PLL_DFS_3,(uint8_t) 0x01); //UP

  fpga_->write_fpga(registers::PLL_DFS_1,(uint8_t) abs(phase));
  
  fpga_->write_fpga(0x00040008,(uint8_t) 0x01);

}

// check bit error
int Dig_Clk_test::Check_Ber(uint32_t *packet, int length, uint8_t pattern) {

  int error =0;
  uint8_t e1, e2, e3;

  for (int k=0; k<length; k++) {
    
    e1=(packet[k] & 0x000000FF);
    
    e2=(packet[k] & 0x0000FF00) >> 8;
    
    e3=(packet[k] & 0x00FF0000) >> 16;
    
    
    if((e1!=pattern) || (e2!=pattern) || (e3!=pattern)) {

      error++;

    }
    
        
  } 
  
  return error;
  
}

// DLL configuration as outlined in the SALT manual
bool Dig_Clk_test::DLL_Check() {
  
  uint8_t data=0xFF;
  uint8_t command=0xFF;
  uint8_t read=0;
  
  salt_->read_salt(registers::dll_vcdl_cfg, &read);
  if(read != 0x60) 
    salt_->write_salt(registers::dll_vcdl_cfg,(uint8_t) 0x60);
  usleep(1);
  
  salt_->read_salt(registers::dll_vcdl_mon, &read);
  
  while((read & 0x80) != 0x00) {
    
    salt_->read_salt(registers::dll_vcdl_cfg, &read);
    read++;
    salt_->write_salt(registers::dll_vcdl_cfg,read);
    //salt_->read_salt(registers::dll_vcdl_cfg, &read);
    salt_->read_salt(registers::dll_vcdl_mon, &read);
  }
  
  salt_->read_salt(registers::dll_vcdl_cfg, &read);
  read=read-1;
  salt_->write_salt(registers::dll_vcdl_cfg,read);
  
  usleep(1);
  
  salt_->read_salt(registers::dll_vcdl_mon, &read);
  
  while((read & 0x80) == 0x00) {
    salt_->read_salt(registers::dll_vcdl_cfg, &read);
    read=read-1;
    salt_->write_salt(registers::dll_vcdl_cfg,read);
    
    usleep(1);
    
    salt_->read_salt(registers::dll_vcdl_mon, &read);
  }
  
  
  return true;
  
}

// PLL configuration as outlined in SALT manual
bool Dig_Clk_test::PLL_Check() {

  uint8_t data = 0x00;
  uint8_t command = 0xFF;

  // Make sure PLL enabled and configured
  salt_->read_salt(registers::pll_main_cfg, &data);

  if(data != 0x8C)
    command=0x8C;
  salt_->write_salt(registers::pll_main_cfg, command);
  command=0xCC;
  salt_->write_salt(registers::pll_main_cfg, command);

  return true;    
    
}


bool Dig_Clk_test::I2C_check() {

  uint8_t data = 0;

  // Configure PLL
  salt_->write_salt(registers::pll_main_cfg, (uint8_t) 0x8C );
  usleep(10);
  salt_->write_salt(registers::pll_vco_cfg, (uint8_t) 0x12 );
  usleep(10);
  salt_->write_salt(registers::pll_main_cfg, (uint8_t) 0xCC );
  usleep(10);
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x22 );
  usleep(10);
  salt_->write_salt(registers::pattern_cfg, (uint8_t) 0xF0 );

  // Check that I2C can Read/Write random patters
  uint8_t x;
  srand(time(NULL)); // seed random number generator
  
  clock_t begin = clock();
  for(int i=0; i<10000; i++) {
    
    x=randomPattern();
    salt_->write_salt(registers::pattern_cfg, x);
    salt_->read_salt(registers::pattern_cfg, &data);
    if(data!=x) return false;

  }
  
  clock_t end = clock();
  double elapsed_sec = double(end - begin) / CLOCKS_PER_SEC;
  return true;

}


// sync between TFC and chip
bool Dig_Clk_test::TFC_DAQ_sync() {
 
  uint8_t length = 3;
  uint8_t command[max_commands]={0};
  uint8_t length_read = 100; // number of clock cycles to read
  uint32_t data[5120]; // data packet
  int period = 3;
   // define single or continuous transmission
  bool singleShot = false;
  bool rightConfig = false;

  int e[256][32] = {0};
  int ibest=0;
  int jbest=0;

  // Set DAQ delay to 0
  fpga_->write_fpga(registers::DAQ_DELAY, (uint8_t) 0x00);
  // set DAQ to DSP out data
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x21);
  for(int k=0; k<256; k++) {

      
    command[0]=k;
    command[1]=command[0];
    command[2]=command[0];

    // loop over pll_clk_cfg values
    for(int i=0; i<16; i++) {
      
      salt_->write_salt(registers::pll_clk_cfg, (uint8_t) (16*i));
      
      // loop over data_clk_sel+deser_byte_start
      for(int j=0; j<32; j++) {
	
	if(e[i][j]!=0) continue;
	salt_->write_salt(registers::deser_cfg, (uint8_t) j);

	fastComm_->Take_a_run(length_read, data, length, 0, command, period, singleShot, false );

	e[i][j]+=Check_Ber(data,length_read,command[0]);
	  
	if(k==255 && e[i][j]==0) {
	  cout << "pll_clk_cfg = " << hex << (unsigned) 16*i << endl;
	  cout << "deser_cfg = " << hex << (unsigned) j << endl;
	  rightConfig=true;
	  ibest=i;
	  jbest=j;
	  break;
	}
	
      }
            if(rightConfig) break;
    }
  }
  // if cann't get right pll config, fail

  salt_->write_salt(registers::pll_clk_cfg, (uint8_t) (16*ibest));
  salt_->write_salt(registers::deser_cfg, (uint8_t) (jbest));
  fastComm_->reset_DAQ();
  return rightConfig;
  
}

bool Dig_Clk_test::DAQ_Delay() {

  uint8_t length = 3;
  // Define command list (BXID and Sync)
  uint8_t command[max_commands]={0};
  // Read out data packet
  uint8_t length_read = 100; // number of clock cycles to read
  uint32_t data[5120]; // data packet
  uint8_t data8=0;
  int period = 100;
  // define single continuous transmission
  bool singleShot = true;
  bool rightConfig = false;
  
  command[0]=0x00;
  command[1]=0xAB;
  command[2]=0x00;
  
  
  
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x21);
  usleep(1000);
  
  // for(int i=0; i<256; i++) {
  
  
  //  for(int k=0; k<10; k++) {
  salt_->write_salt(registers::tfc_fifo_cfg, (uint8_t) 20);
  
  fastComm_->config_daq(length_read, (uint8_t) 0, true);
  fastComm_->config_tfc(length, command, period, singleShot);
  fastComm_->write_tfc();
  
  fastComm_->read_daq(length_read,data,true);
  
  
  
  salt_->read_salt(registers::pll_clk_cfg, &data8);
  cout << "pll_clk_cfg = " << hex<< (unsigned) data8 << endl;
  salt_->read_salt(registers::deser_cfg, &data8);
  cout << "deser_cfg = " << hex << (unsigned) data8 << endl;
  
  
  if(Check_Ber(data,length_read,0xAB)==0) {
    cout << "DAQ_DELAY = " << dec<< (unsigned) 0 << endl;
    fpga_->write_fpga(registers::DAQ_DELAY, (uint8_t) 0);
    return true;
    
    
  }

  command[0]=0x80;
  command[1]=command[0];
  command[2]=command[0];
  for(int i=0; i<256; i++) {

    salt_->write_salt(registers::calib_fifo_cfg, (uint8_t) i);

    
  }
    return rightConfig;
}
  
uint8_t Dig_Clk_test::randomPattern() {

 uint8_t x;
  srand(time(NULL)); // seed random number generator

  x = rand() & 0xFF;
  x |= (rand() & 0xFF) << 8;
  x |= (rand() & 0xFF) << 16;
  x |= (rand() & 0xFF) << 24;

    return x;
  
}

void Dig_Clk_test::unmask_all() {

  salt_->write_salt(registers::mask0_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask1_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask2_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask3_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask4_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask5_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask6_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask7_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask8_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask9_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask10_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask11_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask12_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask13_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask14_cfg,(uint8_t) 0x00);
  salt_->write_salt(registers::mask15_cfg,(uint8_t) 0x00);
}


bool Dig_Clk_test::TFC_Command_Check() {
  
  uint8_t command[max_commands]={0};
  uint8_t length = 90;
  uint16_t length_read = 150;
  string data_string;
  bool singleShot = true;
  int period = length;
  int flag = 0;
  int length1 = 0;
  int bxid = 0;
  int parity = 0;
  uint8_t buffer;
  uint8_t buffer2;
  unsigned twelveBits;
  int counter = 0;
  unmask_all();

  // DSP output
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);

  // set synch pattern registers
  salt_->write_salt(registers::sync1_cfg,(uint8_t) 0x8C);
  salt_->write_salt(registers::sync0_cfg,(uint8_t) 0xAB);

 
  // check TFC commands
  string option[8];
  bool pass[8] = {false};
  option[0] = "Normal";
  option[1] = "BXReset";
  option[2] = "Header";
  option[3] = "NZS";
  option[4] = "BxVeto";
  option[5] = "Normal";
  option[6] = "Snapshot";
  option[7] = "FEReset";
  
  for(int i=0; i<79; i++)
    command[i]=0x04;
  
  for(int i=0; i < 8; i++) {
    if(option[i] == "Normal")        command[79] = 0x00;
    else if(option[i] == "BXReset")  command[79] = 0x01;
    else if(option[i] == "FEReset")  command[79] = 0x02;
    else if(option[i] == "Header")   command[79] = 0x04;
    else if(option[i] == "NZS")      command[79] = 0x08;
    else if(option[i] == "BxVeto")   command[79] = 0x10;
    else if(option[i] == "Snapshot") command[79] = 0x20;
    else if(option[i] == "Synch")    command[79] = 0x40;

 
    
    fastComm_->Take_a_run(length_read, data_string, length, 0, command, period, singleShot, true );

    for(int j=0; j<data_string.length(); j+=3) {
     
      twelveBits = fastComm_->read_twelveBits(data_string, j);
      if(twelveBits == 0x0F0) continue;
      fastComm_->read_Header(twelveBits, bxid, parity, flag, length1);
      
      if(flag == 0) {
	if(option[i] == "Normal") pass[i] = true;
	if(option[i] == "BXReset" && bxid == 0) pass[i] = true; 
	
      }
      if(flag == 1) {
	if(length1 == 0x11 && option[i] == "BxVeto") pass[i] = true;
	if(length1 == 0x12 && option[i] == "Header") pass[i] = true;
	if(length1 == 0x06 && option[i] == "NZS") pass[i] = true;
	
      }
      if(option[i] == "FEReset") {
	salt_->read_salt(registers::header_cnt0_reg, &buffer);
	if(buffer == 0x00) pass[i] = true;
	
      }
      if(option[i] == "Synch") {
       
	//salt_->read_salt(registers::sync1_cfg, &buffer);
	salt_->read_salt(registers::sync0_cfg, &buffer);
	
	twelveBits = fastComm_->read_twelveBits(data_string, j+3);
	

	if((buffer & 0xFF) == (twelveBits & 0xFF))
	  pass[i] = true;
	
      }
      if(option[i] == "Snapshot") {

	salt_->read_salt(registers::header_cnt0_snap_reg, &buffer);
	//salt_->read_salt(registers::bxid_cnt0_sn
	//cout << hex << (unsigned) twelveBits << endl;
	//cout << "snap is : " << hex << (unsigned) buffer << endl;

	//	cout << "BUFFER " << hex << (unsigned) buffer << endl;
	//cout << "bxid " << hex << (unsigned) bxid << endl;
	
	if(buffer == 79)
	  pass[i] = true;
      }
      
    }

    if(!pass[i]) {
      cout << "ERROR::TFC " << option[i] << " fails" << endl;
      counter++;
    }
    
  }
  
  if(counter>0) return false;
  
  return true;
  
}

void Dig_Clk_test::TFC_Reset() {

  // Reset TFC state machine and set all related registers to default values
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x01);
  usleep(1000);
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x00);
  
}
