# Copyright 2020 Nicholas Renninger
# Copyright 2019 Sicco E. Verwer & Christian A. Hammerschmidt
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

CC	=	g++
CFLAGS	=	-O2 -g 
SOURCES := 	$(wildcard *.cpp)
SOURCESOBJS := $(addprefix obj/, $(SOURCES:.cpp=.o))
SOURCESPYTHON =	apta.cpp dfasat.cpp  refinement.cpp evaluation_factory.cpp random_greedy.cpp  state_merger.cpp parameters.cpp searcher.cpp stream.cpp interactive.cpp 
LFLAGS 	= 	-std=c++11 -L/opt/local/lib -I/opt/local/include -I./lib -I. -lm -lpopt -lgsl -lgslcblas
PYTHON_EVAL = evaluation/python.cpp

EVALFILES := $(wildcard evaluation/*.cpp)

ifdef WITH_PYTHON
  PYTHON_VERSION=$(shell python3 -c 'import sys; print("".join([str(v) for v in sys.version_info[:2]]))')
  PYTHON_INC=$(shell python3-config --includes)
  PYTHON_LIBS=$(shell python3-config --libs)
  BOOST_LIBS=-lboost_python-py$(PYTHON_VERSION)
else
  EVALFILES := $(filter-out $(PYTHON_EVAL), $(EVALFILES))
endif

EVALOBJS := $(addprefix evaluation/obj/,$(notdir $(EVALFILES:.cpp=.o)))


OUTDIR ?= .

.PHONY: all clean

all: regen gitversion.cpp flexfringe

regen:
	sh collector.sh
	mkdir -p obj
	mkdir -p evaluation/obj

debug:
	$(CC) -g $(SOURCES) -o flexfringe $(LFLAGS) $(LIBS)

flexfringe: $(SOURCESOBJS) $(EVALOBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS) $(LIBS)

obj/%.o: %.cpp
	$(CC) -fPIC -g -c -o $@ $^ $(LFLAGS) $(LIBS)

evaluation/obj/%.o: evaluation/%.cpp
	$(CC) -fPIC -g -c -o $@ $^ -I./evaluation/lib $(LFLAGS) $(LIBS) $(PYTHON_INC) $(PYTHON_LIBS) $(BOOST_LIBS)

clean:
	rm -f flexfringe ./obj/*.o ./evaluation/obj/*.o generated.cpp named_tuple.py *.dot exposed_decl.pypp.txt flexfringe*.so gitversion.cpp
	
gitversion.cpp: ../.git/modules/dfasat/HEAD ../.git/modules/dfasat/index
	echo "const char *gitversion = \"$(shell git rev-parse HEAD)\";" > $@
	$(CC) -fPIC -c -o obj/gitversion.o $(LFLAGS) $(LIBS)

python: $(EVALOBJS) gitversion.cpp
	export CPLUS_INCLUDE_PATH=/usr/include/python3.5
	$(CC) -fPIC -shared $(CFLAGS)  -o flexfringe.lib.so $(SOURCESPYTHON) $^ -I./ $(LFLAGS) $(LIBS) $(PYTHON_LIBS) $(PYTHON_INC) 
	python3 generate.py
	g++ -W  -g -Wall -fPIC -shared generated.cpp flexfringe.lib.so -o flexfringe.so $(PYTHON_LIBS) $(PYTHON_INC) $(BOOST_LIBS) $(LFLAGS) -Wl,-rpath,'$$ORIGIN' -L. -l:flexfringe.lib.so
	mv flexfringe.lib.so flexfringe.so $(OUTDIR)
