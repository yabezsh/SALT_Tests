#include "Ana_tests.h"
#include "registers_config.h"
#include <fstream>
#include <cmath>
#include <cstring>
#include <iomanip>
//#include "LstSquQuadRegr.h"
//#include <gsl/gsl_sf_bessel.h>


// CONSTRUCTOR
Ana_tests::Ana_tests(Fpga *fpga, Salt *salt, FastComm *fastComm) {

  fpga_=fpga;
  salt_=salt;
  fastComm_=fastComm;
  
}

void Ana_tests::Get_run(string option, int runs, string outText) {
  
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
  float std_dev[128] = {0};
  int ADC[128] = {0};
  int ADC_runs[128][runs] = {0};
  int bxid = 0;
  int parity = 0;
  int mem_space = 0;
  int flag = 0;
  
  int length1= 0;
  unsigned twelveBits;
  length_read = 255;
  float avg_chip[runs] = {0};
  float length_avg = 0;
  period = length;
  
  // Define a number of headers for init.
  for(int i=0; i<79; i++)
    command[i]=0x04;
  
  if(option == "Normal")          command[79] = 0x00;
  else if(option == "BXReset")    command[79] = 0x01;
  else if(option == "Header")     command[79] = 0x04;
  else if(option == "NZS")        command[79] = 0x08;
  else if(option == "BxVeto")     command[79] = 0x10;
  else if(option == "Snapshot")   command[79] = 0x20;
  else if(option == "Synch")      command[79] = 0x40;
  else if(option == "Calib")      command[79] = 0x80;
  else if(option == "Calib_NZS") {command[79] = 0x80; command[80] = 0x08;}
  else {
    cout << "ERROR: Output packet not properly defined." << endl;  
    return;  
  } 
    
  // Take a number of runs
    for(int i = 0; i < runs; i++) {
      fastComm_->Take_a_run(length_read, data_string, length, 0, command, period, singleShot, true );

      // read 12 bits at a time
      for(int j=0; j<data_string.length(); j+=3) {
	twelveBits = fastComm_->read_twelveBits(data_string, j);
	fastComm_->read_Header(twelveBits, bxid, parity, flag, length1);      

	// check flag bit to see if normal data packet
	if((flag == 0) && (option != "Calib_NZS")) {

	  length_avg+=length1/((float) runs);

	  if(length1 == 0 ) continue;
	  
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
	ADC_runs[j][i] = ADC[j];
	avg_ADC[127-j]+=ADC[j]/((float)runs);
	avg_chip[i]+=ADC[127-j]/(128.);
	ADC[j]=0;
      }     
      
    }

     
    
    for(int i = 0; i < 128; i++) {
      m_avg_adc[i] = avg_ADC[i];
      m_std_dev[i] = calculateSD(ADC_runs[i], runs);
      histogram(-32,64,ADC_runs[i],runs, i);

     
    }


    
    m_noise = 0;
    
    for(int i=0; i<runs; i++) 
      m_noise+= avg_chip[i]/((float) runs);
        
    m_noise_rms = calculateSD(avg_chip, runs);
    
    output_file(runs, avg_ADC, avg_chip, avg_noise, length_avg, outText, option);

    //cout << "test4" << endl;
}

// creates output file with run(s) results
void Ana_tests::output_file(int runs, float avg_ADC[], float avg_chip[], float avg_noise, float length_avg, string outText, string option) {

  ofstream outfile;
  outfile.open ((option+"_"+outText+".txt").c_str());
  outfile << "length_avg = " << dec << length_avg << endl;
  
  for(int j = 0; j<runs; j++) avg_noise+=avg_chip[j]/((float) runs);
      
  outfile << "runs = " << dec << runs << ", Average = " << avg_noise << ", std_dev = " << calculateSD(avg_chip, runs)<<endl;
  
  
  
  for(int j=0; j<128; j++) {
      outfile << dec << avg_ADC[j] << endl;
  }
  
  outfile.close();
    
  return;  
  
}

// baseline corrections through trim dac
bool Ana_tests::Baseline_corr() {
 
  // Set SALT to DSP output
  salt_->write_salt(registers::ser_source_cfg, (uint8_t) 0x20);
 
  // output files to save data
  ofstream outfile, trimdac_scan;
  outfile.open("Baseline_value.txt");
  trimdac_scan.open("Trim_DAC_scan.txt");
 
  // variables
  float adc_base[128][256] = {0};
  int baseline[128] = {0};
  float avg_baseline = 0;

  // set commom baseline to all channels
  salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X84);


  //cout << "blah" << endl;
  //get a number of runs for each baseline value
  for(int i = 0; i < 256 ; i++) {
    //cout << "testing" << endl;
    // baseline value written to salt
    salt_->write_salt(registers::baseline_g_cfg,(uint8_t) i);
  
    Get_run("NZS",10,"no_output");
    //cout << "trim dac = " << i << endl;
    for(int j = 0; j < 128; j++) adc_base[j][i] = m_avg_adc[j];

    //cout << "done" << endl;
  }

  cout << "test5" << endl;
  // loop over all baseline values and channels and find best value (lowest abs(ADC))
  for(int j = 0; j < 128; j++) {
    for(int i=0; i<256; i++) {
      trimdac_scan << adc_base[j][i] << "\t";
      if(abs(adc_base[j][baseline[j]]) > abs(adc_base[j][i])) {
	baseline[j]=i;
      }
    }

    // save scan data to output file
    trimdac_scan << endl;

    // calculate avg best value
    avg_baseline += baseline[j]/128.;

    // write best value to salt register
    salt_->write_salt((registers::baseline0_cfg) + j,(uint8_t) baseline[j]);
    //cout << "ch " << j << " set" << endl;
  }

  //cout << "test 6" << endl;
  // write best trim dac setting to file
  outfile << "AVG = " << avg_baseline << endl;
  outfile << "STD_DEV = " << calculateSD(baseline,128) << endl;
  for(int i = 0; i < 128; i++)
    outfile << dec << baseline[i] << endl;

  //cout << "test 7 " << endl;
  // close output files
  outfile.close();
  //cout << "test 7.1 " << endl;
  trimdac_scan.close();

  //cout << "test8" << endl;
  
  // revert to individual baseline value for each channel
  salt_->write_salt(registers::ana_g_cfg, (uint8_t) 0X04);

  //cout << "baseline 1 "<< endl;
  
  if(avg_baseline == 0)
    return false;

  return true;

  
}


