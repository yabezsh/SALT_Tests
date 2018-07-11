#include "Ana_tests.h"
#include "registers_config.h"
#include <fstream>
#include <cmath>
#include <cstring>
//#include <gsl/gsl_sf_bessel.h>


// CONSTRUCTOR
Ana_tests::Ana_tests(Fpga *fpga, Salt *salt, FastComm *fastComm) {

  fpga_=fpga;
  salt_=salt;
  fastComm_=fastComm;
  
}

bool Ana_tests::Get_run(string option, int runs, bool output, string outText) {
  
  // Define command length (will be 2 in this case)
  uint8_t length = 90;  // Define command list (BXID and Sync)
  uint8_t command[256]={0};
  // Read out data packet
  uint16_t length_read = 150; // number of clock cycles to read

  // output file
  ofstream outfile;
  outfile.open ((option+"_"+outText+".txt").c_str());
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
  uint8_t mcm1, mcm2;
  
  
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
    return false;
    
  }
  
  int max;
  if(option == "Calib") max=256;
  else max=1;

  float length_avg = 0;
  for(int k = 0; k < max; k++) {
    
    double v[runs][128] = {0};
    period = length;
    
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

	  if(length1 == 0) continue;
	  
	  fastComm_->read_Normal_packet(data_string, j, ADC);
	  break;
	}
	
	if(length1 == 0x30 || length1 == 0x13 || length1 == 0x14 || length1 == 0x15) continue;
	
	if(length1 == 6) {
	  
	  length_avg+=length1/((float) runs);
	  fastComm_->read_NZS_packet(data_string, j, ADC, bxid, parity, m_mcm_v, m_mcm_ch, mem_space);
	  break;
	}
	
      }
      
      for(int j=0; j<128; j++) {
	avg_ADC[127-j]+=ADC[j]/((float)runs);
	avg_chip[i]+=ADC[127-j]/(128.);
	ADC[j]=0;
      }     
      
    }
    
    for(int i = 0; i < 128; i++) {
      m_avg_adc[i] = avg_ADC[i];
      if(m_avg_adc[i] !=0 && k>0) cout << "k=" << dec << k << endl;
    }
    
  }
  /*
  // output noise parameters to screen if doing a noise run
  if(outText == "noise_sync_ADC") {

    cout << "Raw: noise = " << 
    
  }
  if(outText == "noise_after_ped") {

    
    
  }
  if(outText == "noise_after_mcm") {

    
    
  }
  */
  output_file(runs, avg_ADC, avg_chip, avg_noise, length_avg, outText, option);
  
  return true;
  
}

// creates output file with run(s) results
void Ana_tests::output_file(int runs, float avg_ADC[], float avg_chip[], float avg_noise, float length_avg, string outText, string option) {

  ofstream outfile;
  outfile.open ((option+"_"+outText+".txt").c_str());
  outfile << "length_avg = " << dec << length_avg << endl;
  
  for(int j = 0; j<runs; j++) avg_noise+=avg_chip[j]/((float) runs);

  if(avg_noise != 0 && option == "Calib") cout << "HERE" << endl;
   
  outfile << "runs = " << dec << runs << ", Average = " << avg_noise << ", std_dev = " << calculateSD(avg_chip, runs)<<endl;
  
  
  
    for(int j=0; j<128; j++) {
      outfile << dec << avg_ADC[j] << endl;
    }
    

   outfile.close();
  
  return;  
  
}

