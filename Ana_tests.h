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
//#include <vector>

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
  bool Get_noise(int runs, string data_type, string option);
  bool Check_NZS();
  bool Baseline_corr();
  float calculateSD(float data[], int runs);
  float calculateSD(int data[], int runs);
  void adc_output(int min, int bins);
  void histogram(int start, int bins, int data[], int size, int ch);
  void set_calib_fifo();
  //vector<int> m_hist[128];
  int m_hmin;
  int m_bins;
  float m_avg_adc[128];
  float m_std_dev[128];
  float m_noise;
  float m_noise_rms;
  int m_mcm_ch;
  int m_mcm_v;
  // int m_hist[128];
  int adc_hist[128][64];
  // vector<int> hist;
  bool Check_PedS();
  bool Check_MCMS();
  bool Check_MCMS(float ADC[128], int mcm1, int mcm2, int mcm_ch, int mcm_v);
  bool Check_linear(float x[], float y[], int size, float thresh);
  bool Get_Quad_Coef(float x[], float y[], int PointsNum, float &a, float &b, float &c);
  void Get_quadTerms(float x[], float y[], int npoints, float &a, float &b, float &c);

  private:
  Fpga* fpga_;
  Salt* salt_;
  FastComm* fastComm_;

};
