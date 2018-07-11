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
void Dig_Clk_test::DAQ_Sync() {
  bool all_elink = false;
  uint32_t data[5120];
  //else {uint8_t data[5120];};
  const int length=100;
  uint8_t length_read = 100;
  int e[8][8] = {0};
  int bs_p[2];
  bool found_opt = false;
  bool tfc_trig = false; // Do not set tfc trigger for pattern readout

  // DAQ Reset & Clear FIFO
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x11);
  usleep(100);
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);

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
	  // cout << "eij = " << dec << unsigned(e[i][j]) << endl;
	
	if(k==9 && (e[i][j]==0)) {
	  
	  bs_p[0] = i;
	  bs_p[1] = j;
	  
	  
	  cout << "FPGA Clock Synch finished" << endl;
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
    // fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x10);
    FPGA_PLL_shift(-8);
    
  }
  
   
  if(!found_opt) cout << "CLK synch failed: Could not find optimal bit slip/phase" << endl;
  else cout << "CLK synch finished. Optimal values = " <<bs_p[0]  << ", " << bs_p[1] << endl; 
  
}


void Dig_Clk_test::FPGA_PLL_shift(int16_t phase) {

  if(phase>0)  fpga_->write_fpga(registers::PLL_DFS_3,(uint8_t) 0x21);
  else fpga_->write_fpga(registers::PLL_DFS_3,(uint8_t) 0x01); //UP

  fpga_->write_fpga(registers::PLL_DFS_1,(uint8_t) abs(phase));
  
  fpga_->write_fpga(0x00040008,(uint8_t) 0x01);

}
/*
int Dig_Clk_test::Check_Ber(uint8_t *packet, int length) {

  int error =0;
 
   for (int k=0; k<length-5; k+=5) {

    if(packet[k]!=packet[k+5]+1) error++;

  }
   
  return error;

}
*/
// check bit error
int Dig_Clk_test::Check_Ber(uint32_t *packet, int length, uint8_t pattern) {

  int error =0;
  uint8_t e1, e2, e3;

  for (int k=0; k<length; k++) {
    
    e1=(packet[k] & 0x000000FF);
    
    e2=(packet[k] & 0x0000FF00) >> 8;
    
    e3=(packet[k] & 0x00FF0000) >> 16;
    
    
    //  if(k==length-1) {
    // cout << "packet[]=" << hex << unsigned(packet[k]) << endl;
    
    // cout << "error = " << error << endl;
    // }
    
    if((e1!=pattern) || (e2!=pattern) || (e3!=pattern)) {
      // if(pattern != 0) cout << "pattern = " << hex << (unsigned) pattern << endl;
      //cout << "e1=" << hex << (unsigned) packet[k] << endl;
      error++;
      //else
    }
        if(error!=0 && error<10) {
    
	  //    cout << "packet[]=" << hex << unsigned(packet[k]) << endl; 
    
    // if(k==length-1) {
    
    //   cout << "error after= " << error << endl;
        }
    
  } 
  
  return error;
  
}

// DLL configuration as outlined in the SALT manual
bool Dig_Clk_test::DLL_Check() {
  
  uint8_t data=0xFF;
  uint8_t command=0xFF;
  uint8_t read=0;


  cout << "Setting dll_cp_cfg to default value: 0x9A" << endl;
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
  uint8_t command = 0xFF;

  // Configure PLL
  //cout << "I2C_check: Configuring PLL" << endl;
  // command = 0x0D;
  // salt_->write_salt(registers::pll_main_cfg, command);
  //cout << "I2C_check: PLL configured" << endl;
 salt_->write_salt(registers::pll_main_cfg, (uint8_t) 0x8C );
   usleep(1000);
   salt_->write_salt(registers::pll_vco_cfg, (uint8_t) 0x12 );
   usleep(1000);
  salt_->write_salt(registers::pll_main_cfg, (uint8_t) 0xCC );
   usleep(1000);
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x22 );
  usleep(1000);
   salt_->write_salt(registers::pattern_cfg, (uint8_t) 0xF0 );
  // Check that I2C can Read/Write random patters
  uint8_t x;
  srand(time(NULL)); // seed random number generator

  clock_t begin = clock();
   for(int i=0; i<10000; i++) {

     x=randomPattern();

   
    //cout << "Writing to pattern register: "  << hex << unsigned(x) << endl;
    
    salt_->write_salt(registers::pattern_cfg, x);

    //cout << "Wrote to pattern register" << endl;
    salt_->read_salt(registers::pattern_cfg, &data);
    //cout << "Reading from pattern register: " << hex<<unsigned(data) << endl;
    if(data!=x) return false;

  }

   clock_t end = clock();
   double elapsed_sec = double(end - begin) / CLOCKS_PER_SEC;
   cout << "I2C time: " << elapsed_sec << " seconds" << endl;
  return true;

}

