#pragma once

#include "Fpga.h"

//#include <time.h> 
using namespace std;

class FastComm {

 public:
  FastComm(){};
  FastComm(Fpga*);
  ~FastComm(){};

  void write_tfc(uint8_t length, uint8_t command[], int period, bool singleShot);
  void read_daq(uint32_t clock_delay, uint32_t length, int trigger, uint8_t (&packet)[5120]);
   void read_daq(uint8_t clock_delay, uint8_t length, int trigger, uint32_t (&packet)[5120]);

 private:
    Fpga *fpga_;

};