// check noise functionality of chip. Specify run number and data output stream type, i.e. masked ch ("MASK"), sync ch ("SYNC"), after ped sub ("PEDS"), after mcm("MCMS"), and either Normal or NZS data packet
bool Ana_tests::Get_noise(int runs, string data_type, string option) {
  
  string outText;
  uint8_t buffer=0x00;

  // set mcms thresholds to smallest and largets value
  salt_->write_salt(registers::mcm_th_cfg, (uint8_t) 0x1F);
  salt_->write_salt(registers::mcm_th2_cfg, (uint8_t) 0x20);

  // DSP output
  salt_->write_salt(registers::ser_source_cfg,(uint8_t) 0x20);
  
  // check which data stream is selected
  if(data_type == "MASK")
    buffer = 0x00;
  else if(data_type == "SYNC")
    buffer = 0x20;
  else if(data_type == "PEDS")
    buffer = 0x40;
  else if(data_type == "MCMS")
    buffer = 0x60;
  else {
    cout << "ERROR:: Data stream not properly defined" << endl;
    return false;
  }
  
  salt_->write_salt(registers::n_zs_cfg, buffer);
  
  // read back n_zs_cfg to make proper outfile name
  salt_->read_salt(registers::n_zs_cfg, &buffer);
  
  if((buffer & 0b01100000) == 0) outText = "noise_masked_ADC";
  else if((buffer & 0x60) == 0b00100000) outText = "noise_sync_ADC";
  else if((buffer & 0x60) == 0b01000000) outText = "noise_after_ped";
  else if((buffer & 0x60) == 0b01100000) outText = "noise_after_mcm";
  else {
    
    cout <<"ERROR::n_zs_cfg not properly defined" << endl;
    return false;
    
  }
  
  // Do Normal packet
  Get_run(option,runs,outText);

  

  cout << "NOISE = " << m_noise << " +- " << m_noise_rms << endl;
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0);
  
  return true;
  
}

