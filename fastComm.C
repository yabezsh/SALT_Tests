#include "Fpga.h"
#include "fastComm.h"
#include "registers_config.h"
#include  <iomanip>
#include <sstream>

FastComm::FastComm(Fpga *fpga){fpga_=fpga;}

void FastComm::config_daq(uint16_t length, uint8_t clock_delay, bool tfc_trig) {
  uint8_t ACQ_L = 0;
  uint8_t ACQ_H = 0;

  ACQ_L = (length & 0x00FF);
  ACQ_H = (length & 0x0F00) >> 8;
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
  uint8_t data, original_DAQ_TRIGGER;//, data8;
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
}

void FastComm::read_daq(uint32_t clock_delay, uint32_t length, int trigger, uint8_t (&packet)[5120]) {
  uint32_t data =0;
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

void FastComm::read_daq(uint8_t length, uint32_t *packet)
{	//Used in: Dig_Clk_test::TFC_DAQ_sync()
        for(uint i = 0; i < length; i++) {
		fpga_->read_DAQ_READ0(&(packet[i]));
	packet[i]=arrange_Elinks(packet[i]);
	}
}

string FastComm::read_daq(uint8_t length)
{
  uint32_t data;
  stringstream data_stream;
  for(uint i = 0; i < length; i++)
    {
      fpga_->read_fpga(registers::DAQ_READ0, &data);
      data_stream << hex << arrange_Elinks(data);
    } 
  return data_stream.str();
}

void FastComm::read_elinks(uint16_t length_read, uint32_t (&data)[5120]) {

 // uint32_t data[5120]={0};

  for(uint i = 0; i < length_read; i++)
  {
	fpga_->read_fpga(registers::DAQ_READ0, &data[i]);
	data[i] = (arrange_Elinks(data[i]) & 0xFFFFFF00) >> 8;


  }

  //return data[length];

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
/*                                                                                                                                                                               
void FastComm::Take_a_run(uint16_t length_read, uint32_t *packet, uint8_t length, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig)
{	//Used in: Dig_Clk_test::TFC_DAQ_sync()
	this->config_daq(length_read, clock_delay, tfc_trig);
	this->config_tfc(length, command, period, singleShot);
	this->Launch_ACQ(tfc_trig);
	this->read_daq(length_read, packet);
}
*/
void FastComm::Take_a_run(uint16_t length_read, string &data, uint8_t length, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig)
{ //The one with strings (slow, but it works)
  this->config_daq(length_read, clock_delay, tfc_trig);
  this->config_tfc(length, command, period, singleShot);
  this->Launch_ACQ(tfc_trig);
  data = this->read_daq(length_read);
  //read_NZS_packet(data_stream);
  reset_DAQ();
}

void FastComm::Take_a_run(uint16_t length_read, uint32_t (&data)[5120], uint8_t length, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig)
{ //The one with strings (slow, but it works)
  this->config_daq(length_read, clock_delay, tfc_trig);
  this->config_tfc(length, command, period, singleShot);
  this->Launch_ACQ(tfc_trig);
  this->read_elinks(length_read,data);
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

//  clock_t begin, end;// = clock();
   //begin = clock();
  stringstream data_stream;
  
  data_stream << hex << setfill('0') << setw(2) << e0;
  data_stream << hex << setfill('0') << setw(2) << e1;
  data_stream << hex << setfill('0') << setw(2) << e2;
  if(e_linkN>3)
    data_stream << hex << setfill('0') << setw(2) << e3;
  if(e_linkN>4)
  data_stream << hex << setfill('0') << setw(2) << e4;

  data_stream >> hex >> data;
   //end=clock();
   	//double elapsed_sec = double(end - begin) / CLOCKS_PER_SEC;
	//cout << "time: " << elapsed_sec << " seconds" << endl;

  return data_stream.str();
}


uint32_t FastComm::arrange_Elinks(uint32_t data) {

  uint32_t e0, e1, e2, e3, e4, data_out;
  int e_linkN = 3;
  
  e0=(data & 0x000000FF) << 24;
  e1=(data & 0x0000FF00) << 8;
  e2=(data & 0x00FF0000) >> 8;
  e3=(data & 0xFF000000) >> 24;
 // e4=(data & 0x000000FF00000000) >> 32;

//  clock_t begin, end;// = clock();
   //begin = clock();

 data_out = e0 | e1 | e2;
/*
  stringstream data_stream;
  
  data_stream << hex << setfill('0') << setw(2) << e0;
  data_stream << hex << setfill('0') << setw(2) << e1;
  data_stream << hex << setfill('0') << setw(2) << e2;
  if(e_linkN>3)
    data_stream << hex << setfill('0') << setw(2) << e3;
  if(e_linkN>4)
  data_stream << hex << setfill('0') << setw(2) << e4;

  data_stream >> hex >> data;
   //end=clock();
   	//double elapsed_sec = double(end - begin) / CLOCKS_PER_SEC;
	//cout << "time: " << elapsed_sec << " seconds" << endl;

  return data_stream.str();
*/
 return data_out;
  }
// read i'th set of twelve bits in a string
unsigned FastComm::read_twelveBits(string data, int i) {
 
  stringstream buffer;
  unsigned twelveBits;
  buffer << hex << data[i] << hex << data[i+1] << hex << data[i+2];
  buffer >> twelveBits;
  
  return twelveBits;
}

unsigned FastComm::read_twelveBits(uint32_t data, int i) {

  unsigned twelveBits;
  if(i==0) {
	 //cout << "Here 0" << endl; 
twelveBits = (data & 0xFFF000) >> 12;
  }
  if(i==1){
	  //cout << "Here 1" << endl;
	  twelveBits = (data & 0x000FFF);
  }
  return twelveBits;
}
// Convert NLanes x 8b of length L into 12b
void FastComm::DecodeData( uint16_t *decoded_data, uint32_t *data, int L, int NLanes) {
	//For now it only works with 3 elinks. Can be extended...
	//3x8b=24b -> 12b => Double length at the output for the case of NLanes=3;
	uint16_t *data16Bits = (uint16_t *)data; //Addressed as 16b bus
	for (int i=0; i< L; i++)
	{ //0x12345678;
    //  3 2 1 0 
		decoded_data[2*i]  = ( data16Bits[2*i]   ) &  0x0FFF; //The and is to crop to size to 12b
		decoded_data[2*i+1]= ( data16Bits[2*i+1] ) >> 4;      //We just remove the 4 trailing bits (16-4=12)
	}
}
// read header
void FastComm::read_Header(unsigned twelveBits, int &bxid, int &parity, int &flag, int &length) {
  // get bxid, parity, flag and length
  bxid = (twelveBits & 0xF00) >> 8;
  parity = (twelveBits & 0x080) >> 7;
  flag = (twelveBits & 0x040) >> 6;
  length = (twelveBits & 0x03F);
}

// read a normal data packet NEW FORMAT
void FastComm::read_Normal_packet(uint16_t *data_decoded, int length, int startBit, int *ADC) 
{
  int ch, value;
  for(int i=startBit+1; i<length; i++) //The +1 is to skip the header
	{
    ch = (data_decoded[i] & 0xFE0) >> 5;
    value = data_decoded[i] & 0x1F; 
//		if (value >= 16) value -= 32; //EXTEND THE 2s COMPLEMENT (needed? correct?)
    ADC[ch]=value;
    if(ch==127) break; //Probably unnecessary...
  }
}

// read a normal data packet
void FastComm::read_Normal_packet(uint16_t length_read, unsigned (&twelveBits)[10240], int startBit, int (&ADC)[128]) {
  //unsigned twelveBits; 
  int ch=0, value;
  int flag[128] = {0};
  for(int i=startBit+1; i<length_read*2; i++) {
    //twelveBits = read_twelveBits(data, i);
    if(ch>((twelveBits[i] & 0xFE0) >> 5) ) break;
    ch = (twelveBits[i] & 0xFE0) >> 5;
    value = twelveBits[i] & 0x1F;
    //  cout << "ch = " << dec << ch << ", value = " << dec << value << endl;
    if(flag[ch]==1) break;
    ADC[ch]=value;
    flag[ch]=1;
    if(ch==127) break;
  }
  // cout << "Finished reading Normal packet" << endl;
}


// read a NZS data packet NEW FORMAT
void FastComm::read_NZS_packet(uint16_t *data_decoded, int length, int startBit, int *ADC, int &bxid, int &parity, int &mcm_v, int &mcm_ch, int &mem_space) 
{
  unsigned dsp_mon;
  stringstream buffer;
  int j=0;
    
  // read DSP monitoring bits
  dsp_mon = data_decoded[startBit+1];
  
  mcm_v = (dsp_mon & 0xFC000) >> 18;
  mcm_ch = (dsp_mon & 0x3FC00) >> 10;
  mem_space = (dsp_mon & 0x3FE) >> 1;

  for(int i=startBit+3; i < length; i++) {

    ADC[j]   = (data_decoded[i] & 0xFC0) >> 6;
    ADC[j+1] = (data_decoded[i] & 0x3F);

    ADC[j]=(ADC[j] < 32 ? ADC[j] : ( ADC[j] - 64));
    ADC[j+1]=(ADC[j+1] < 32 ? ADC[j+1] : ( ADC[j+1] - 64));    

    j+=2;
    if(j==128) {
      // cout << "FINISHED READING NZS DATA" << endl;
      break;
    }
    
  }
  
}

// read a NZS data packet, i.e. get ADC, bxid, parity, mcm value, mcm channels, mem space
void FastComm::read_NZS_packet(uint16_t length_read, unsigned (&twelveBits)[10240], int startBit, int (&ADC)[128], int &bxid, int &parity, int &mcm_v, int &mcm_ch, int &mem_space) {

  //unsigned twelveBits;
  unsigned dsp_mon;
  int j=0;
    
  // read DSP monitoring bits
  dsp_mon = twelveBits[startBit+1] << 12;
  dsp_mon = dsp_mon | twelveBits[startBit+2];

  //cout << "DSP Mon = " << hex << (unsigned) dsp_mon << endl;
  
  mcm_v = (dsp_mon & 0xFC000) >> 18;
  mcm_ch = (dsp_mon & 0x3FC00) >> 10;
  mem_space = (dsp_mon & 0x3FE) >> 1;
  
  for(int i=startBit+3; i < length_read*2; i++) {

    // read twelve bits at a time
    //twelveBits = read_twelveBits(data, i);

    // get 
    ADC[2*j]=(twelveBits[i] &  0xFC0) >> 6;
    ADC[2*j+1] = (twelveBits[i] & 0x3F);

//cout << "ADC[" << dec << 2*j << "] = " << ADC[2*j] << endl;
//cout << "ADC[" << dec << 2*j+1 << "] = " << ADC[2*j+1] << endl;

    ADC[2*j]=(ADC[2*j] < 32 ? ADC[2*j] : ( ADC[2*j] - 64));
    ADC[2*j+1]=(ADC[2*j+1] < 32 ? ADC[2*j+1] : ( ADC[2*j+1] - 64));    

    j++;
    if(j==64) {
      // cout << "FINISHED READING NZS DATA" << endl;
      break;
    }
    
  }
  
}