bool Dig_Clk_test::TFC_check() {

  // Define command length (will be 2 in this case)
  uint8_t length = 6;
  // Define command list (BXID and Sync)
  uint8_t command[max_commands]={0};
  // Read out data packet
  uint16_t length_read = 115; // number of clock cycles to read
  uint32_t data[5120]; // data packet
  uint8_t delay = 0; // clock delay
  int trigger = 1; // trigger aquisition
  int period = length;
  uint8_t data8 = 0;
  
  // Reset TFC state machine and set all related registers to default values
  cout << "Reset state machine and set all registers to default values" << endl;
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x01);
  usleep(1000);
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x00);
  cout << "Reset complete" << endl;

  cout << "about to do TFC Chip sync" << endl;
  // check TFC-DAQ clk sync
  if(!TFC_DAQ_sync()) cout << "TFC Chip sync bad" << endl;//return false;
  
   else cout << "TFC Chip sync OK" << endl;

  //unmask_all();

      
   return false;
   
   
   cout << "TFC checks finished" << endl;
   return true;
   
}

// sync between TFC and chip
bool Dig_Clk_test::TFC_DAQ_sync() {
 

 // Define command length (will be 2 in this case)
  uint8_t length = 3;
  // Define command list (BXID and Sync)
  uint8_t command[max_commands]={0};
  // Read out data packet
  uint8_t length_read = 100; // number of clock cycles to read
  uint32_t data[5120]; // data packet
  uint8_t delay = 0; // clock delay
  int trigger = 1; // trigger aquisition
  int period = 3;
  uint8_t data8 = 0;
   // define single continuous transmission
  bool singleShot = false;
  bool rightConfig = false;
  int error=0;
  int e[256][32] = {0};
  int ibest=0;
  int jbest=0;
  // uint8_t c[7];
  /*
  c[0]=0x01;
  c[1]=0x02;
  c[2]=0x03;
  c[4]=0x10;
  c[5]=0x20;
  c[6]=0xAB;
  */
  // Reset TFC state machine and set all related registers to default values
  cout << "Reset state machine and set all registers to default values" << endl;
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x01);
  usleep(1000);
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x00);
  cout << "Reset complete" << endl;

  // Set DAQ delay to 0
  fpga_->write_fpga(registers::DAQ_DELAY, (uint8_t) 0x00);
  // set DAQ to DSP out data
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x21);
  for(int k=0; k<256; k++) {
    // set TFC command to 0xAB
    error=0;
    //cout << "k is " << k << endl;
    //command[0]=c[k];
    //command[0]=randomPattern();
    command[0]=k;
    command[1]=command[0];
    command[2]=command[0];

    //    cout << "pattern1 = " << hex << unsigned(command[0]) << endl;
    // configure DAQ
    // fastComm_->config_daq(length_read, 0, false);
    // configure TFC
    //fastComm_->config_tfc(length, command, period, singleShot);

    //    fastComm_->write_tfc();
    //usleep(1000);
    //fastComm_->Take_a_run(length_read, data, length, 0, command, period, singleShot, false );
    // loop over pll_clk_cfg values
    for(int i=0; i<16; i++) {
      //cout << "i = " << i << endl;
      salt_->write_salt(registers::pll_clk_cfg, (uint8_t) (16*i));
      
      // loop over data_clk_sel+deser_byte_start
      for(int j=0; j<32; j++) {

	
	//cout << "j = " << j << endl;
	if(e[i][j]!=0) continue;
	salt_->write_salt(registers::deser_cfg, (uint8_t) j);
	//usleep(100);
	fastComm_->Take_a_run(length_read, data, length, 0, command, period, singleShot, false );
	//	fastComm_->read_daq(length_read,data,false);
	e[i][j]+=Check_Ber(data,length_read,command[0]);
	//if(k==255)
	//	cout << "k = " << k << endl;
	//cout << "command = " << command[0] << endl;
	//cout << "e[" << dec << i << "][" << dec << j << "] = " << e[i][j] << endl;
  
	if(k==255 && e[i][j]==0) {
	  cout << "pll_clk_cfg = " << hex << (unsigned) 16*i << endl;
	  cout << "deser_cfg = " << hex << (unsigned) j << endl;
	  rightConfig=true;
	  //break;
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
  
  cout << "About to set DAQ delay" << endl;
  
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
  
  
  for(int j=0; j<length_read; j++) 
    cout << "data1["<< dec << j << "]=" << hex << (unsigned) data[j] << endl;
  
  //}
  //  }
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
    //cout << "x = " << x << endl;
    x |= (rand() & 0xFF) << 8;
    //cout << "x = " << x << endl;
    x |= (rand() & 0xFF) << 16;
    //cout << "x = " << x << endl;
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
  unsigned twelveBits;
  unmask_all();
  
  // check TFC commands
  string option[8];
  bool pass[8] = {false};
  option[0] = "Normal";
  option[1] = "BXReset";
  option[2] = "Header";
  option[3] = "NZS";
  option[4] = "BxVeto";
  option[5] = "Snapshot";
  option[6] = "Synch";
  option[7] = "FEReset";
  
  
  for(int i=0; i<79; i++)
    command[i]=0x04;
  
  if(option[0] == "Normal")        command[79] = 0x00;
  else if(option[0] == "BXReset")  command[79] = 0x01;
  else if(option[0] == "FEReset")  command[79] = 0x02;
  else if(option[0] == "Header")   command[79] = 0x04;
  else if(option[0] == "NZS")      command[79] = 0x08;
  else if(option[0] == "BxVeto")   command[79] = 0x10;
  else if(option[0] == "Snapshot") command[79] = 0x20;
  else if(option[0] == "Synch")    command[79] = 0x40;
  
  for(int j=0; j<data_string.length(); j+=3) {
    twelveBits = fastComm_->read_twelveBits(data_string, j);
    fastComm_->read_Header(twelveBits, bxid, parity, flag, length1);
    
    if(flag == 0) {
      if(option[0] == "Normal") pass[0] = true;
      if(option[0] == "BXReset" && bxid == 0) pass[0] = true; 
      
    }
    if(flag == 1) {
      
      if(length1 == 0x11 && option[0] == "BxVeto") pass[0] = true;
      if(length1 == 0x12 && option[0] == "Header") pass[0] = true;
      if(length1 == 0x06 && option[0] == "NZS") pass[0] = true;
      
      
      // if(option[i] == "BXReset") {
      // 	 salt_->read_salt(registers::header_cnt0_snap_reg, &buffer);
      // 	 if(buffer == 0x00
      // }
    }
    
    if(option[0] == "FEReset") {
      salt_->read_salt(registers::header_cnt0_reg, &buffer);
      if(buffer == 0x00) pass[0] = true;
      
    }
    
  }

  int j=0;
  //for(int i = 0; i < 8) {
    
  //  if(!pass[0]) {
  //    cout << "ERROR::TFC " << option[0] << "fails" << endl;
  //    j++;
  //  }
  // }
  
  if(j>0) return false;
  
  return true;
  
}
