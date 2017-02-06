# RPI Radio
TARGET = displaytime
OBJS += displaytime.o volume.o
PAMIXEROBJS += pamixer/pulseaudio.o pamixer/device.o pamixer/callbacks.o

CFLAGS += -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall 

CXXFLAGS += -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall 

LDFLAGS +=

LDLIBS += -lvlc -lwiringPi -lpthread -lArduiPi_OLED
LDLIBS += -lpulse -lboost_program_options

all:	pamixer-project $(TARGET)

pamixer-project:
	make -C pamixer

displaytime.o: displaytime.cpp rpi-radio.h	

volume.o: volume.cpp rpi-radio.h

$(TARGET):	$(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ $(PAMIXEROBJS) $(LDLIBS) -o $@
	
clean:
	make -C pamixer clean
	rm -f *.o *~ $(TARGET)

INSTALL:
	-sudo killall $(TARGET)
	sudo cp $(TARGET) /usr/local/bin
	sudo /usr/local/bin/$(TARGET) &	
