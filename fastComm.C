#include "Fpga.h"
 #include "fastComm.h"
#include "registers_config.h"
#include  <iomanip>
#include <sstream>

FastComm::FastComm(Fpga *fpga){fpga_=fpga;}

void FastComm::config_daq(uint16_t length, uint8_t clock_delay, bool tfc_trig) {

  uint8_t ACQ_L = 0;
  uint8_t ACQ_H = 0;

  //if(length<256) ACQ_L = length;
  //else {

    
    
  //}

  ACQ_L = (length & 0x00FF);
  ACQ_H = (length & 0x0F00) >> 8;
  //cout << "ACQ_H = " << hex << (unsigned) ACQ_H << endl;
  //cout << "ACQ_L = " << hex << (unsigned) ACQ_L << endl;
  // clear fifo
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x11);
  usleep(1);
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);

  
  // Specify length
  fpga_->write_fpga(registers::DAQ_ACQ_L, ACQ_L);
  fpga_->write_fpga(registers::DAQ_ACQ_H, ACQ_H);
    
  // Specify clock delay
  fpga_->write_fpga(registers::DAQ_DELAY, clock_delay);

  
  // Set to TFC trig
  if(tfc_trig)
    fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t) 0x02); //It is left "untriggered" but TFC_Triggered is ON        
  else
    fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t) 0x00);

  //   uint8_t data8;
  // fpga_->read_fpga(registers::DAQ_TRIGGER, &data8);
  // cout << "TRIGGER_1 = " << hex << unsigned(data8) << endl;
  
      
}

void FastComm::config_tfc(uint8_t length, uint8_t command[], uint8_t period, bool singleShot) {

 
  // reset TFC state machine
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x01);
  usleep(1);
  fpga_->write_fpga(registers::TFC_CFG,(uint8_t) 0x00);
  
  // Specify Length
  fpga_->write_fpga(registers::TFC_LENGTH, length);
  // Specify period
  fpga_->write_fpga(registers::TFC_PERIOD0, period);
  
  // Specify single shot or cont.
  if(singleShot)
    fpga_->write_fpga(registers::TFC_CFG, (uint8_t)0x02); // single shot
  else
    fpga_->write_fpga(registers::TFC_CFG, (uint8_t)0x00); // continuous
  
  // specify command list
  for(uint i = 0; i < length; i++)
    fpga_->write_fpga(registers::TFC_WR, command[i]);
  
}


void FastComm::Launch_ACQ(bool tfc_trig)
{
//Pre: if tfc_trig is specified the TFC must have been configured beforehand                                                                                                                      
//Post: it stops both TFC and DAQ triggers before leaving this function. REGARDLESS OF WETHER IT WENT RIGHT OR NOT                                                                                


  
  uint8_t data, original_DAQ_TRIGGER, data8;
  
  fpga_->read_fpga(registers::DAQ_TRIGGER, &original_DAQ_TRIGGER);
  fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)(original_DAQ_TRIGGER | 0x01));
  


  
  //else
  if(tfc_trig) {
    //  fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x03);
    fpga_->write_fpga(registers::TFC_TRIGGER, (uint8_t)0x01); //Trigger the TFC
    
    }
  
  clock_t begin = clock();
  do
    {
      fpga_->read_fpga(registers::DAQ_TRIGGER, &data);
      //cout << "data = " << hex << (unsigned) data << endl;
    }
  while( ((data & 0x10) != 0x10) && (clock()-begin) < 1E-3*CLOCKS_PER_SEC);                                                          
  if(!tfc_trig)
  fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)(original_DAQ_TRIGGER & 0xFE)); //Zeroes only the trigger
  // cout << "time is " << (clock()-begin) << endl;
    //else
  // usleep(1000);
   
 
  if (tfc_trig)
    fpga_->write_fpga(registers::TFC_TRIGGER, (uint8_t)0x00); //Trigger the TFC                                                                                                 
  //fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x03);
    
  if ((data & 0x10) == 0)
    {

      cout << "ERROR::DAQ NEVER TRIGGERED AQUISITION" << endl;
      //SOMETHING WENT WRONG, IT TOOK TOO MUCH TIME                                                                                                                                     
      // COMPLAIN!                                                                                                                                                                      
    }
}

void FastComm::write_tfc()
{
  
  fpga_->write_fpga(registers::TFC_TRIGGER, (uint8_t)0x01);

  
  //usleep(100);
  //   fpga_->write_fpga(registers::TFC_TRIGGER, (uint8_t)0x00);
  
  
}

