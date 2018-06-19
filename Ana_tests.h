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
  void Trim_dac_scan();
  void Check_noise();
  void Check_NZS();
  float calculateSD(float data[]);


  private:
  Fpga* fpga_;
  Salt* salt_;
  FastComm* fastComm_;

};
