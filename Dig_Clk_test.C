#include <time.h>
#include "Dig_Clk_test.h"
#include "Salt.h"
#include <unistd.h>
#include "Fpga.h"
#include "registers_config.h"
#include "fastComm.h"

// CONSTRUCTOR
Dig_Clk_test::Dig_Clk_test(Fpga *fpga, Salt *salt, FastComm *fastComm) {
   fpga_=fpga;
  salt_=salt;
  fastComm_=fastComm;
}

/*
// Gets Hex representation address of command "name"
uint8_t mainScript::assignAddress(string name, string name_list[], uint8_t address_list[]) {

  int i = 0;
  //cout << "i is " << i << endl;
  //cout << "name is " << name << endl;
  //cout << "name at i is " << name_list[i];
  while( name != name_list[i] ) i++;

  //cout << "address is " << name_list[i] << endl;
  return address_list[i];
  
  //Salt *st = new Salt();                                                                                                                                                                                                                         
  //uint8_t data = 0;                                                                                                                                                                                                                              
  //st->read_salt(0b101,0, &data);                                                                                                                                                                                                                 
  //printf(" 0x%02x \n",data);
///
}
*/

// Synch output of DAQ to clock
void Dig_Clk_test::DAQ_Sync() {

  uint32_t data = 0;

  // DAQ Reset
  //fpga_reg = assignAddress("DAQ_Cfg", m_FPGA_address_name, m_FPGA_address);
  fpga_->write_fpga(registers::DAQ_CFG, 0x01);

  // Clear FIFO
  //  fpga_reg = assignAddress("DAQ_Ctl", m_FPGA_address_name, m_FPGA_address);
  fpga_->write_fpga(registers::DAQ_CFG,0x10);

  // Set correct pattern
  salt_->write_salt(registers::pattern_cfg, 0xAB); // Set pattern for synch, in this case hAB
  salt_->write_salt(registers::ser_source_cfg, 0x22); // Reset ser_source_cfg (count up, pattern register output)

  // DAQ Sync
  //fpga_reg = assignAddress("DAQ_Cfg", m_FPGA_address_name, m_FPGA_address);

  for (int i = 0; i < 10000; i++) {
    
    fpga_->write_fpga(registers::DAQ_CFG,0x02);
    
    while(1==1) {
      fpga_->read_fpga(registers::DAQ_CFG, &data);
      if(data == 0x04) break;
    
    }

    fastComm_->read_daq(0,1,1,&data);
    if(data == 0xAB) {
      
      cout << "E-links synched to clock" << endl;
      return;
      
    }
    else {
      cout << "Failed to Synch E-links to clock" << endl;
      exit(1);
    }
  
  }
}

// DLL configuration as outlined in the SALT manual
bool Dig_Clk_test::DLL_Check() {

  uint16_t data=0xFF;

  // Set correct value of CP current
  salt_->write_salt(registers::dll_cp_cfg, 0x9A);

  // Set dll_vcdl_cfg to start value
  uint16_t init = 0x60;
  salt_->write_salt(registers::dll_vcdl_cfg, init);
  
  // Wait 1 us
  usleep(1);

  // Read dll_cur_ok bit from dll_vcdl_mon and make sure it is 0, otherwise increase start value of dll_vcdl_cfg
  //salt_->read_salt(chipID, registers::dll_cur_ok, &data);
  while(1 == 1) {
    salt_->read_salt(registers::dll_vcdl_mon, &data);
    if((data & 0b1000000) == 0) break;
    init++;
    salt_->write_salt(registers::dll_vcdl_cfg, init);
}
  while((data & 0b1000000) == 0) {

    init--;
    
    salt_->write_salt(registers::dll_vcdl_cfg, init);

    //    I2C_WRITE("Other", "dll_vcdl_cfg", init);
    
   // Wait 1 us
    usleep(1);
    salt_->read_salt(registers::dll_vcdl_mon, &data);
  }

  // start synch process
  salt_->write_salt(registers::others_g_cfg, 0x40);
  //I2C_WRITE("Other", "others_g_cfg", 0x40); // set dll_start to 1

  // read dll_vcdl_voltage. if = 0 then pass, otherwise fail
  salt_->read_salt(registers::dll_vcdl_mon, &data);
  uint16_t value = data;

  //I2C_READ("Other", "dll_vcdl_mon");


  for(int B=0; B<6; B++) {
    if( (value>>B & 0x01) != 0 ) 
      return false;
  }

  return true;
	

}