// baseline corrections through trim dac
void Ana_tests::Baseline_corr() {

  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);


  
  ofstream outfile, trimdac_scan;
  outfile.open("Baseline_value.txt");
  trimdac_scan.open("Trim_DAC_scan.txt");
  float adc_base[128][256] = {0};
  int baseline[128] = {0};
  
  // set commom baseline to all channels
  salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X84);

  // loop over baseline values
  for(int i = 0; i < 256 ; i++) {

    salt_->write_salt(registers::baseline_g_cfg,(uint8_t) i);

    // get adc of 1 run
    Get_run("NZS",10,true,"no_output");
    for(int j = 0; j < 128; j++) adc_base[j][i] = m_avg_adc[j];
    
  }

  for(int j = 0; j < 128; j++) {
    for(int i=0; i<256; i++) {
      trimdac_scan << adc_base[j][i] << "\t";
      if(abs(adc_base[j][baseline[j]]) > abs(adc_base[j][i])) {
	baseline[j]=i;
	
      }
    
    }
      trimdac_scan << endl;
    //cout << "baseline[" << dec << j << "] = " << dec << baseline[j] << endl;
    outfile << dec << baseline[j] << endl;
    salt_->write_salt((registers::baseline0_cfg) + j,(uint8_t) baseline[j]);
  }
  

  // revert to individual baseline value for each channel
  outfile.close();
  trimdac_scan.close();
  salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X04);
  
  
}


// check noise functionality of chip
void Ana_tests::Check_noise() {
  
  string option = "NZS";
  string outText;
  uint8_t buffer=0x00;

  // set mcms thresholds to smallest and largets value
  salt_->write_salt(registers::mcm_th_cfg, (uint8_t) 0x1F);
  salt_->write_salt(registers::mcm_th2_cfg, (uint8_t) 0x20);

  // DSP output
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);

  
  // loop over different data streams, i.e. masked ADC, sync ADC, after ped, after mcms
  for(int i = 0; i < 4; i++) {

    buffer = i << 5;
    salt_->write_salt(registers::n_zs_cfg, buffer);
    
    // read back n_zs_cfg to make proper outfile name
    salt_->read_salt(registers::n_zs_cfg, &buffer);
        
    if((buffer & 0b01100000) == 0) outText = "noise_masked_ADC";
    else if((buffer & 0b01100000) == 0b00100000) outText = "noise_sync_ADC";
    else if((buffer & 0b01100000) == 0b01000000) outText = "noise_after_ped";
    else if((buffer & 0b01100000) == 0b01100000) outText = "noise_after_mcm";
    else {
      
      cout <<"ERROR::n_zs_cfg not properly defined" << endl;
      return;
      
    }
    
    // Do Normal packet
    Get_run("Normal",100,true,outText);
    
    // Do NZS packet
    Get_run("NZS",100,true,outText);
  }
  
  
}

// checks mcmch and mcm_v
bool Ana_tests::Check_MCMS(float ADC[128], int mcm1, int mcm2, int mcm_ch, int mcm_v) {

  int mcm_ch_calc=0;
  int mcm_v_calc=0;

  // check that expected and calculated mcm_ch are equal
  for(int i=0; i < 128; i++) 
    if((ADC[i] >= mcm2) && (ADC[i] < mcm1)) mcm_ch_calc++;
  
  if(mcm_ch_calc != mcm_ch) {
    cout << "ERROR::Calculated and expected MCM channel not equal" << endl;
    cout << "mcm_ch_calc = " << dec << mcm_ch_calc << endl;
    cout << "mcm_ch = " << dec << mcm_ch << endl;
    return false;
  }

  // check that expected and calculated mcm_value are equal
  for(int i=0; i < 128; i++) 
    if((ADC[i] >= mcm2) && (ADC[i] < mcm1)) mcm_v_calc=+ADC[i]/mcm_ch;
  
  if(abs(mcm_v-mcm_v_calc)>1 ) {
    cout << "ERROR::Calculated and expected MCM value not equal" << endl;
    cout << "mcm_v_calc = " << dec << mcm_v_calc << endl;
    cout << "mcm_v = " << dec << mcm_v << endl;
    return false;
  }
  
  return true;
}

