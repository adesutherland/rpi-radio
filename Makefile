# RPI Radio
TARGET = rpi-radio
OBJS += rpi-radio.o volume.o rotarycontrol.o shared/displaylogic.o remote.o
OBJS += abstractmodule.o vlcradio.o rpidisplay.o controller.o
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

all:	pamixer-project spotify-module $(TARGET) arduino

pamixer-project:
	make -C pamixer
	
spotify-module:
	make -C spotify

rpi-radio.o: rpi-radio.cpp rpi-radio.h shared/displaylogic.h remote.h

controller.o: controller.cpp rpi-radio.h shared/displaylogic.h

rotarycontrol.o: rotarycontrol.cpp rpi-radio.h

volume.o: volume.cpp rpi-radio.h

shared/displaylogic.o: shared/displaylogic.cpp shared/displaylogic.h rpi-radio.h 

remote.o: remote.cpp shared/displaylogic.h rpi-radio.h remote.h

rpidisplay.o: rpidisplay.cpp shared/displaylogic.h

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
	make -C spotify clean
	rm -f *.o *~ $(TARGET)
	rm -f shared/*.o shared/*~
	rm -f configFiles/*~
	rm -f slave/displaylogic.h
	rm -f slave/displaylogic_imp.h

INSTALL:
	-sudo killall $(TARGET)
	sudo cp $(TARGET) /usr/local/bin
	sudo cp configFiles/rc.local /etc
	sudo cp configFiles/asound.conf /etc
	sudo cp configFiles/daemon.conf /etc/pulse
	sudo /usr/local/bin/$(TARGET) &	
