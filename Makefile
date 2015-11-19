CC	=	g++
CFLAGS	=	-O2
SOURCES = 	*.cpp
LFLAGS 	= 	-std=c++11 -L/opt/local/lib -I/opt/local/include -I./lib -lm -lpopt -lgsl -lgslcblas

EVALFILES := $(wildcard evaluation/*.cpp)
EVALOBJS := $(addprefix evaluation/,$(notdir $(EVALFILES:.cpp=.o)))

.PHONY: all clean

all: regen dfasat

regen:
	sh collector.sh

debug:
	$(CC) -g $(SOURCES) -o dfasat $(LFLAGS) $(LIBS)

dfasat: $(EVALOBJS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $^ -I./ $(LFLAGS) $(LIBS)

evaluation/%.o: evaluation/%.cpp
	$(CC) -c -o $@ $< -I./lib $(LFLAGS) $(LIBS)


clean:
	rm -f dfasat ./evaluation/*.o
