#include <time.h>
#include "Dig_Clk_test.h"
#include "Salt.h"
#include <unistd.h>
#include "Fpga.h"
#include "registers_config.h"
#include "fastComm.h"
#include <iostream>
#include <ctime>
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
  int e[8][8] = {0};
  int bs_p[2];
  bool found_opt = false;


  // DAQ Reset & Clear FIFO
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x11);
  usleep(1);
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);

  // Set correct pattern
  salt_->write_salt(registers::pattern_cfg, (uint8_t) 0xAB); // Set pattern for synch, in this case hAB
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x22); // Reset ser_source_cfg (count up, pattern register output)

  // Loop over phases
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      for(int k=0; k<10; k++) {

	fastComm_->read_daq(0,length,1,data);

	e[i][j]+=Check_Ber(data,length);   

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
    fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);
    
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

int Dig_Clk_test::Check_Ber(uint8_t *packet, int length) {

  int error =0;
 
   for (int k=0; k<length-5; k+=5) {

    if(packet[k]!=packet[k+5]+1) error++;

  }
   
  return error;

}

int Dig_Clk_test::Check_Ber(uint32_t *packet, int length) {

  int error =0;
  uint8_t e1[2], e2[2], e3[2];

  for (int k=0; k<length-1; k++) {
        
    e1[0]=(packet[k] & 0x000000FF);
    e1[1]=(packet[k+1] & 0x000000FF);
    e2[0]=(packet[k] & 0x0000FF00) >> 8;
    e2[1]=(packet[k+1] & 0x0000FF00) >> 8;
    e3[0]=(packet[k] & 0x00FF0000) >> 16;
    e3[1]=(packet[k+1] & 0x00FF0000) >> 16;
    
    if(e1[0]!=0xAB || e2[0]!=0xAB || e3[0]!=0xAB) error++;
  }
    
  return error;

}

// DLL configuration as outlined in the SALT manual
bool Dig_Clk_test::DLL_Check() {
  
  uint8_t data=0xFF;
  uint8_t command=0xFF;
  uint8_t read=0;


  // ---------- FOR SALT8 ----------
  cout << "Setting dll_cp_cfg to default value: 0x9A" << endl;
  salt_->read_salt(registers::dll_cp_cfg, &read);
  if(read != 0x9A) 
    salt_->write_salt(registers::dll_cp_cfg,(uint8_t) 0x9A);
  usleep(100);

  cout << "Enabling DLL" << endl;
  salt_->write_salt(0x301, (uint8_t) 0x80);
  usleep(100);

  //salt_->write_salt(0x303, (uint8_t) 0x9A);
  usleep(100);
  
  // salt_->write_salt(0x301, (uint8_t) 0xE0);
  //usleep(100);
  //salt_->write_salt(0x301, (uint8_t) 0x20);
  //usleep(1000);
  
  //salt_->write_salt(0x303, (uint8_t) 0x9A);
  // -------------------------------

  
  /* 
  // Set correct value of CP current
  command = 0x9A;
  salt_->write_salt(registers::dll_cp_cfg, command);
  //cout << "test1" << endl;
  // Set dll_vcdl_cfg to start value
  uint8_t init = 0x60;
  salt_->write_salt(registers::dll_vcdl_cfg, init);
  //cout << "test2" << endl;
  // Wait 1 us
  usleep(1);
  //cout << "test3" << endl;
  // Read dll_cur_ok bit from dll_vcdl_mon and make sure it is 0, otherwise increase start value of dll_vcdl_cfg
  //salt_->read_salt(chipID, registers::dll_cur_ok, &data);

  bool fail=true;
  for(int i=0; i<1000; i++) {
    //  while(1 == 1) {
    salt_->read_salt(registers::dll_vcdl_mon, &data);
    if((data & 0b1000000) == 0) {fail=false; break;}
    init++;
    salt_->write_salt(registers::dll_vcdl_cfg, init);
}
  // cout << "test4" << endl;
  if(fail) {cout << "failed after 1000 times" << endl; return false;}
  cout << "data is : " << hex << (unsigned(data) &0b1000000) << endl;
  
  while((data & 0b1000000) != 0) {

    // cout << "data is : " << hex << unsigned(data) << endl;
    //cout << "init is : " << hex << unsigned(init) << endl;
    init--;
    
    salt_->write_salt(registers::dll_vcdl_cfg, init);

    //    I2C_WRITE("Other", "dll_vcdl_cfg", init);
    
   // Wait 1 us
    usleep(1);
    salt_->read_salt(registers::dll_vcdl_mon, &data);
    
  }
  //cout << "test5" << endl;

  // start synch process
  command = 0x40;
  salt_->write_salt(registers::others_g_cfg, command);
  //I2C_WRITE("Other", "others_g_cfg", 0x40); // set dll_start to 1

  // read dll_vcdl_voltage. if = 0 then pass, otherwise fail
  salt_->read_salt(registers::dll_vcdl_mon, &data);
  uint8_t value = data;

  //I2C_READ("Other", "dll_vcdl_mon");


  for(int B=0; B<6; B++) {
    if( (value>>B & 0x01) != 0 ) 
      return false;
  }

  return true;
  */	

}

