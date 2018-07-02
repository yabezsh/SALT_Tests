#include "Ana_tests.h"
#include "registers_config.h"
#include <fstream>
#include <cmath>
#include <cstring>


// CONSTRUCTOR
Ana_tests::Ana_tests(Fpga *fpga, Salt *salt, FastComm *fastComm) {

  fpga_=fpga;
  salt_=salt;
  fastComm_=fastComm;
  
}

void Ana_tests::Get_run(string option, int runs, bool output, string outText) {
  
  // Define command length (will be 2 in this case)
  uint8_t length = 90;
  // Define command list (BXID and Sync)
  uint8_t command[256]={0};
  // Read out data packet
  uint16_t length_read = 150; // number of clock cycles to read
  //uint32_t data[5120]; // data packet
  //uint8_t delay = 0; // clock delay

  ofstream outfile;
  outfile.open (("noise_"+option+"_"+outText+".txt").c_str());
  float avg_noise = 0;
  int period = length;
  bool singleShot = true;
  string data_string;
  float avg_ADC[128] = {0};
  int ADC[128] = {0};
  int bxid = 0;
  int parity = 0;
  int mcm_v = 0;
  int mcm_ch = 0;
  int mem_space = 0;
  int flag = 0;
  //int runs = 1;
  int length1= 0;
  unsigned twelveBits;
  //int largest=0;
  length_read = 255;
  float avg_chip[runs] = {0};
  float std_dev = 0;
  
  
  // cout << "option = " << option << endl;
  

  // Define a number of headers for init.
  for(int i=0; i<79; i++)
    command[i]=0x04;
  
  
  if(option == "Normal")        command[79] = 0x00;
  else if(option == "BXReset")  command[79] = 0x01;
  else if(option == "Header")   command[79] = 0x04;
  else if(option == "NZS")      command[79] = 0x08;
  else if(option == "BxVeto")   command[79] = 0x10;
  else if(option == "Snapshot") command[79] = 0x20;
  else if(option == "Synch")    command[79] = 0x40;
  else if(option == "Calib")    command[79] = 0x80;
  else {
    cout << "ERROR: Output packet not properly defined." << endl;

  }
  
  double v[runs][128] = {0};
  period = length;
  float length_avg = 0;

  // Take a number of runs
  for(int i = 0; i < runs; i++) {
    fastComm_->Take_a_run(length_read, data_string, length, 0, command, period, singleShot, true );     
    
    // read 12 bits at a time
    for(int j=0; j<data_string.length(); j+=3) {
      twelveBits = fastComm_->read_twelveBits(data_string, j);
      fastComm_->read_Header(twelveBits, bxid, parity, flag, length1);      

     
      // check flag bit to see if normal data packet
      if(flag == 0) {
	
	length_avg+=length1/((float) runs);
	if(length1 == 0) break;
	
	fastComm_->read_Normal_packet(data_string, j, ADC);
	break;
      }
      //       cout << "length = " << dec << length1 << endl;
      if(length1 == 0 || length1 == 3) continue;
      if(length1 == 1 && option == "BxVeto") cout << "BxVeto OK" << endl;
      if(length1 == 2 && option == "Header") cout << "Header OK" << endl;
      if(length1 == 4 || length1 == 5) {
	//cout << "Buffer Full: exiting run" << endl;
	continue;
      }
      if(length1 == 6) {
	// cout << "bxid = " << bxid << endl;
	// cout << "parity = " << parity << endl;
	//  cout << "flag = " << flag << endl;
	//	 cout << "length = " << dec << length1 << endl;
	 length_avg+=length1/((float) runs);
	fastComm_->read_NZS_packet(data_string, j, ADC, bxid, parity, mcm_v, mcm_ch, mem_space);
	break;
      }
      
    }

    for(int j=0; j<128; j++) {
      v[i][j]+=ADC[j]/((float)runs);
      avg_ADC[127-j]+=ADC[j]/((float)runs);
      avg_chip[i]+=ADC[127-j]/(128.);
	ADC[j]=0;
      }     
    
  }

  
  for(int i = 0; i < 128; i++) {

    //cout << avg_ADC[i] << endl;
    m_avg_adc[i] = avg_ADC[i];
  }

  output_file(runs, avg_ADC, avg_chip, avg_noise, length_avg, outText, option);
  
  return;

}

