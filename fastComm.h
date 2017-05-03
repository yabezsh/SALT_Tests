#include "Fpga.h"

//#include <time.h> 
using namespace std;

class fastComm {

 public:

  fastComm();
  fastComm(Fpga*);
  
  ~fastComm();

  // define fpga link
// Fpga *fpga = new Fpga();
 // Fpga *fpga;

  // functions
  uint32_t assignAddress(string name, string name_list[], uint32_t address_list[]);
  unsigned int DAQ_READ(int clock_delay, int length, int trigger);
  void TFC_W(uint32_t length, uint32_t command[], int period, int singleShot);

  // addresses
  uint32_t m_FPGA_address[37]= {
    0x00, 0x01, // General FPGA Configuration                                                                                                                                                                       
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, // I2C Configuration Channel 0 (Dedicated to SALT128)                                                                                                                       
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, // I2C Configuration Channel 1                                                                                                                                              
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, // I2C Configuration Channel 2                                                                                                                                              
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, // TFC Configuration                                                                                                                                                        
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCF // DAQ Registers                                                                                                                               
  };
  uint32_t m_FPGA_address_value[37]= {
    0x00, 0x00, // General FPGA Configuration                                                                                                                                                                       
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // I2C Configuration Channel 0 (Dedicated to SALT128)                                                                                                                       
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // I2C Configuration Channel 1                                                                                                                                              
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // I2C Configuration Channel 2                                                                                                                                              
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // TFC Configuration                                                                                                                                                        
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00 // DAQ Registers                                                                                                                                
  };
  string m_FPGA_address_name[37]={
    "Main_Cfg", "Power_Cfg", // General FPGA Configuration                                                                                                                                                          
    "I2C0_Cfg", "I2C0_Cmd", "I2C0_Addr", "I2C0_DataW", "I2C0_DataR", "I2C0_Trigger", // I2C Configuration Channel 0 (Dedicated to SALT128)                                                                          
    "I2C1_Cfg", "I2C1_Cmd", "I2C1_Addr", "I2C1_DataW", "I2C1_DataR", "I2C1_Trigger", // I2C Configuration Channel 1                                                                                                 
    "I2C2_Cfg", "I2C2_Cmd", "I2C2_Addr", "I2C2_DataW", "I2C2_DataR", "I2C2_Trigger", // I2C Configuration Channel 2                                                                                                 
    "TFC_Cfg", "TFC_Length", "TFC_Period0", "TFC_Period1", "TFC_WR", "TFC_Trigger", // TFC Configuration                                                                                                            
    "DAQ_Cfg", "DAQ_Ctl", "DAQ_Delay", "DAQ_Length0", "DAQ_Length1", "DAQ_Read0", "DAQ_Read1", "DAQ_Read2", "DAQ_Read3", "DAQ_Read4", "DAQ_Trigger" // DAQ Registers                                                
  };

 private:
    Fpga *fpga;

};
