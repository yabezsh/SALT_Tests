#ifndef REGISTERS_CONFIG_H
#define REGISTERS_CONFIG_H

namespace registers
{
// General FPGA Configuration    
    const int RESET          = 0x50000;
    const int I2C_ADD        = 0x50001;
    const int POWER_CFG      = 0x50002;
    const int RESERVED       = 0x50003;

// TFC Configuration
    const int TFC_CFG        = 0x52000;
    const int TFC_LENGTH     = 0x52001;
    const int TFC_PERIOD0    = 0x52002;
    const int TFC_PERIOD1    = 0x52003;
    const int TFC_WR         = 0x52004;
    const int TFC_TRIGGER    = 0x52005;
    
//DAQ Registers
    const int DAQ_CFG        = 0x51000;
    const int DAQ_FIFO_L     = 0x51001;
    const int DAQ_FIFO_H     = 0x51002;
    
    const int DAQ_DELAY      = 0x51003;
    const int DAQ_ACQ_L      = 0x51004;
    const int DAQ_ACQ_H      = 0x51005;
    const int DAQ_TRIGGER    = 0x51006;
    const int DAQ_READ0      = 0x51007;
    const int DAQ_READ1      = 0x51008;
    const int DAQ_READ2      = 0x51009;
    const int DAQ_READ3      = 0x5100A;    
    const int DAQ_READ4      = 0x5100B;    

// TODO: DAQ PLL System
    
}

#endif // REGISTERS_CONFIG_H