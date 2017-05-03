#pragma once

#include "Fpga.h"

//#include <time.h> 
using namespace std;

class FastComm {

 public:
  FastComm(){};
  FastComm(Fpga*);
  ~FastComm(){};

  void write_tfc(uint32_t length, uint32_t command[], int period, bool singleShot);
  void read_daq(int clock_delay, int length, int trigger, uint32_t *packet);

 private:
    Fpga *fpga_;

};
