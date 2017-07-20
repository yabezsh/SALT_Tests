CC=arm-linux-gnueabihf-g++
CFLAGS= -Wall  -std=c++11 -Dsoc_cv_av -g 
INCLUDES= -I/home/iaro/intelFPGA/16.1/embedded/ip/altera/hps/altera_hps/hwlib/include -I/home/iaro/intelFPGA/16.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av -I/home/iaro/intelFPGA/16.1/embedded/ip/altera/hps/altera_hps/hwlib/include/soc_cv_av/socal/
SRCS= main.cpp I2C.cpp Fpga.cpp CurrentMonitor.cpp ExternalADC.cpp Salt.cpp
OBJS=$(SRCS:.cpp=.o)
MAIN=main

.PHONY= depend clean
all: $(MAIN)
	@echo  Congratulation! Compilation succeeded! 
$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS)

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c  $< -o $@
clean:
	rm *.o

