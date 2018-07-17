#include <stddef.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "Salt.h"
#include "Fpga.h"
#include "fastComm.h"
#include <fstream>

using namespace std;

class Ana_tests {

 public:

  Ana_tests(){};
  Ana_tests(Fpga*, Salt*, FastComm*);
  ~Ana_tests(){};

  void Get_run(string option, int runs, string outText);
  void output_file(int runs, float avg_ADC[], float avg_chip[], float avg_noise, float length_avg, string outText, string option);
  void Trim_dac_scan();
  void Check_Gain();
  void Get_noise(int runs, string data_type, string option);
  bool Check_NZS();
  void Baseline_corr();
  float calculateSD(float data[], int runs);
  float calculateSD(int data[], int runs);
  float m_avg_adc[128];
  float m_noise;
  float m_noise_rms;
  int m_mcm_ch;
  int m_mcm_v;
  bool Check_PedS();
  bool Check_MCMS();
  bool Check_MCMS(float ADC[128], int mcm1, int mcm2, int mcm_ch, int mcm_v);


  private:
  Fpga* fpga_;
  Salt* salt_;
  FastComm* fastComm_;

};