void Ana_tests::adc_output(int min, int bins) {

  // adv avg value
  for(int i = 0; i < 128; i++) {
    
    cout  << showpos<< fixed << setprecision(3) <<  m_avg_adc[i] << " ";
    
  }
  cout << endl;

  // std dev
  for(int i = 0; i < 128; i++) {
    
    cout  << showpos << fixed << setprecision(3) << m_std_dev[i] << " ";
    
  }
  cout << endl;

  // histogram min and number of bins
  cout << noshowpos << dec << min << endl;
  cout << bins << endl;

  
  // output histogram results for each channel
  for(int i = 0; i < 128; i++) {

    for(int j=min+32; j< min+32+bins; j++) {  
      cout << setfill('0') << setw(3) << adc_hist[i][j] << " ";
      
    }
    cout << endl;
  }
  
}

// make histogram
void Ana_tests::histogram(int start, int bins, int data[], int size, int ch) {

  start=start+32;
  int counter = 0;
  //int hist[64] = {0};
  //adc_hist[ch]=hist;

  //cout << "defining histo" << endl;
  //cout << "size is " << size << endl;
  //cout << "start is " << start << endl;
  
  //for(int i=0; i < 128; i++)
  //for(int j=0; j < 64; j++)
  //   adc_hist[i][j]=0;
  
   if(start+bins > 64) {
     cout << "ERROR::Histogram defined to be out of range" << endl;
     return;
   }
   
   for (int i = start; i < (start + bins); i++) {

     counter = 0;
     
     for(int j=0; j< size; j++) {

       
       // cout << "data[" << dec << j << "] = " << data[j] << endl;
       // cout << "i = " << dec << i << endl;
       if((data[j]>=(i-32)) && (data[j] <(i+1-32))) counter++;
       
     }
     //cout << "counter is " << counter << endl;
     
     adc_hist[ch][i]=counter;
     
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
      
      Get_run("NZS",1,"mcm_test");
      
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
  bool flag = true;
  
  // Set output to ped subtraction data stream
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x40);
  
  for(int i = 0; i < 64; i++) {

    // set common value of ped=i to all channels
    ped = 0x40 | i;

    salt_->write_salt(registers::ped_g_cfg, (uint8_t) ped);

    Get_run("NZS",10,"ped_test");
   
    ped = i;
    if(i >= 32) 
      ped = i-64;
    
    for(int k = 0; k < 128; k++) {
      
      if(abs(m_avg_adc[k] - ped) > 1) {
	cout << "ERROR::Ch" << dec << k << " failed Pedestal subtraction" << endl;
	outfile << "ch = " << dec << k << endl;
	flag = false;
      }
    }
    
  }
  
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x20);
  salt_->write_salt(registers::ped_g_cfg, (uint8_t) 0x00);
  
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
    Get_run("Normal",1,"no_out");

    // Check that no channel has hits above the threshold
    for(int k = 0; k < 128; k++) {      
      
      if(m_avg_adc[k] < i && m_avg_adc[k]>0) {
	cout << "ERROR: Hits below ZS threshold" << endl;
	return false;
      }
      
    }
    
    // take 100 runs and save result
    outText << "nzs_th_" << dec << i;
    Get_run("Normal",100,outText.str());
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
    sum += data[i];
  
  mean = sum/((float) runs);
  
  for(i = 0; i < runs; ++i)
    standardDeviation += pow(data[i] - mean, 2);
  
  return sqrt(standardDeviation / ((float) runs));
}

float Ana_tests::calculateSD(int data[], int runs) {

  float sum = 0.0, mean, standardDeviation = 0.0;
  int i;
  for(i = 0; i < runs; ++i)
    sum += data[i];
  
  mean = sum/((float) runs);
  
  for(i = 0; i < runs; ++i)
    standardDeviation += pow(data[i] - mean, 2);
  
  return sqrt(standardDeviation / ((float) runs));
}
 
