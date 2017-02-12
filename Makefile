# RPI Radio
TARGET = displaytime
OBJS += displaytime.o volume.o rotarycontrol.o shared/displaylogic.o remote.o
OBJS += abstractmodule.o vlcradio.o
PAMIXEROBJS += pamixer/pulseaudio.o pamixer/device.o pamixer/callbacks.o

#CFLAGS += -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s 
#CXXFLAGS += -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s 

CFLAGS += -g 
CXXFLAGS += -g 


CFLAGS += -DRPI
CXXFLAGS += -DRPI 

CFLAGS += -Wall 
CXXFLAGS += -Wall 


LDFLAGS +=

LDLIBS += -lvlc -lwiringPi -lpthread -lArduiPi_OLED
LDLIBS += -lpulse -lboost_program_options

all:	pamixer-project $(TARGET) arduino

pamixer-project:
	make -C pamixer

displaytime.o: displaytime.cpp rpi-radio.h shared/displaylogic.h remote.h

rotarycontrol.o: rotarycontrol.cpp rpi-radio.h

volume.o: volume.cpp rpi-radio.h

shared/displaylogic.o: shared/displaylogic.cpp shared/displaylogic.h

remote.o: remote.cpp shared/displaylogic.h rpi-radio.h remote.h

abstractmodule.o: abstractmodule.cpp rpi-radio.h

vlcradio.o: vlcradio.cpp rpi-radio.h

$(TARGET):	$(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ $(PAMIXEROBJS) $(LDLIBS) -o $@

arduino:
	cp shared/displaylogic.h slave/displaylogic.h
	cp shared/displaylogic.cpp slave/displaylogic_imp.h

test:
	-sudo killall $(TARGET)
	-sudo ./$(TARGET)

stoptesting:
	-sudo killall $(TARGET)
	sudo /usr/local/bin/$(TARGET) &	

clean:
	make -C pamixer clean
	rm -f *.o *~ $(TARGET)
	rm -f shared/*.o shared/*~ $(TARGET)
	rm -f slave/displaylogic.h
	rm -f slave/displaylogic_imp.h

INSTALL:
	-sudo killall $(TARGET)
	sudo cp $(TARGET) /usr/local/bin
	sudo /usr/local/bin/$(TARGET) &	
