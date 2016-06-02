# README #

DFASAT in C++

### What is this repository for? ###



### How do I get set up? ###

Have libpopt installed and run make clean all. If you want to use the SAT-solving part of it, get lingeling from http://fmv.jku.at/lingeling/ and run its build.sh, or a similar solver.

### How do I run it? ###

Run ./dfasat --help to get help.

Example:

$ ./dfasat -q alergia -z alergia_data -n 1 -b 0 -d 2000 inputfile "/bin/true"

q defines the algorithm, and z the data type. n defines the number of iterations, which is useful if you use the SAT-solver. -b and -d a used to decide when to switch from the heuristic to the SAT-solver. The parameters in the example essentially prevent the use of the solver.

The thresholds for relevance of states and symbols can be adjusted using -t and -y.

### Output files ###

DFASAT will generate several files:

* pre_dfa1.dot, is a dot file of the problem instance send to the SAT solver
* dfa1.dot is the result in dot format if you provided a SAT-solver

You can plot the dot files via

> $ dot -Tpdf file.dot -o outfile.pdf

after installing dot from graphviz.

### Contribution guidelines ###

* Fork and implement, request pulls.
* You can find sample evaluation files in ./evaluation. Make sure to REGISTER your own file to be able to access it via the -q flag.

### Who do I talk to? ###

* Sicco Verwer
* one of his PhD students: Qin, Nino, or Chris