void FastComm::read_daq(uint32_t clock_delay, uint32_t length, int trigger, uint8_t (&packet)[5120]) {

  uint32_t data =0;
 
  // // Clear FIFO
  // uint32_t Cmd = 0x10; // Clear_Fifo command
  // fpga_->write_fpga(registers::DAQ_CFG, Cmd);
  // Cmd = 0x00; 
  // fpga_->write_fpga(registers::DAQ_CFG, Cmd);


  
  // // Specify length
  // fpga_->write_fpga(registers::DAQ_ACQ_L, length);
  
  // // Specify configuration
  // fpga_->write_fpga(registers::DAQ_DELAY, clock_delay); // specify clock delay


  
  // if(trigger) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x01 ); // Writes 1 to DAQ_Triggered
  // //usleep(100);
  // if(trigger) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x00 ); 
  
  // // Trigger acquisition 

  // read e-links and construct data packet
    for(uint i = 0; i < length; i++) {
      fpga_->read_fpga(registers::DAQ_READ0, &data);
      //        cout << "1st e-link is " << hex << unsigned(data) << endl;
      packet[5*i]=data;
      

  }

  
  return;// packet;


}

void FastComm::read_daq(uint8_t length, uint32_t (&packet)[5120], bool tfc_trig) {

  uint32_t data;
  
  // trigger aquisition if not tfc triggered
  trigger_DAQ(tfc_trig);
  
  // stop all triggers
  fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x00 );
   
  // read DAQ registers
  for(uint i = 0; i < length; i++) {
    fpga_->read_fpga(registers::DAQ_READ0, &data);
    packet[i]=data;
    // cout << "data is " << hex << data << endl;
  }

  // if TFC trig we prepare for next trigger
  if(tfc_trig) fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)0x03 );

  reset_DAQ();  
   
 
  return;// packet;
 

}

void FastComm::read_daq(uint8_t length, uint32_t (&packet)[5120])
{
        uint32_t data;
        for(uint i = 0; i < length; i++)
        {
                fpga_->read_fpga(registers::DAQ_READ0, &data);
                packet[i]=data;
        }
	//cout << "packet = " << hex << (unsigned) packet[0] << endl;
	//	fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x10);
	//usleep(100);
	//fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);
	//reset_DAQ();
}

string FastComm::read_daq(uint8_t length)
{
  uint64_t data;
  stringstream data_stream;
  // clock_t begin, end;// = clock();
  for(uint i = 0; i < length; i++)
    {
     
      fpga_->read_fpga(registers::DAQ_READ0, &data);
      //  begin = clock();
    
      data_stream << hex << arrange_Elinks(data);
      //   end=clock();
     
      //packet[i]=data;
      
    }
  //double elapsed_sec = double(end - begin) / CLOCKS_PER_SEC;
  //  cout << "I2C time: " << elapsed_sec << " seconds" << endl;
  //cout << "test data stream = " << data_stream.str() << endl;
 
  return data_stream.str();
  //	arrange_Elinks(data);
  

}

void FastComm::reset_DAQ() {
  
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x11);
  usleep(1);
  fpga_->write_fpga(registers::DAQ_CFG, (uint8_t) 0x00);
  
}

void FastComm::trigger_DAQ(bool tfc_trig) {

  uint8_t data;
  //usleep(100);
  fpga_->read_fpga(registers::DAQ_TRIGGER, &data);  
  if(!tfc_trig) {
    
    fpga_->write_fpga(registers::DAQ_TRIGGER, (uint8_t)(data | 0x01));
  
  }
  clock_t begin = clock();
  do {
    
    fpga_->read_fpga(registers::DAQ_TRIGGER, &data);
  }
  while( ((data & 0x10) != 0x10) && ((clock()-begin)<26E-6*CLOCKS_PER_SEC));

  if((data & 0x10) == 0) {
    cout << "SOMETHING WENT WRONT, TOOK TO MUCH TIME" << endl;
    //cout << "TIME = "
  }
}

//Wrapping function                                                                                                                                                                               
void FastComm::Take_a_run(uint16_t length_read, uint32_t (&packet)[5120], uint8_t length, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig)
{
        this->config_daq(length_read, clock_delay, tfc_trig);
        //if (tfc_trig)
	  this->config_tfc(length, command, period, singleShot);
        this->Launch_ACQ(tfc_trig);
        this->read_daq(length_read, packet);
	//		reset_DAQ();
}

void FastComm::Take_a_run(uint16_t length_read, string &data, uint8_t length, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig)
{

 
  // string data_stream;
  this->config_daq(length_read, clock_delay, tfc_trig);
  //if (tfc_trig)
 
  this->config_tfc(length, command, period, singleShot);
 
  this->Launch_ACQ(tfc_trig);
  
  data = this->read_daq(length_read);

  
  //cout << "string is " << data << endl;
  //read_NZS_packet(data_stream);
  		reset_DAQ();
}

