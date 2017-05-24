#include <iostream>
#include <stddef.h>
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "Salt.h"

//#include <time.h> 
using namespace std;

class Dig_Clk_test {

 public:

  Dig_Clk_test(){};
  Dig_Clk_test(Fpga*, Salt*, int*, FastComm*);
  ~Dig_Clk_test(){};

  //Salt *st = new Salt();

  void DAQ_Sync();
  bool DLL_Check();
  bool PLL_Check();
  bool I2C_check();

};
