#include "Fpga.h"
#include "fastComm.h"

fastComm::fastComm(){}

void fastComm::TFC_W(int length, uint8_t command[], int period, int singleShot) {

  uint8_t fpga_reg;

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

    if(fpga->read_fpga(fpga_reg)==0x00) {

      cout << "TFC Sent" << endl;
      return;
    }

  }
}

unsigned int fastComm::DAQ_READ(int clock_delay, int length, int trigger) {

  unsigned int packet=0;
  uint8_t fpga_reg;
  uint8_t e0 = 0;
  uint8_t e1 = 0;
  uint8_t e2 = 0;
  uint8_t e3 = 0;
  uint8_t e4 = 0;

  // Clear FIFO
  fpga_reg = assignAddress("DAQ_Ctl", m_FPGA_address_name, m_FPGA_address);
  uint8_t Cmd = 0x01; // Clear_Fifo command
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
    e0 = fpga->read_fpga(fpga_reg);

    fpga_reg = assignAddress("DAQ_Read1", m_FPGA_address_name, m_FPGA_address);
    e1 = fpga->read_fpga(fpga_reg);

    fpga_reg = assignAddress("DAQ_Read2", m_FPGA_address_name, m_FPGA_address);
    e2 = fpga->read_fpga(fpga_reg);

    fpga_reg = assignAddress("DAQ_Read3", m_FPGA_address_name, m_FPGA_address);
    e3 = fpga->read_fpga(fpga_reg);

    fpga_reg = assignAddress("DAQ_Read4", m_FPGA_address_name, m_FPGA_address);
    e4 = fpga->read_fpga(fpga_reg);

    packet |= e0 << (length - (5*i+1) )*8;

    packet |= e1 << (length - (5*i+2) )*8;

    packet |= e2 << (length - (5*i+3) )*8;

    packet |= e3 << (length - (5*i+4) )*8;

    packet |= e4 << (length - (5*i+5) )*8;

  }


  return packet;


}
