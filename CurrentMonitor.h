//CurrentMonitor.h
#pragma once

#include "I2C.h"

class CurrentMonitor : public I2C { 
    public:
      //  CurrentMonitor(){};
        CurrentMonitor(int8_t, int8_t);
        ~CurrentMonitor();
        
        void configure();
        void read_current(uint16_t *);

    private:
        uint8_t bit_15_8;
        uint8_t bit_7_0;
    
};