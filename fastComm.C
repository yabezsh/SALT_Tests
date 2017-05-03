#include "Fpga.h"
#include "fastComm.h"

fastComm::fastComm(){}

void fastComm::TFC_W(int length, uint32_t command[], int period, int singleShot) {

  uint32_t fpga_reg;
  uint32_t data = 0;

  fpga_reg = assignAddress("TFC_Length", m_FPGA_address_name, m_FPGA_address);
  fpga->write_fpga(fpga_reg, length);

  // specify command list
  fpga_reg = assignAddress("TFC_WR", m_FPGA_address_name, m_FPGA_address);
  for(int i = 0; i < length; i++)
    fpga->write_fpga(fpga_reg, command[i]);

  // Specify configuration
  fpga_reg = assignAddress("TFC_Cfg", m_FPGA_address_name, m_FPGA_address);
  if(singleShot)
    fpga->write_fpga(fpga_reg, 0x02); // single shot
  else fpga->write_fpga(fpga_reg, 0x00); // continuous
  
  // Specify period
  fpga_reg = assignAddress("TFC_Period0", m_FPGA_address_name, m_FPGA_address);
  fpga->write_fpga(fpga_reg, length);

  // Trigger (for single shot only)
  fpga_reg = assignAddress("TFC_Cfg", m_FPGA_address_name, m_FPGA_address);
  if(singleShot) fpga->write_fpga(fpga_reg, 0x01);
  
  while(1==1) {
    fpga->read_fpga(fpga_reg, &data);
    if(data==0x00) {

      cout << "TFC Sent" << endl;
      return;
    }

  }
}

unsigned int fastComm::DAQ_READ(int clock_delay, int length, int trigger) {

  unsigned int packet=0;
  uint32_t fpga_reg;
  uint32_t data =0;
  uint32_t e0 = 0;
  uint32_t e1 = 0;
  uint32_t e2 = 0;
  uint32_t e3 = 0;
  uint32_t e4 = 0;

  // Clear FIFO
  fpga_reg = assignAddress("DAQ_Ctl", m_FPGA_address_name, m_FPGA_address);
  uint32_t Cmd = 0x01; // Clear_Fifo command
  fpga->write_fpga(fpga_reg, Cmd);

  // Specify length
  fpga_reg = assignAddress("DAQ_Length0", m_FPGA_address_name, m_FPGA_address);
  fpga->write_fpga(fpga_reg, length);
  
  // Specify configuration
  fpga_reg = assignAddress("DAQ_Delay", m_FPGA_address_name, m_FPGA_address);
  fpga->write_fpga(fpga_reg, clock_delay); // specify clock delay

  fpga_reg = assignAddress("DAQ_Trigger", m_FPGA_address_name, m_FPGA_address);
  if(trigger) fpga->write_fpga(fpga_reg, 0x01 << 1); // Writes 1 to DAQ_Triggered

  // Trigger acquisition 
  fpga->write_fpga(fpga_reg, 1);


  // read e-links and construct data packet
  for(int i = 0; i < length; i++) {
    fpga_reg = assignAddress("DAQ_Read0", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(fpga_reg, &data);
    e0=data;

    fpga_reg = assignAddress("DAQ_Read1", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(fpga_reg, &data);
    e1=data;

    fpga_reg = assignAddress("DAQ_Read2", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(fpga_reg, &data);
    e2=data;

    fpga_reg = assignAddress("DAQ_Read3", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(fpga_reg, &data);
    e3=data;

    fpga_reg = assignAddress("DAQ_Read4", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(fpga_reg, &data);
    e4=data;

    packet |= e0 << (length - (5*i+1) )*8;

    packet |= e1 << (length - (5*i+2) )*8;

    packet |= e2 << (length - (5*i+3) )*8;

    packet |= e3 << (length - (5*i+4) )*8;

    packet |= e4 << (length - (5*i+5) )*8;

  }


  return packet;


}


// Gets Hex representation address of command "name"

uint32_t fastComm::assignAddress(string name, string name_list[], uint8_t address_list[]) {
  int i = 0;

  while( name != name_list[i] ) i++;

  return address_list[i];

}
