/*
 * The DFASAT main file, runs all the routines on an input file
 */


#include <stdlib.h>
//#include <malloc.h>
#include <popt.h>
#include "random_greedy.h"
#include "state_merger.h"
#include "evaluate.h"
#include "dfasat.h"
#include <sstream>
#include <iostream>
#include "evaluation_factory.h"
#include <string>

#include "parameters.h"

// this file is generaterated by collector.sh
// during make, so contributors don't have to
// touch anything than their own files
#include "evaluators.h"

using namespace std;

/*
 * Input parameters, see 'man popt'
 */

// map_type* BaseFactory::map = NULL;

class parameters{
public:
    string dfa_file;
    vector<string> dfa_data;
    string dot_file;
    string sat_program;
    string hName;
    string hData;
    int tries;
    int sinkson;
    int seed;
    int apta_bound;
    int dfa_bound;
    float lower_bound;
    int offset;
    int merge_sinks_p;
    int merge_sinks_d;
    int method;
    int extend;
    int heuristic;
    int symbol_count;
    int state_count;
    float correction;
    float parameter;
    int extra_states;
    int target_rejecting;
    int symmetry;
    int forcing;
    
    parameters();
};

parameters::parameters(){
    dot_file = "dfa";
    sat_program = "";
    hName = "default";
    hData = "evaluation_data";
    tries = 100;
    sinkson = 1;
    seed = 12345678;
    apta_bound = 2000;
    dfa_bound = 50;
    merge_sinks_p = 1;
    merge_sinks_d = 0;
    lower_bound = -1.0;
    offset = 5;
    method=1;
    heuristic=1;
    extend=1;
    symbol_count = 10;
    state_count = 25;
    correction = 1.0;
    parameter = 0.5;
    extra_states = 0;
    target_rejecting = 0;
    symmetry = 1;
    forcing = 0;
};



void init_with_params(parameters* param) {

    srand(param->seed);
    
    APTA_BOUND = param->apta_bound;
    CLIQUE_BOUND = param->dfa_bound;
    
    STATE_COUNT = param->state_count;
    SYMBOL_COUNT = param->symbol_count;
    CHECK_PARAMETER = param->parameter;
    CORRECTION = param->correction;

    LOWER_BOUND = param->lower_bound;
    OFFSET = param->offset;
    USE_SINKS = param->sinkson;
    MERGE_SINKS_PRESOLVE = param->merge_sinks_p;
    MERGE_SINKS_DSOLVE = param->merge_sinks_d;
    EXTEND_ANY_RED = param->extend;
    
    SYMMETRY_BREAKING = param->symmetry;
    FORCING = param->forcing;
        
    EXTRA_STATES = param->extra_states;
    TARGET_REJECTING = param->target_rejecting;
 
    eval_string = param->hData;

    try {
       (DerivedRegister<evaluation_function>::getMap())->at(param->hName);
       std::cout << "valid: " << param->hName << std::endl;
       
    } catch(const std::out_of_range& oor ) {
       std::cout << "invalid: " << param->hName << std::endl;
    }
}


void run(parameters* param) {

    state_merger merger;

    init_with_params(param);
    
    evaluation_function *eval;

    cout << "getting data" << endl;
    try {
       eval = (DerivedRegister<evaluation_function>::getMap())->at(param->hName)();
       std::cout << "Using heuristic " << param->hName << std::endl;
       
    } catch(const std::out_of_range& oor ) {
       std::cerr << "No named heuristic found, defaulting back on -h flag" << std::endl;
    }
   
    cout << "creating apta " <<  "using " << eval_string << endl; 
    apta* the_apta = new apta();
    merger = state_merger(eval,the_apta);
    
    
    ifstream input_stream(param->dfa_file);
    merger.read_apta(input_stream);

    if(param->method == 1) GREEDY_METHOD = RANDOMG;
    if(param->method == 2) GREEDY_METHOD = NORMALG;
    
    int solution = -1;
    
    std::ostringstream oss3;
    oss3 << "init_" << param->dot_file << ".dot";
    FILE* output = fopen(oss3.str().c_str(), "w");
    merger.todot();
    merger.print_dot(output);
    fclose(output);
    
    for(int i = 0; i < param->tries; ++i){
        std::ostringstream oss;
        oss << param->dot_file << (i+1) << ".aut";
        std::ostringstream oss2;
        oss2 << param->dot_file << (i+1) << ".dot";
        solution = dfasat(merger, param->sat_program, oss2.str().c_str(), oss.str().c_str());
        if(solution != -1)
            CLIQUE_BOUND = min(CLIQUE_BOUND, solution - OFFSET + EXTRA_STATES);
    }
    input_stream.close();
}


