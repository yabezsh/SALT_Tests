#pragma once

#include "Fpga.h"

//#include <time.h> 
using namespace std;

class FastComm {

 public:
  FastComm(){};
  FastComm(Fpga*);
  ~FastComm(){};

  void write_tfc();
  void read_daq(uint32_t clock_delay, uint32_t length, int trigger, uint8_t (&packet)[5120]);
  void read_daq(uint8_t length, uint32_t (&packet)[5120], bool tfc_trig);
  void read_daq(uint8_t length, uint32_t (&packet)[5120]);
  // void read_daq(uint8_t length, uint64_t (&packet)[5120]);
  string read_daq(uint8_t length);
  string arrange_Elinks(uint64_t data);
  void Take_a_run(uint16_t length_read, uint32_t (&packet)[5120], uint8_t length_write, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig);
  void Take_a_run(uint16_t length_read, uint64_t (&packet)[5120], uint8_t length_write, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig);
 void Take_a_run(uint16_t length_read, string &data, uint8_t length_write, uint8_t clock_delay, uint8_t command[], uint8_t period, bool singleShot, bool tfc_trig);
  void Launch_ACQ(bool tfc_trig);  
  
   void config_daq(uint16_t length, uint8_t clock_delay, bool tfc_trig);
   
   void config_tfc(uint8_t length, uint8_t command[], uint8_t period, bool singleShot);

   void reset_DAQ();
   unsigned read_twelveBits(string data, int i);
   void read_Header(unsigned twelveBits, int &bxid, int &parity, int &flag, int &length);
   void read_Normal_packet(string data, int startBit, int (&ADC)[128]);
   void read_NZS_packet(string data, int startBit, int (&ADC)[128], int &bxid, int &parity, int &mcm_v, int &mcm_ch, int &mem_space);

   void trigger_DAQ(bool tfc_trig);
   
 private:
    Fpga *fpga_;

};
