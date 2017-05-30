# README #

flexfringe (formerly DFASAT), a flexible state-merging framework written in C++.

### What is this repository for? ###

You can issue pull requests for bug fixes and improvements. Most work will happen in the development branch while master contains a more stable version.

### How do I get set up? ###

flexfringe has one required dependency: libpopt for argument parsing. Some heuristic functions bring their own dependencies. We provide an implementation of a likelihood-based merge function for probabilistic DFAs. It needs the GNU scientific library (development) package (e.g. the libgsl-dev package in Ubuntu).
 
If you want to use the reduction to SAT and automatically invoke the SAT solver, you need to provide the path to the solver binary. flexfringe has been tested with lingeling (which you can get from http://fmv.jku.at/lingeling/ and run its build.sh).

You can build and compile the flexfringe project by running

$ make clean all

in the main directory to build the executable named /flexfringe/.


### How do I run it? ###

Run ./flexfringe --help to get help.

The start.sh script together with some .ini files provides a shortcut to storing 

Example:

$ ./start.sh mealy-batch.ini data/simple.traces 

See the .ini files for documentation of parameter flags. 


### Output files ###

flexfringe will generate several .dot files into the specified output directory (. by default):

* pre\:\*.dot are intermediary dot files created during the merges/search process.
* final.dot is the end result

You can plot the dot files via

> $ dot -Tpdf file.dot -o outfile.pdf
or
> $ ./show.sh final.dot

after installing dot from graphviz.

### Contribution guidelines ###

* Fork and implement, request pulls.
* You can find sample evaluation files in ./evaluation. Make sure to REGISTER your own file to be able to access it via the -h flag.

### Who do I talk to? ###

* Sicco Verwer (original author; best to reach out to for questions on batch mode and SAT reduction)
* Christian Hammerschmidt (author of the online/streaming mode and interactive mode)
