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

  void Get_run(string option, int runs, bool output, string outText);
  void output_file(int runs, float avg_ADC[], float avg_chip[], float avg_noise, float length_avg, string outText, string option);
    void Trim_dac_scan();
  void Check_noise();
  void Check_NZS();
  void Baseline_corr();
  float calculateSD(float data[]);
  float m_avg_adc[128];


  private:
  Fpga* fpga_;
  Salt* salt_;
  FastComm* fastComm_;

};
