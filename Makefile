CC	=	g++
CFLAGS	=	-O2
SOURCES = 	*.cpp
SOURCESPYTHON =	apta.cpp dfasat.cpp  evaluation_factory.cpp random_greedy.cpp  state_merger.cpp parameters.cpp 
LFLAGS 	= 	-std=c++11 -L/opt/local/lib -I/opt/local/include -I./lib -I. -lm -lpopt -lgsl -lgslcblas
PYTHON_EVAL = evaluation/python.cpp

EVALFILES := $(wildcard evaluation/*.cpp)
EVALOBJS := $(addprefix evaluation/,$(notdir $(EVALFILES:.cpp=.o)))

ifdef WITH_PYTHON
  PYTHON_VERSION=$(shell python3 -c 'import sys; print("".join([str(v) for v in sys.version_info[:2]]))')
  PYTHON_INC=$(shell python3-config --includes)
  PYTHON_LIBS=$(shell python3-config --libs)
  BOOST_LIBS=-lboost_python-py$(PYTHON_VERSION)
else
  EVALFILES := $(filter-out $(PYTHON_EVAL), $(EVALFILES))
  EVALOBJS := $(addprefix evaluation/,$(notdir $(EVALFILES:.cpp=.o)))
endif


OUTDIR ?= .

.PHONY: all clean

all: regen dfasat

regen:
	sh collector.sh

debug:
	$(CC) -g $(SOURCES) -o dfasat $(LFLAGS) $(LIBS)

dfasat: $(EVALOBJS)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $^ -I./ $(LFLAGS) $(LIBS)

evaluation/%.o: evaluation/%.cpp
	$(CC) -fPIC -c -o $@ $< -I./lib $(LFLAGS) $(LIBS) $(PYTHON_INC) $(PYTHON_LIBS) $(BOOST_LIBS) 

clean:
	rm -f dfasat ./evaluation/*.o generated.cpp named_tuple.py *.dot exposed_decl.pypp.txt dfasat*.so
	

python: $(EVALOBJS)
	$(CC) -fPIC -shared $(CFLAGS)  -o dfasat.lib.so $(SOURCESPYTHON) $^ -I./ $(LFLAGS) $(LIBS) $(PYTHON_LIBS) $(PYTHON_INC) 
	python3 generate.py
	g++ -W  -g -Wall -fPIC -shared generated.cpp dfasat.lib.so -o dfasat.so $(PYTHON_LIBS) $(PYTHON_INC) $(BOOST_LIBS) $(LFLAGS) -Wl,-rpath,'$$ORIGIN' -L. -l:dfasat.lib.so
	mv dfasat.lib.so dfasat.so $(OUTDIR)
