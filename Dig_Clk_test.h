#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "Salt.h"
#include "Fpga.h"
#include "fastComm.h"

//#include <time.h> 
using namespace std;

class Dig_Clk_test {

 public:

  Dig_Clk_test(){};
  Dig_Clk_test(Fpga*, Salt*, FastComm*);
  ~Dig_Clk_test(){};

  //Salt *st = new Salt();

  void DAQ_Sync();
  int Check_Ber(uint8_t *packet, int length);
  void FPGA_PLL_shift(int16_t phase);
  bool DLL_Check();
  bool PLL_Check();
  bool I2C_check();
  bool TFC_check();
 private:
  Fpga* fpga_;
  Salt* salt_;
  FastComm* fastComm_;
  int max_commands = 20;
};