// loop over mcm thresholds and check if MCMS works properly
bool Ana_tests::Check_MCMS() {
  
  int mcm1, mcm2;
  
  // set to ADC after Ped mode
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x40);
  
  for(int i = 0; i < 64; i++) {
    
    
    if(i < 32) mcm1 = i;
    else mcm1 = i-64;
    
    salt_->write_salt(registers::mcm_th_cfg, (uint8_t) i);
    
    
    for(int j = 0; j < 64; j++) {
      
      if(i < 32) mcm2 = j;
      else mcm2 = j-64;
      if(mcm2>mcm1) continue;
      
      salt_->write_salt(registers::mcm_th2_cfg, (uint8_t) j);
      
      Get_run("NZS",1,true,"mcm_test");
      
      if(!Check_MCMS(m_avg_adc, mcm1, mcm2, m_mcm_ch, m_mcm_v)) 
	return false;
      
    }
    
  }
  

  return true;
}
  


bool Ana_tests::Check_PedS() {

  ofstream outfile;
  outfile.open("Ped_sub_failed_ch.txt");
  int ped;
  int bad_ch[128] = {0};
  bool flag = true;
  //float  = 0;
  
  // Set output to ped subtraction data stream
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x40);
  
  for(int i = 0; i < 64; i++) {

    // set common value of ped=i to all channels
    ped = 0x40 | i;

    salt_->write_salt(registers::ped_g_cfg, (uint8_t) ped);

    Get_run("NZS",10,true,"ped_test");
   
    ped = i;
    if(i >= 32) 
      ped = i-64;
    
    for(int k = 0; k < 128; k++) {
      
      if(abs(m_avg_adc[k] - ped) > 1) {
	cout << "ERROR::Ch" << dec << k << " failed Pedestal subtraction" << endl;
	outfile << "ch = " << dec << k << endl;
	//bad_ch[k]=1;
	flag = false;
      }
    }
    
  }
  
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x20);
  salt_->write_salt(registers::ped_g_cfg, (uint8_t) 0x00);
  outfile.close();
  return flag;
  
}

// Check ZS threshold
bool Ana_tests::Check_NZS() {
    
  stringstream outText; 
  uint8_t n_zs = 0x20;
  
  for(int i = 0; i < 32; i++) {

    // set ZS threshold
    n_zs = n_zs | i;
    salt_->write_salt(registers::n_zs_cfg, n_zs);
    
    // Take a run
    Get_run("Normal",1,true,"no_out");

    // Check that no channel has hits above the threshold
    for(int k = 0; k < 128; k++) {      
      
      if(m_avg_adc[k] < i && m_avg_adc[k]>0) {
	cout << "ERROR: Hits below ZS threshold" << endl;
	return false;
      }
      
    }
    
    // take 100 runs and save result
    outText << "nzs_th_" << dec << i;
    Get_run("Normal",100,true,outText.str());
    outText.str("");
  }

  // revert threshold to 0
  salt_->write_salt(registers::n_zs_cfg,(uint8_t) 0);

  return true;
}

float Ana_tests::calculateSD(float data[], int runs) {

  float sum = 0.0, mean, standardDeviation = 0.0;
  
  int i;
  
  for(i = 0; i < runs; ++i)
    {
      sum += data[i];
    }
  
  mean = sum/10;
  
  for(i = 0; i < runs; ++i)
    standardDeviation += pow(data[i] - mean, 2);
  
  return sqrt(standardDeviation / ((float) runs));
}
 
void Ana_tests::Check_Gain() {

  stringstream outText;
  // calibration pulse run
  salt_->write_salt(registers::calib_main_cfg,(uint8_t) 0x95);
  salt_->write_salt(registers::calib_enable0_cfg,(uint8_t) 0xFF);
   salt_->write_salt(registers::calib_clk_cfg, (uint8_t) 0x00);
   
    
  for(int j = 0; j < 32; j++) {
    
    salt_->write_salt(registers::adc_clk_cfg, (uint8_t) j);
    
    cout << "clk = " << dec << j << endl;
    
    for(int i = 0; i < 32; i=i+4) {
      
      outText << "clk_" << j;
      salt_->write_salt(registers::calib_volt_cfg, (uint8_t) i);
      cout << "vth_" << dec << i << endl;
      outText << "_vth_" << i;
      Get_run("Calib",1,true,outText.str());
      outText.str("");
    }
 
  }
}