void Ana_tests::Check_Gain() {



  // float x[10], y[10];

  // for(int i=0; i<10; i++) {
  //   x[i]=i;
  //   y[i] = 2*i+1;
  // }

  // Check_linear(x,y,10,0.1);
  /*
  stringstream outText;
  // calibration pulse run
  salt_->write_salt(registers::calib_main_cfg,(uint8_t) 0x0F);
  salt_->write_salt(registers::calib_enable0_cfg,(uint8_t) 0x0F);
  salt_->write_salt(registers::calib_clk_cfg, (uint8_t) 0x20);
  
  //Get_run("Calib",1,"test_pulse");
  
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x60);
  salt_->write_salt(registers::calib_volt_cfg, (uint8_t) 31);   
  // return;
  for(int j = 0; j < 64; j=j+4) {
    
    salt_->write_salt(registers::adc_clk_cfg, (uint8_t) j);
    if(j<8 || j>40)
      salt_->write_salt(registers::pack_adc_sync_cfg,(uint8_t) 0x80);
    else
      salt_->write_salt(registers::pack_adc_sync_cfg,(uint8_t) 0x00);
   
  
    
     for(int i = 0; i < 32; i++) {
      
   
       Get_run("Calib",1,"test");
   
     }
 
  }
   salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0);
}
  */



}

bool Ana_tests::Check_linear(float x[], float y[], int size, float thresh) {

  float a, b, c, ratio;

  Get_quadTerms(x,y,size,a,b,c);
  ratio = abs(c/b);
  
  if(ratio > thresh) {
    cout << "ERROR:: DATA SET NOT LINEAR" << endl; 
    return false;
  }
}

// my quadratic fit
void Ana_tests::Get_quadTerms(float x[], float y[], int npoints, float &a, float &b, float &c) {

  float S01 = 0, S02 = 0, S03 = 0, S04 = 0, S10 = 0, S11 = 0, S12 = 0;
  float a2, b2, c2, a3, b3, c3, a4, b4, c4;
  float m, n;
  for(int i=0; i < npoints; i++) {

    S01 += x[i];
    S02 += (x[i]*x[i]);
    S03 += (x[i]*x[i]*x[i]);
    S04 += (x[i]*x[i]*x[i]*x[i]);
    S10 += y[i];
    S11 += (y[i]*x[i]);
    S12 += (y[i]*x[i]*x[i]);
  }

  a2 = S01/((float) npoints);
  a3 = S02/((float) npoints);
  a4 = S10/((float) npoints);

  b2 = S02/S01;
  b3 = S03/S01;
  b4 = S11/S01;

  c2 = S03/S02;
  c3 = S04/S02;
  c4 = S12/S02;

  
  m = (a4-c4) + (b4-a4)/(b2-a2)*(c2-a2);
  n = (c3-a3) + (b3-a3)/(b2-a2)*(c2-a2);

  c = -m/n;

  b = 1/(b2-a2)*( (b4-a4) +(b3-a3)*c);

  a = a4-a2*b-a3*c;
}

void Ana_tests::set_calib_fifo() {

  float best=0;
  bool flag =  false;
  int best_l = 0;
  int best_c = 0;
  
  salt_->write_salt(registers::calib_fifo_cfg, (uint8_t) 0x06);
  // calibration pulse run
  salt_->write_salt(registers::calib_main_cfg,(uint8_t) 0x05);
  for(int i = 0x307; i < 0x317; i++) {
    
    salt_->write_salt(i,(uint8_t) 0xFF);

    }
  salt_->write_salt(registers::calib_clk_cfg, (uint8_t) 0x20);
  salt_->write_salt(registers::n_zs_cfg, (uint8_t) 0x60);
  salt_->write_salt(registers::calib_volt_cfg, (uint8_t) 0x31);   
 
  for(int i=0; i < 8 ; i++) {

    
    salt_->write_salt(registers::calib_fifo_cfg, (uint8_t) i);
    for(int j = 0; j < 64; j=j+4) {
      salt_->write_salt(registers::adc_clk_cfg, (uint8_t) j);

      
      
      Get_run("Calib", 10, "Calib");
      
      //best = m_noise;
      if(abs(m_noise) > abs(best)){
	best = m_noise;
	best_l = i;
	best_c = j;
	//adc_output(-10,20);

	
      }
    
    }
    
  }
  salt_->write_salt(registers::calib_fifo_cfg, (uint8_t) best_l);
  
  salt_->write_salt(registers::adc_clk_cfg, (uint8_t) best_c);
  Get_run("Calib", 100, "Calib");
  cout << "i = " << best_l << endl;
  cout << "j = " << best_c << endl;
  cout << "ADC_avg = "  << m_noise << " +- " << m_noise_rms << endl;
  adc_output(-32,64);
}
