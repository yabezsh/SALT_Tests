#include "Fpga.h"
 #include "fastComm.h"
#include "registers_config.h"

FastComm::FastComm(Fpga *fpga){fpga_=fpga;}

void FastComm::write_tfc(uint8_t length, uint8_t command[], int period, bool singleShot)
{

  uint8_t data = 0;
  uint32_t data32 = 0;
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x01);
  usleep(1000);
  //fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x00);
  //usleep(1000);
  fpga_->read_fpga(registers::TFC_CFG, &data32);
  //cout << "tfc registers are " << hex << unsigned(data32) << endl;
  fpga_->write_fpga(registers::TFC_LENGTH, length);
  
  
  // specify command list
  for(uint i = 0; i < length; i++)
    {
      //cout << "writing command = " << hex << unsigned(command[i]) << endl;
      fpga_->write_fpga(registers::TFC_WR, command[i]);
      fpga_->read_fpga(registers::TFC_WR, &data);
      //cout << "wrote " << hex << unsigned(command[i]) << endl;
	}
  
  // Specify configuration
    if(singleShot)
      {
	fpga_->write_fpga(registers::TFC_CFG, (uint8_t)0x02); // single shot
      }
    else fpga_->write_fpga(registers::TFC_CFG, (uint8_t)0x00); // continuous

   
    
    // Specify period
    fpga_->write_fpga(registers::TFC_PERIOD0, length);
    
    // Trigger (for single shot only)
    //if(singleShot)
   
    
    
    //fpga_->read_fpga(registers::TFC_CFG, &data);
    // cout << "TFC_CFG = " << hex << unsigned(data) << endl;
    //fpga_->read_fpga(registers::TFC_TRIGGER, &data);
    //	cout << "trigger before set is is " << hex << unsigned(data) << endl;
    fpga_->write_fpga(registers::TFC_TRIGGER, (uint8_t)0x01);
    //fpga_->read_fpga(registers::TFC_TRIGGER, &data);
    //	cout << "data is " << hex << unsigned(data) << endl;
	
	/*while(data!=0x00)
    {
        fpga_->read_fpga(registers::TFC_TRIGGER, &data);
	//cout << "data is " << hex << unsigned(data) << endl;
        if(data==0x00) {
            cout << "TFC Sent" << endl;
            return;
        }
    }
	*/
    usleep(100);
    fpga_->write_fpga(registers::TFC_TRIGGER, (uint8_t)0x00);
}

void FastComm::read_daq(uint32_t clock_delay, uint32_t length, int trigger, uint8_t (&packet)[5120]) {

  uint32_t data =0;
 
  // Clear FIFO
  uint32_t Cmd = 0x10; // Clear_Fifo command
  fpga_->write_fpga(registers::DAQ_CFG, Cmd);
  Cmd = 0x00; 
  fpga_->write_fpga(registers::DAQ_CFG, Cmd);


  
  // Specify length
  fpga_->write_fpga(registers::DAQ_ACQ_L, length);
  
  // Specify configuration
  fpga_->write_fpga(registers::DAQ_DELAY, clock_delay); // specify clock delay


  
  if(trigger) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x01 ); // Writes 1 to DAQ_Triggered
  usleep(1000);
  if(trigger) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x00 ); 
  
  // Trigger acquisition 

  // read e-links and construct data packet
  for(uint i = 0; i < length; i++) {
    fpga_->read_fpga(registers::DAQ_READ0, &data);
    //        cout << "1st e-link is " << hex << unsigned(data) << endl;
    packet[5*i]=data;
    

  }

  
  return;// packet;


}

void FastComm::read_daq(uint8_t clock_delay, uint8_t length, int trigger, uint32_t (&packet)[5120]) {

  uint32_t data =0;
  uint8_t data8 =0;
  // Clear FIFO
  uint8_t Cmd = 0x10; // Clear_Fifo command
  fpga_->write_fpga(registers::DAQ_CFG, Cmd);
  Cmd = 0x00;
  usleep(100);
  fpga_->write_fpga(registers::DAQ_CFG, Cmd);


  
  // Specify length
  fpga_->write_fpga(registers::DAQ_ACQ_L, length);
  
  // Specify configuration
  fpga_->write_fpga(registers::DAQ_DELAY, clock_delay); // specify clock delay

  if(trigger==1) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x01 ); // Writes 1 to DAQ_Triggered
  usleep(1000);
  //if(trigger==1) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x00 ); 
  
  // Trigger acquisition 

 // fpga_->read_fpga(registers::DAQ_TRIGGER, &data);
 while(data8 != 0x10) {

   fpga_->read_fpga(registers::DAQ_TRIGGER, &data8);
   //   cout << "daq trigger is " << hex << unsigned(data8) << endl;

 }
 fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x00 ); 
  // read e-links and construct data packet
  for(uint i = 0; i < length; i++) {
    fpga_->read_fpga(registers::DAQ_READ0, &data);
    // cout << "1st e-link is " << hex << unsigned(data) << endl;
    packet[i]=data;

    
  }

  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x10);

  
  
  return;// packet;


}
