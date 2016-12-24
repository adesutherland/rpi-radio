# Makefile
CCFLAGS = -Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall  -lvlc -lwiringPi -lpthread -lArduiPi_OLED

PROGRAMS = displaytime
SOURCES = ${PROGRAMS:=.cpp}

all: ${PROGRAMS}

${PROGRAMS}: ${SOURCES}
	g++ ${CCFLAGS} $@.cpp -o $@

INSTALL:
	-sudo killall displaytime
	sudo cp displaytime /usr/local/bin
	sudo /usr/local/bin/displaytime &	

clean:
	rm -rf $(PROGRAMS)