bool Dig_Clk_test::PLL_Check() {

  uint16_t data = 0x00;

  // Make sure PLL enabled and configured
  salt_->read_salt(registers::pll_main_cfg, &data);

  if(data != 0x8D)
    salt_->write_salt(registers::pll_main_cfg, 0x8D);

  // Make sure pll_cp_cfg is set to default b10011010
  salt_->read_salt(registers::pll_main_cfg, &data);
  if(data != 0x9A)
    salt_->write_salt(registers::pll_cp_cfg, 0x9A);

  // Read pll_vco_cfg and make sure approx 0
  while(1 == 1) {

    salt_->read_salt(registers::pll_vco_cfg, &data);

    if(abs(data) < 3) break;

    if(data > 3) data--;  
    else if (data < -3) data++;

//    salt_->write_salt(chipID_, registers::pll_vco_cur, data);

  }

  return 1;

}

bool Dig_Clk_test::I2C_check() {

  uint16_t data = 0;

  // Configure PLL
  salt_->write_salt(registers::pll_main_cfg, 0x8D);

  // Check that I2C can Read/Write random patters
  uint32_t x;
  srand(time(NULL)); // seed random number generator

  for(int i=0; i<100; i++) {

    x = rand() & 0xFF;
    x |= (rand() & 0xFF) << 8;
    x |= (rand() & 0xFF) << 16;
    x |= (rand() & 0xFF) << 24;

    salt_->write_salt(registers::pattern_cfg, x);
    
    salt_->read_salt(registers::pattern_cfg, &data);

    if(data!=x) return false;

  }
  return true;

}

bool Dig_Clk_test::TFC_check() {


  // Reset TFC state machine and set all related registers to default values
  fpga_->write_fpga(registers::TFC_CFG,0x01);

  // define single shot tyransmission
  bool singleShot = 1;

  // Set syncX_cfg to default values
  salt_->write_salt(registers::sync0_cfg, 0x0F);
  salt_->write_salt(registers::sync1_cfg, 0x99);
  salt_->write_salt(registers::sync2_cfg, 0x55);
  salt_->write_salt(registers::sync3_cfg, 0xAA);
  salt_->write_salt(registers::sync4_cfg, 0xC);

  // Define command length (will be 2 in this case)
  uint32_t length = 0x02;

  // Define command list (BXID and Sync)
  uint32_t command[max_commands];
  command[0] = 0x01; // BXID
  command[1] = 0x40; // Sync

  // Execute commands
  fastComm_->write_tfc(length, command, length, singleShot);

  // Read out data packet
  uint32_t length_read = 1; // number of clock cycles to read
  uint32_t data = 0; // data packet
  uint32_t delay = 0; // clock delay
  int trigger = 1; // trigger aquisition
  fastComm_->read_daq(delay,length_read,trigger,&data);
 
  if((data & 15) != 0xC) return false; // check sync4
  if(((data >> 4) & 255) != 0xAA) return false; // check sync3
  if(((data >> 12) & 255) != 0x55) return false; // check sync2
  if(((data >> 20) & 255) != 0x99) return false; // check sync1
  if(((data >> 28) & 255) != 0x0F) return false; // check sync0

  cout << "TFC sync completed" << endl;

  // Reset TFC registers and empty data buffers by doing an FEReset
  command[0]=0x02;
  length = 0x01;
  fastComm_->write_tfc(length, command, length, singleShot);
  fastComm_->read_daq(delay,length_read,trigger,&data);
  if(data != 0) return false; // check to make sure FEReset clears data buffers, otherwise chip is BAD
 
  // Check Header TFC command
  command[0]=0x04;
  fastComm_->write_tfc(length, command, length, singleShot);
  fastComm_->read_daq(delay,length_read,trigger,&data);

  if((data & 15) != 9 || (data & 15) != 8) return false; // check header (should be more robust to make sure polarity is OK)
  cout << "Header command check finished" << endl;

  // Check BxVeto (should be same output as header command)
  command[0] = 0x10;
  fastComm_->write_tfc(length, command, length, singleShot);
  fastComm_->read_daq(delay,length_read,trigger,&data);
  if((data & 15) != 9 || (data & 15) !=8) return false; // check header (should be more robust to make sure polarity is OK)                                                 
  cout << "BxVeto command check finished" << endl;

  cout << "TFC checks finished" << endl;
  return true;

}
