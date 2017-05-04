#include "Fpga.h"
#include "fastComm.h"
#include "registers_config.h"

FastComm::FastComm(Fpga *fpga){fpga_=fpga;}

void FastComm::write_tfc(uint32_t length, uint32_t command[], int period, bool singleShot)
{
    fpga_->write_fpga(registers::TFC_LENGTH, length);
    
    // specify command list
    for(uint i = 0; i < length; i++)
    {
        fpga_->write_fpga(registers::TFC_WR, command[i]);
    }
    
    // Specify configuration
    if(singleShot)
    {
        fpga_->write_fpga(registers::TFC_CFG, 0x02); // single shot
    } else fpga_->write_fpga(registers::TFC_CFG, 0x00); // continuous
    
    // Specify period
    fpga_->write_fpga(registers::TFC_PERIOD0, length);
    
    // Trigger (for single shot only)
    if(singleShot) fpga_->write_fpga(registers::TFC_CFG, 0x01);
    
    uint32_t data = 0;
    while(1==1)
    {
        fpga_->read_fpga(registers::TFC_CFG, &data);
        if(data==0x00) {
            cout << "TFC Sent" << endl;
            return;
        }
    }
}
/*
void FastComm::read_daq(int clock_delay, int length, int trigger, uint32_t *packet)
{
  uint32_t data =0;
  uint32_t e0 = 0;
  uint32_t e1 = 0;
  uint32_t e2 = 0;
  uint32_t e3 = 0;
  uint32_t e4 = 0;
  
  // Clear FIFO
  uint32_t Cmd = 0x01;
 // fpga_->write_fpga(fpga_reg, Cmd);
}
*/

void FastComm::read_daq(int clock_delay, int length, int trigger, uint32_t *packet) {

  unsigned int packet=0;
  uint32_t fpga_reg;
  uint32_t data =0;
  uint32_t e0 = 0;
  uint32_t e1 = 0;
  uint32_t e2 = 0;
  uint32_t e3 = 0;
  uint32_t e4 = 0;

  // Clear FIFO
  //fpga_reg = assignAddress("DAQ_Ctl", m_FPGA_address_name, m_FPGA_address);
  uint32_t Cmd = 0x10; // Clear_Fifo command
  fpga->write_fpga(registers::DAQ_CFG, Cmd);

  // Specify length
  //  fpga_reg = assignAddress("DAQ_Length0", m_FPGA_address_name, m_FPGA_address);
  fpga->write_fpga(registers::DAQ_ACQ_L, length);
  
  // Specify configuration
  //fpga_reg = assignAddress("DAQ_Delay", m_FPGA_address_name, m_FPGA_address);
  fpga->write_fpga(registers::DAQ_DELAY, clock_delay); // specify clock delay

  //fpga_reg = assignAddress("DAQ_Trigger", m_FPGA_address_name, m_FPGA_address);
  if(trigger) fpga->write_fpga(registers::DAQ_TRIGGER, 0x01 << 1); // Writes 1 to DAQ_Triggered

  // Trigger acquisition 
  fpga->write_fpga(registers::DAQ_TRIGGER, 1);


  // read e-links and construct data packet
  for(int i = 0; i < length; i++) {
    //fpga_reg = assignAddress("DAQ_Read0", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(registers::DAQ_READ0, &data);
    e0=data;

    //fpga_reg = assignAddress("DAQ_Read1", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(registers::DAQ_READ1, &data);
    e1=data;

    //fpga_reg = assignAddress("DAQ_Read2", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(registers::DAQ_READ2, &data);
    e2=data;

    //fpga_reg = assignAddress("DAQ_Read3", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(registers::DAQ_READ3, &data);
    e3=data;

    //fpga_reg = assignAddress("DAQ_Read4", m_FPGA_address_name, m_FPGA_address);
    fpga->read_fpga(registers::DAQ_READ4, &data);
    e4=data;

    packet |= e0 << (length - (5*i+1) )*8;

    packet |= e1 << (length - (5*i+2) )*8;

    packet |= e2 << (length - (5*i+3) )*8;

    packet |= e3 << (length - (5*i+4) )*8;

    packet |= e4 << (length - (5*i+5) )*8;

  }


  return;// packet;


}
