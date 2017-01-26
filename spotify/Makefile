TARGET = spotifyplayer
OBJS += spotifyplayer.o session.o workflow.o event.o getmacaddress.o soundiodriver.o dummydriver.o

CFLAGS  += -g
CFLAGS  += $(shell pkg-config --cflags libspotify)

CXXFLAGS  += -g
CXXFLAGS  += $(shell pkg-config --cflags libspotify)

LDFLAGS += $(shell pkg-config --libs-only-L libspotify)
LDLIBS  += $(shell pkg-config --libs-only-l --libs-only-other libspotify)

LDLIBS  += -lpthread 

CFLAGS += -Wall
CXXFLAGS += -Wall

all:	$(TARGET)

spotifyplayer.o: spotifyplayer.cpp spotifywrapper.h soundiodriver.h dummydriver.h

soundiodriver.o: soundiodriver.cpp spotifywrapper.h soundiodriver.h

dummydriver.o: dummydriver.cpp spotifywrapper.h dummydriver.h

session.o: session.cpp spotifywrapper.h

workflow.o: workflow.cpp spotifywrapper.h

event.o: event.cpp spotifywrapper.h

$(TARGET):	$(OBJS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
	
clean:
	rm -f *.o *~ $(TARGET)
