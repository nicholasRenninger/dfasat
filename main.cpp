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

/*
 * Input parameters, see 'man popt'
 */

class parameters{
public:
    const char* dfa_file;
    const char* dot_file;
    const char* sat_program;
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

int main(int argc, const char *argv[]){
    
    char c = 0;
    parameters* param = new parameters();
    
    /* below parses command-line options, see 'man popt' */
    poptContext optCon;
    struct poptOption optionsTable[] = {
        { "version", 0, POPT_ARG_NONE, NULL, 1, "Display version information", NULL },
        { "seed", 's', POPT_ARG_INT, &(param->seed), 's', "Seed for random merge heuristic; default=12345678", "integer" },
        { "output file name", 'o', POPT_ARG_STRING, &(param->dot_file), 'o', "The filename in which to store the learned DFAs in .dot and .aut format, default: \"dfa\".", "string" },
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
        { "extra paremeter", 'p', POPT_ARG_FLOAT, &(param->parameter), 'p', "Extra parameter used during statistical tests, the significance level for the likelihood ratio test, the alpha value for ALERGIA, default=0.5", "float" },
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
    
    char* f2 = const_cast<char*>(poptGetArg(optCon));
    if( f2 == 0 ){
        cout << "A string specifying the \"sat_solver arg1 arg2 ..\" command is required." << endl << endl;
        exit( 1 );
    }
    param->sat_program = f2;
    
    poptFreeContext( optCon );
    
    srand(param->seed);
    
    ifstream input_stream(param->dfa_file);
    apta* the_apta = new apta(input_stream);
    state_merger merger;
    
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
    
    if (param->heuristic==1){
        evaluation_function *eval = new evaluation_function();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==2){
        evidence_driven *eval = new evidence_driven();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==3){
        depth_driven *eval = new depth_driven();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==4){
        overlap_driven *eval = new overlap_driven();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==5){
        series_driven *eval = new series_driven();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==6){
        alergia *eval = new alergia();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==7){
        likelihoodratio *eval = new likelihoodratio();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==8){
        aic *eval = new aic();
        merger = state_merger(eval,the_apta);
    }
    if (param->heuristic==9){
        kldistance *eval = new kldistance();
        merger = state_merger(eval,the_apta);
    }
   
    /* metric driven merging - addition by chrham */
     if (param->heuristic==10){
        metric_driven *eval = new metric_driven();
        merger = state_merger(eval,the_apta);
    }
	
	/* end addition */ 
	
    if(param->method == 1) GREEDY_METHOD = RANDOMG;
    if(param->method == 2) GREEDY_METHOD = NORMALG;
    
    int solution = -1;
    
    std::ostringstream oss3;
    oss3 << "init_" << param->dot_file << ".dot";
    FILE* output = fopen(oss3.str().c_str(), "w");
    merger.todot(output);
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
    
    delete param;
    delete merger.eval;
    input_stream.close();
    return 0;
}
