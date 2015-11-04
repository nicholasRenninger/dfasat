CC	=	g++
CFLAGS	=	-O2
SOURCES = 	*.cpp
LFLAGS 	= 	-std=c++11 -L/opt/local/lib -I/opt/local/include -I./lib -lm -lpopt -lgsl -lgslcblas

all: dfasat

debug:
	$(CC) -g $(SOURCES) -o dfasat $(LFLAGS) $(LIBS)
dfasat: evaluation/evaluation.o
	$(CC) $(CFLAGS) -o dfasat $(SOURCES) evaluation/evaluation.o $(LFLAGS) $(LIBS)
evaluation/evaluation.o:
	$(CC) -c ./evaluation/*.cpp -o $@  -I./lib $(LIBS) $(LFLAGS)
	
clean:
	rm -f dfasat ./evaluation/*.o