void Ana_tests::output_file(int runs, float avg_ADC[], float avg_chip[], float avg_noise, float length_avg, string outText, string option) {

  ofstream outfile;
  outfile.open (("noise_"+option+"_"+outText+".txt").c_str());
  outfile << "length_avg = " << dec << length_avg << endl;
  
  for(int j = 0; j<runs; j++) avg_noise+=avg_chip[j]/((float) runs);
   
  outfile << "runs = " << dec << runs << ", Average = " << avg_noise << ", std_dev = " << calculateSD(avg_chip)<<endl;
  
  
  
    for(int j=0; j<128; j++) {
      outfile << dec << avg_ADC[j] << endl;
    }
    

   outfile.close();
  
  return;  
  
}

// baseline corrections through trim dac
void Ana_tests::Baseline_corr() {

  float adc_base[128][256] = {0};
  // set commom baseline to all channels
  salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X84);

  // loop over baseline values
  for(int i = 0; i < 256 ; i++) {

    salt_->write_salt(registers::baseline_g_cfg,(uint8_t) i);

    // get adc of 1 run
    Get_run("NZS",10,true,"no_output");
    for(int j = 0; j < 128; j++) {
      
      //cout << "ch " << dec << j << " = " << dec << m_avg_adc[j] << endl;
      adc_base[j][i] = m_avg_adc[j];
      
      //
    }
    
    // if(i==0) continue;
    for(int j = 0; j < 127; j++) {
      if(abs(adc_base[j][i+1] > abs(adc_base[j][i])) {
	salt_->write_salt(0x206 + j,(uint8_t) i);
	//cout << "channel " << dec << j << " baseline " << dec << i << endl; 
      }
    }
      
  }
  
 salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X04);
  
  

}
	

void Ana_tests::Check_noise() {
  
    string option = "NZS";
    string outText;
    uint8_t buffer=0x00;
    cout << "PERFORMING NOISE CHECK" << endl;
    
    // DSP output
    salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);
    salt_->write_salt(registers::n_zs_cfg,buffer);
    
    // read back n_zs_cfg to make proper outfile name
    salt_->read_salt(registers::n_zs_cfg, &buffer);

    if((buffer & 0b01100000) == 0) outText = "noise_masked_ADC";
    else if((buffer & 0b01100000) == 0b0010000) outText = "noise_sync_ADC";
    else if((buffer & 0b01100000) == 0b0100000) outText = "noise_after_ped";
    else if((buffer & 0b01100000) == 0b0110000) outText = "noise_after_mcm";
    else {

      cout <<"ERROR::n_zs_cfg not properly defined" << endl;
      return;
      
    }
    
    // Do Normal packet
    Get_run("Normal",1000,true,outText);

    // Do NZS packet
    Get_run("NZS",1000,true,outText);
  
    
    
}


void Ana_tests::Check_NZS() {

  cout << "DOING NZS TEST" << endl;

  stringstream outText;

  
  for(int i = 0; i < 32; i++) {

    salt_->write_salt(registers::n_zs_cfg,(uint8_t) i);

    outText << "nzs_th_" << dec << i;

    cout << i << ", ";

    Get_run("Normal",100,true,outText.str());

    outText.str("");
  }
  
}
 
float Ana_tests::calculateSD(float data[]) {

  float sum = 0.0, mean, standardDeviation = 0.0;
  
  int i;
  
  for(i = 0; i < 10; ++i)
    {
      sum += data[i];
    }
  
  mean = sum/10;
  
  for(i = 0; i < 10; ++i)
    standardDeviation += pow(data[i] - mean, 2);
  
  return sqrt(standardDeviation / 10);
}
 
