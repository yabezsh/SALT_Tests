//Salt.h
#pragma once

#include "I2C.h"

class Salt : public I2C {
    public:
        Salt();
        ~Salt();
        void find_device_address(int8_t);
        void read_salt(int8_t, int8_t, uint8_t*);
    private:
        int8_t chip_id;

    
};