CC	=	g++
CFLAGS	=	-O2
SOURCES = 	*.cpp
LFLAGS 	= 	-L/opt/local/lib -I/opt/local/include -lm -lpopt -lgsl -lgslcblas

all: dfasat

debug:
	$(CC) -g $(SOURCES) -o dfasat $(LFLAGS) $(LIBS)
dfasat:
	$(CC) $(CFLAGS) $(SOURCES) -o dfasat $(LFLAGS) $(LIBS)

clean:
	rm -f dfasat

