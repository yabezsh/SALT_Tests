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


  //  return false;
  // define single shot transmission
  bool singleShot = true;
  uint8_t commands = 0x00;
 
  uint64_t data64[5120] = {0};
  salt_->write_salt(0x507,(uint8_t) 0x01);
  //salt_->write_salt(registers::ped_g_cfg,(uint8_t) 0x8F);
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);
  salt_->write_salt(registers::n_zs_cfg,(uint8_t) 0x00); // data after ped
  //   salt_->write_salt(registers::sync0_cfg,(uint8_t) 0xAB);
  // salt_->write_salt(registers::sync1_cfg,(uint8_t) 0xFB);
  singleShot=true;
  length=200;
  period=200;
  for(int i=0; i<79; i++)
    command[i]=0x04;

  command[79]=0x08;
  // command[79]=0x00;
  //command[79]=0x80;
  length_read=250;
  string data_string;
  float avg_ADC[128] = {0};
  int ADC[128] = {0};
  int bxid = 0;
  int parity = 0;
  int mcm_v = 0;
  int mcm_ch = 0;
  int mem_space = 0;
  int flag = 0;
  int runs = 256;
  int length1= 0;
  unsigned twelveBits;
  int largest=0;

  double v[runs][128] = {0};
  
  
  // make sure all channels are unmasked
  unmask_all();
  int avg  = 0;

    return false;
  salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X84);
  
  for(int i=0; i<runs; i++) {
    cout << "run = " << dec << i << endl;
    salt_->write_salt(registers::baseline_g_cfg,(uint8_t) i);
    for(int k=0; k<100; k++) {
      
      
      fastComm_->Take_a_run(length_read, data_string, length, 0, command, period, singleShot, true );     
      
      // read 12 bits at a time
      for(int j=0; j<data_string.length(); j+=3) {
	twelveBits = fastComm_->read_twelveBits(data_string, j);
	fastComm_->read_Header(twelveBits, bxid, parity, flag, length1);
	
	
	if(flag==0) {
	  cout << "bxid = " << bxid << endl;
	  cout << "parity = " << parity << endl;
	  cout << "flag = " << flag << endl;
	  cout << "length = " << dec << length1 << endl;
	  if(length1==0) break;
	  
	  fastComm_->read_Normal_packet(data_string, j, ADC);
	  break;
	}
	
	
	if(length1==6) {
	  // cout << "bxid = " << bxid << endl;
	// cout << "parity = " << parity << endl;
	// cout << "flag = " << flag << endl;
	// cout << "length = " << length1 << endl;
	  fastComm_->read_NZS_packet(data_string, j, ADC, bxid, parity, mcm_v, mcm_ch, mem_space);
	  break;
	}
	
      }
      
      for(int j=0; j<128; j++) {
	v[i][j]+=ADC[j]/100.;
	ADC[j]=0;
      }     
    }
    
  }
  
  for(int i=0; i<runs; i++ ) {
    for(int j=0; j<128; j++) {
      cout << dec << v[i][j] << ", ";//endl;
    }
    cout << endl;
  }
  return false;
  
  // cout << "TFC sync completed" << endl;
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);//0x08);

  for(int i=0; i<255; i++) {
    
  //data={0};
    // Reset TFC registers and empty data buffers by doing an FEReset
  command[0]=0xAB;
  //command[1]=0x01;
  //cout << "command is " << hex << unsigned(command[0]) << endl;
  //fastComm_->write_tfc(length, command, length, singleShot);
  //fastComm_->read_daq(delay,length_read,trigger,data);
   //if((data[0] & 0x000000FF) != 0x000000FF) {
     // cout << "TFC_main data[" << i << "] = " << hex << unsigned(data[0]) << endl;
     cout << "TFC_main data[" << i << "] = " << hex << unsigned(data[i]) << endl;
     //}
   salt_->write_salt(registers::tfc_fifo_cfg,(uint8_t) i);
   usleep(100);
  
}
 
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
  //fastComm_->write_tfc(length, command, length, singleShot);
  //fastComm_->read_daq(delay,length_read,trigger,data);

  for(int i=0; i<5*length; i++) {
  if((data[i] & 15) != 9 || (data[i] & 15) != 8) return false; // check header (should be more robust to make sure polarity is OK)
  }
  cout << "Header command check finished" << endl;

  // Check BxVeto (should be same output as header command)
  command[0] = 0x10;
  //fastComm_->write_tfc(length, command, length, singleShot);
  
  //fastComm_->read_daq(delay,length_read,trigger,data);
  for(int i=0; i<5*length; i++) {
  if((data[i] & 15) != 9 || (data[i] & 15) !=8) return false; // check header (should be more robust to make sure polarity is OK)
  }
  cout << "BxVeto command check finished" << endl;

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
