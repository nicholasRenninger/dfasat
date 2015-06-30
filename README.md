# README #

DFASAT in C++

### What is this repository for? ###


### How do I get set up? ###

Have libpopt installed and run make. Get lingeling from http://fmv.jku.at/lingeling/ and run its build.sh

### Output files ###

DFASAT will generate several files:

* pre_dfa1.dot, is a dot file of the problem instance send to the SAT solver
* dfa1.dot is the result in dot format
* dfa1.aut is the result used by verify

You can plot the dot files via

> $ dot -Tpdf file.dot -o outfile.pdf

after installing dot from graphviz.

### Contribution guidelines ###

* fork and implement


### Who do I talk to? ###

* Sicco Verwer