string FastComm::arrange_Elinks(uint64_t data) {

  uint32_t e0, e1, e2, e3, e4;
  int e_linkN = 3;
  
  e0=(data & 0x00000000000000FF);
  e1=(data & 0x000000000000FF00) >> 8;
  e2=(data & 0x0000000000FF0000) >> 16;
  e3=(data & 0x00000000FF000000) >> 24;
  e4=(data & 0x000000FF00000000) >> 32;

  clock_t begin, end;// = clock();
   begin = clock();
  stringstream data_stream;
  
  data_stream << hex << setfill('0') << setw(2) << e0;
  data_stream << hex << setfill('0') << setw(2) << e1;
  data_stream << hex << setfill('0') << setw(2) << e2;
  if(e_linkN>3)
    data_stream << hex << setfill('0') << setw(2) << e3;
  if(e_linkN>4)
  data_stream << hex << setfill('0') << setw(2) << e4;

  data_stream >> hex >> data;
   end=clock();
   	double elapsed_sec = double(end - begin) / CLOCKS_PER_SEC;
	//cout << "time: " << elapsed_sec << " seconds" << endl;

  return data_stream.str();
}


// read i'th set of twelve bits in a string
unsigned FastComm::read_twelveBits(string data, int i) {

  stringstream buffer;
  unsigned twelveBits;
  buffer << hex << data[i] << hex << data[i+1] << hex << data[i+2];
  buffer >> twelveBits;
  
  return twelveBits;
}

// read header
void FastComm::read_Header(unsigned twelveBits, int &bxid, int &parity, int &flag, int &length) {
  
  //unsigned twelveBits;
  //stringstream buffer;

  // read in data string
  //buffer << hex << data[0] << hex << data[1] << hex << data[2];
  //buffer >> twelveBits;
  
  // get bxid, parity, flag and length
  bxid = (twelveBits & 0xF00) >> 8;
  parity = (twelveBits & 0x080) >> 7;
  flag = (twelveBits & 0x040) >> 6;
  length = (twelveBits & 0x03F);
  
  
}

// read a normal data packet
void FastComm::read_Normal_packet(string data, int startBit, int (&ADC)[128]) {

  unsigned twelveBits;
  int ch=0, value;
  int flag[128] = {0};
  
  for(int i=startBit+3; i<data.length(); i+=3) {

    
    twelveBits = read_twelveBits(data, i);
    if(ch>((twelveBits & 0xFE0) >> 5) ) break;
    ch = (twelveBits & 0xFE0) >> 5;
    value = twelveBits & 0x1F;
    //  cout << "ch = " << dec << ch << ", value = " << dec << value << endl;
    
    if(flag[ch]==1) break;
    ADC[ch]=value;
    flag[ch]=1;
    if(ch==127) break;
    
  }
  // cout << "Finished reading Normal packet" << endl;
  
}



// read a NZS data packet, i.e. get ADC, bxid, parity, mcm value, mcm channels, mem space
void FastComm::read_NZS_packet(string data, int startBit, int (&ADC)[128], int &bxid, int &parity, int &mcm_v, int &mcm_ch, int &mem_space) {

  unsigned twelveBits;
  unsigned dsp_mon;
  unsigned test;
  bool start_nzs = false;
  bool dsp_check = false;
  bool dsp_read = false;
  stringstream buffer;
  int j=0;
    
  // read DSP monitoring bits
  dsp_mon = read_twelveBits(data, startBit+3) << 12;
  dsp_mon = dsp_mon | read_twelveBits(data, startBit+6);
  
  mcm_v = (dsp_mon & 0xFC000) >> 18;
  mcm_ch = (dsp_mon & 0x3FC00) >> 10;
  mem_space = (dsp_mon & 0x3FE) >> 1;
  
  // if(mcm_ch != 0 ) {
  //   cout << "mcm_v = " << dec << mcm_v << endl;
  //   cout << "mcm_ch = " << dec << mcm_ch << endl;
  //   cout << "mem_space = " << dec << mem_space << endl;
  // }
  
  for(int i=startBit+9; i < data.length(); i+=3) {

    // read twelve bits at a time
    twelveBits = read_twelveBits(data, i);

    // get 
    ADC[2*j]=(twelveBits &  0xFC0) >> 6;
    ADC[2*j+1] = (twelveBits & 0x3F);

    ADC[2*j]=(ADC[2*j] < 32 ? ADC[2*j] : ( ADC[2*j] - 64));
    ADC[2*j+1]=(ADC[2*j+1] < 32 ? ADC[2*j+1] : ( ADC[2*j+1] - 64));
    
    j++;
    if(j==64) {

      // cout << "FINISHED READING NZS DATA" << endl;
      break;
      
    }
    
  }
  
}

