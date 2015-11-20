# README #

DFASAT in C++

### What is this repository for? ###



### How do I get set up? ###

Have libpopt installed and run make. Get lingeling from http://fmv.jku.at/lingeling/ and run its build.sh

### How do I run it? ###

Run ./dfasat --help to get help.

Example:

$ ./dfasat -h 4 inputfile "./lingeling"

runs DFASAT using an overlap-driven merge heuristic as in the Stamina paper on the input file "inputfile" and uses lingeling as a SAT solver.

The thresholds for relevance of states and symbols can be adjusted using -t and -y.

You can also access the evaluation heuristics using the -q flag

$ ./dfasat -q mse_error inputfile "./lingeling"


### Output files ###

DFASAT will generate several files:

* pre_dfa1.dot, is a dot file of the problem instance send to the SAT solver
* dfa1.dot is the result in dot format
* dfa1.aut is the result used by verify

You can plot the dot files via

> $ dot -Tpdf file.dot -o outfile.pdf

after installing dot from graphviz.

### Contribution guidelines ###

* Fork and implement, request pulls.
* You can find sample evaluation files in ./evaluation. Make sure to REGISTER your own file to be able to access it via the -q flag.

### Who do I talk to? ###

* Sicco Verwer
* one of his PhD students