int main(int argc, const char *argv[]){
    
    char c = 0;
    parameters* param = new parameters();
    
    /* temporary holder for string arguments */
    char* dot_file = NULL;
    char* sat_program = NULL;
    char* hName;
    char* hData;
    
    /* below parses command-line options, see 'man popt' */
    poptContext optCon;
    struct poptOption optionsTable[] = {
        { "version", 0, POPT_ARG_NONE, NULL, 1, "Display version information", NULL },
        { "seed", 's', POPT_ARG_INT, &(param->seed), 's', "Seed for random merge heuristic; default=12345678", "integer" },
        { "output file name", 'o', POPT_ARG_STRING, &dot_file, 'o', "The filename in which to store the learned DFAs in .dot and .aut format, default: \"dfa\".", "string" },
	{ "heuristic-name", 'q', POPT_ARG_STRING, &hName, 'q', "Name of the merge heurstic to use; will default back on -p flag if not specified.", "string" },
{ "data-name", 'z', POPT_ARG_STRING, &hData, 'z', "Name of the merge data class to use.", "string" },
        { "runs", 'n', POPT_ARG_INT, &(param->tries), 'n', "Number of DFASAT runs/iterations; default=100", "integer" },
        {"sink states", 'i', POPT_ARG_INT, &(param->sinkson), 'i', "Set to 1 to use sink states, 0 to consider all states", "integer"},
        { "apta bound", 'b', POPT_ARG_INT, &(param->apta_bound), 'b', "Maximum number of remaining states in the partially learned DFA before starting the SAT search process. The higher this value, the larger the problem sent to the SAT solver; default=2000", "integer" },
        { "dfa bound", 'd', POPT_ARG_INT, &(param->dfa_bound), 'd', "Maximum size of the partially learned DFA before starting the SAT search process; default=50", "integer" },
        { "lower bound", 'l', POPT_ARG_FLOAT, &(param->lower_bound), 'l', "Minimum value of the heuristic function, smaller values are treated as inconsistent, also used as the paramter value in any statistical tests; default=-1", "float" },
        { "extra states", 'e', POPT_ARG_INT, &(param->offset), 'e', "DFASAT runs a SAT solver to find a solution of size at most the size of the partially learned DFA + e; default=5", "integer" },
        { "additional states", 'a', POPT_ARG_INT, &(param->extra_states), 'a', "With every iteration, DFASAT tries to find solutions of size at most the best solution found + a, default=0", "int" },
        { "merge sinks during greedy", 'u', POPT_ARG_INT, &(param->merge_sinks_d), 'u', "Sink nodes are candidates for merging during the greedy runs (setting 0 or 1); default=0", "integer" },
        { "merge sinks presolve", 'r', POPT_ARG_INT, &(param->merge_sinks_p), 'r', "Merge all sink nodes (setting 0 or 1) before sending the problem to the SAT solver; default=1", "integer" },
        { "target rejecting sink", 'j', POPT_ARG_INT, &(param->target_rejecting), 'j', "Make all transitions from red states without any occurrences target the rejecting sink (setting 0 or 1) before sending the problem to the SAT solver; default=0", "integer" },
        { "symmetry breaking", 'k', POPT_ARG_INT, &(param->symmetry), 'k', "Add symmetry breaking predicates to the SAT encoding (setting 0 or 1), based on Ulyantsev et al. BFS symmetry breaking; default=1", "integer" },
        { "transition forcing", 'f', POPT_ARG_INT, &(param->forcing), 'f', "Add predicates to the SAT encoding that force transitions in the learned DFA to be used by input examples (setting 0 or 1); default=0", "integer" },
        { "extend any red", 'x', POPT_ARG_INT, &(param->extend), 'r', "During greedy runs any merge candidate (blue) that cannot be merged with any (red) target is immediately changed into a (red) target; default=1. If set to 0, a merge candidate is only changed into a target when no more merges are possible.", "integer" },
        { "method", 'm', POPT_ARG_INT, &(param->method), 'm', "Method to use during the greedy preprocessing, default value 1 is random greedy (used in Stamina winner), 2 is one non-randomized greedy", "integer" },
        { "heuristic", 'h', POPT_ARG_INT, &(param->heuristic), 'h', "Heuristic to use during the greedy preprocessing, default value 1 counts the number of merges, 2 is EDSM (evidence driven state merging), 3 is shallow first (like RPNI), 4 counts overlap in merged positive transitions (used in Stamina winner), 5 is the overlap method for time series/flows (no loops or back-arcs), 6 is ALERGIA consistency check with shallow first, 7 computes a likelihoodratio test for score and consistency (like RTI algorithm), 8 computes the Akaike Information Criterion, 9 computes the Kullback-Leibler divergence (based on MDI algorithm). Statistical tests are computed only on positive traces.", "integer" },
        { "state count", 't', POPT_ARG_INT, &(param->state_count), 't', "The minimum number of positive occurrences of a state for it to be included in overlap/statistical checks, default=25", "integer" },
        { "symbol count", 'y', POPT_ARG_INT, &(param->symbol_count), 'y', "The minimum number of positive occurrences of a symbol/transition for it to be included in overlap/statistical checks, symbols with less occurrences are binned together, default=10", "integer" },
        { "correction", 'c', POPT_ARG_FLOAT, &(param->correction), 'c', "Value of a Laplace correction (smoothing) added to all symbol counts when computing statistical tests (in ALERGIA, LIKELIHOODRATIO, AIC, and KULLBACK-LEIBLER), default=1.0", "float" },
        { "extra parameter", 'p', POPT_ARG_FLOAT, &(param->parameter), 'p', "Extra parameter used during statistical tests, the significance level for the likelihood ratio test, the alpha value for ALERGIA, default=0.5", "float" },
        { "solver", 'S', POPT_ARG_STRING, &sat_program, 'S', "Path to the program used to solve the problem, default=none", "string" },
        POPT_AUTOHELP
        POPT_TABLEEND
    };
    optCon = poptGetContext(NULL, argc, (const char**)argv, optionsTable, 0);
    poptSetOtherOptionHelp(optCon, "[OPTIONS]* [input dfa file]");
    
    while ((c = poptGetNextOpt(optCon)) >= 0){
        if(c == 1){
            cout << endl << "DFASAT with random greedy preprocessing" << endl;
            cout << "Copyright 2015 Sicco Verwer and Marijn Heule, Delft University of Technology." << endl;
            exit( 1 );
        }
    }
    if( c < -1 ){
        cerr << poptBadOption( optCon, POPT_BADOPTION_NOALIAS ) << ": " << poptStrerror(c) << endl;
        exit( 1 );
    }
    
    char* f = const_cast<char*>(poptGetArg(optCon));
    if( f == 0 ){
        cout << "A DFA learning input file in Abbadingo format is required." << endl << endl;
        exit( 1 );
    }
    param->dfa_file = f;
    
    while ((c = poptGetNextOpt(optCon)) >= 0);
    if( c < -1 ){
        cerr << poptBadOption( optCon, POPT_BADOPTION_NOALIAS ) << ": " << poptStrerror(c) << endl;
        exit( 1 );
    }
    
    if(dot_file != NULL)
        param->dot_file = dot_file;

    if(sat_program != NULL)
        param->sat_program = sat_program;

    param->hName = hName;
    param->hData = hData;

    //poptFreeContext( optCon );
   
    run(param); 
    
    delete param;
    
    return 0;    
}