bool Dig_Clk_test::PLL_Check() {

  uint8_t data = 0x00;
  uint8_t command = 0xFF;

  // Make sure PLL enabled and configured
  salt_->read_salt(registers::pll_main_cfg, &data);

  if(data != 0x8D){
    command=0x8D;
      salt_->write_salt(registers::pll_main_cfg, command);
      
  }
  // Make sure pll_cp_cfg is set to default b10011010
  salt_->read_salt(registers::pll_cp_cfg, &data);
  cout << "data is " << hex<<unsigned(data) << endl;
  if(data != 0x9A) {
    command = 0x9A;
    salt_->write_salt(registers::pll_cp_cfg, command);
  }
  // Read pll_vco_cfg and make sure approx 0
  /*
  while(1 == 1) {

    salt_->read_salt(registers::pll_vco_cfg, &data);

    if(abs(data) < 3) break;

    if(data > 3) data--;  
    else if (data < -3) data++;

//    salt_->write_salt(chipID_, registers::pll_vco_cur, data);

  }
  */
  return 1;

}

bool Dig_Clk_test::I2C_check() {

  uint8_t data = 0;
  uint8_t command = 0xFF;

  // Configure PLL
  //cout << "I2C_check: Configuring PLL" << endl;
  command = 0x8D;
  salt_->write_salt(registers::pll_main_cfg, command);
  //cout << "I2C_check: PLL configured" << endl;

  // Check that I2C can Read/Write random patters
  uint8_t x;
  srand(time(NULL)); // seed random number generator

  clock_t begin = clock();
   for(int i=0; i<10000; i++) {

    x = rand() & 0xFF;
    //cout << "x = " << x << endl;
    x |= (rand() & 0xFF) << 8;
    //cout << "x = " << x << endl;
    x |= (rand() & 0xFF) << 16;
    //cout << "x = " << x << endl;
    x |= (rand() & 0xFF) << 24;

   
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

 
  
  // salt_->read_salt(registers::pack_cfg, (uint8_t) );
  //salt_->read_salt(registers::pack_cfg, &read);
  //cout << "pack_cfg = " << hex << unsigned(read) << endl;
  
  // Reset TFC state machine and set all related registers to default values
  cout << "Reset state machine and set all registers to default values" << endl;
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x01);
  usleep(1000);
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x00);
  cout << "Reset complete" << endl;

  // define single shot transmission
  bool singleShot = true;
  uint8_t commands = 0x00;

  // salt_->write_salt(registers::pattern_cfg, (uint8_t) 0xFF);
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x20);

  // Set syncX_cfg to default values
  /*
  commands = 0x0F;
  salt_->write_salt(registers::sync0_cfg, commands);
  commands = 0x99;
  salt_->write_salt(registers::sync1_cfg, commands);
  commands =  0x55;
  salt_->write_salt(registers::sync2_cfg, commands);
  commands = 0xAA;
  salt_->write_salt(registers::sync3_cfg, commands);
  commands = 0xC;
  salt_->write_salt(registers::sync4_cfg, commands);
  */
   salt_->write_salt(0x105, (uint8_t) 0xAA);
  salt_->write_salt(0x106, (uint8_t) 0x2A);
  // Define command length (will be 2 in this case)
  uint8_t length = 0x05;
  // Define command list (BXID and Sync)
  uint8_t command[max_commands]={0};
  
  // command[0] = 0x01; // BXID
  //command[1] = 0x40; // Sync
  //command[2] = 0x00;
  command[0]=0x00;
  //cout << "TFC_main data[i] = " << hex << unsigned(data[i]) << endl;
  cout << "about to write to TFC" << endl;
  // Execute commands
  fastComm_->write_tfc(length, command, length, singleShot);

  // Read out data packet
  uint8_t length_read = 10; // number of clock cycles to read
  uint32_t data[5120]; // data packet
  uint8_t delay = 0; // clock delay
  int trigger = 1; // trigger aquisition

 
  
  cout << "about to read DAQ" << endl;
  fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "finished reading DAQ" << endl;
  
  for(int i=0; i<length_read; i++){
    fastComm_->write_tfc(length, command, length, singleShot);
   cout << "i is " << i << endl;
   cout << "DSP data[i] = " << hex << unsigned(data[i]) << endl;
   data[i]=0;
    /*  
  if((data[3*i] & 15) != 0xC) return false; // check sync4
  if(((data[3*i+1] >> 4) & 255) != 0xAA) return false; // check sync3
  if(((data[3*i+2] >> 12) & 255) != 0x55) return false; // check sync2
  if(((data[i+3] >> 20) & 255) != 0x99) return false; // check sync1
  if(((data[i+4] >> 28) & 255) != 0x0F) return false; // check sync0
    */
  }
  
 
  cout << "TFC sync completed" << endl;
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x25);//0x08);

  for(int i=0; i<255; i++) {
    
  //data={0};
    // Reset TFC registers and empty data buffers by doing an FEReset
  command[0]=0xAB;
  //command[1]=0x01;
  //cout << "command is " << hex << unsigned(command[0]) << endl;
  fastComm_->write_tfc(length, command, length, singleShot);
   fastComm_->read_daq(delay,length_read,trigger,data);
   //if((data[0] & 0x000000FF) != 0x000000FF) {
     // cout << "TFC_main data[" << i << "] = " << hex << unsigned(data[0]) << endl;
     cout << "TFC_main data[" << i << "] = " << hex << unsigned(data[0]) << endl;
     //}
   salt_->write_salt(registers::tfc_fifo_cfg,(uint8_t) i);
   usleep(100);
  
}
  //command[2]=0x40;
  length_read = 0x01;
  uint8_t c[8];
  c[0]=0x01;
  c[1]=0x02;
  c[2]=0x04;
  c[3]=0x08;
  c[4]=0x10;
  c[5]=0x20;
  c[6]=0x40;
  c[7]=0x80;/*
  for(int j=0; j<8; j++) {
    command[0]=c[j];
    cout << "command is " << hex << unsigned(c[j]) << endl;
  fastComm_->write_tfc(length, command, length, singleShot);
   fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "TFC_main data[i] = " << hex << unsigned(data[0]) << endl;
  //salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x20);
  //fastComm_->read_daq(delay,length_read,trigger,data);
    for(int i=0; i<length_read; i++){
   
    salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x20);
    fastComm_->read_daq(delay,length_read,trigger,data);
    cout << "i = " << i << endl;
  cout << "DSP data[i] = " << hex << unsigned(data[i]) << endl;
  //fastComm_->write_tfc(length, command, length, singleShot);
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x21);
  fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "TFC_main data[i] = " << hex << unsigned(data[i]) << endl;
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x22);
  fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "pattern data[i] = " << hex << unsigned(data[i]) << endl;
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x23);
  fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "counter data[i] = " << hex << unsigned(data[i]) << endl;
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x24);
  fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "TFC_des data[i] = " << hex << unsigned(data[i]) << endl;
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x25);
  fastComm_->read_daq(delay,length_read,trigger,data);
  cout << "TFC_fifo data[i] = " << hex << unsigned(data[i]) << endl;
  
    }
  }
	    */
  //salt_->write_salt(0x301, (uint8_t) 0x80);
  bool tfc_reset_fail=false;
  /*for(int i=0; i<length_read; i++){
    cout << "i is " << i << endl;
    cout << "data[i] = " << hex << unsigned(data[i]) << endl;
    /*  
  if((data[3*i] & 15) != 0xC) return false; // check sync4
  if(((data[3*i+1] >> 4) & 255) != 0xAA) return false; // check sync3
  if(((data[3*i+2] >> 12) & 255) != 0x55) return false; // check sync2
  if(((data[i+3] >> 20) & 255) != 0x99) return false; // check sync1
  if(((data[i+4] >> 28) & 255) != 0x0F) return false; // check sync0
  
    if((0x00FFFFFF & data[i])!=0) tfc_reset_fail=true;
  }
  */
  if(tfc_reset_fail) return false; // check to make sure FEReset clears data buffers, otherwise chip is BAD
 
  // Check Header TFC command
  command[0]=0x04;
  fastComm_->write_tfc(length, command, length, singleShot);
  fastComm_->read_daq(delay,length_read,trigger,data);

  for(int i=0; i<5*length; i++) {
  if((data[i] & 15) != 9 || (data[i] & 15) != 8) return false; // check header (should be more robust to make sure polarity is OK)
  }
  cout << "Header command check finished" << endl;

  // Check BxVeto (should be same output as header command)
  command[0] = 0x10;
  fastComm_->write_tfc(length, command, length, singleShot);
  
  fastComm_->read_daq(delay,length_read,trigger,data);
  for(int i=0; i<5*length; i++) {
  if((data[i] & 15) != 9 || (data[i] & 15) !=8) return false; // check header (should be more robust to make sure polarity is OK)
  }
  cout << "BxVeto command check finished" << endl;

  cout << "TFC checks finished" << endl;
  return true;

}
