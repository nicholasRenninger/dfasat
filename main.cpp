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
#include "searcher.h"

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
    int runs;
    int sinkson;
    int seed;
    int sataptabound;
    int satdfabound;
    float lower_bound;
    int satextra;
    int mergesinks;
    int satmergesinks;
    int method;
    int extend;
    int heuristic;
    int symbol_count;
    int state_count;
    float correction;
    float extrapar;
    int satplus;
    int satfinalred;
    int symmetry;
    int forcing;
    int blueblue;
    int finalred;
    int largestblue;
    int testmerge;
    int shallowfirst;
    string mode;    
    parameters();
};

parameters::parameters(){
    mode = "batch";
    dot_file = "dfa";
    sat_program = "";
    hName = "default";
    hData = "evaluation_data";
    runs = 1;
    sinkson = 1;
    seed = 12345678;
    sataptabound = 0;
    satdfabound = 50;
    mergesinks = 1;
    satmergesinks = 0;
    lower_bound = -1.0;
    satextra = 5;
    method=1;
    heuristic=1;
    extend=1;
    symbol_count = 10;
    state_count = 25;
    correction = 0.0;
    extrapar = 0.5;
    satplus = 0;
    satfinalred = 0;
    symmetry = 1;
    forcing = 0;
    blueblue = 0;
    finalred = 0;
    largestblue = 0;
    testmerge = 0;
    shallowfirst =0;
};



void init_with_params(parameters* param) {

    srand(param->seed);
    
    APTA_BOUND = param->sataptabound;
    CLIQUE_BOUND = param->satdfabound;
    
    STATE_COUNT = param->state_count;
    SYMBOL_COUNT = param->symbol_count;
    CHECK_PARAMETER = param->extrapar;
    CORRECTION = param->correction;

    LOWER_BOUND = param->lower_bound;
    OFFSET = param->satextra;
    USE_SINKS = param->sinkson;
    MERGE_SINKS_PRESOLVE = param->mergesinks;
    MERGE_SINKS_DSOLVE = param->satmergesinks;
    EXTEND_ANY_RED = param->extend;
    
    SYMMETRY_BREAKING = param->symmetry;
    FORCING = param->forcing;
        
    EXTRA_STATES = param->satplus;
    TARGET_REJECTING = param->satfinalred;

    /* new since master-dev merge */
    MERGE_MOST_VISITED = param->largestblue;
    MERGE_BLUE_BLUE = param->blueblue;
    RED_FIXED = param->finalred;
    MERGE_WHEN_TESTING = !param->testmerge;
    DEPTH_FIRST = param->shallowfirst;


    if(param->method == 1) GREEDY_METHOD = RANDOMG;
    if(param->method == 2) GREEDY_METHOD = NORMALG;
 
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
    
    for(auto myit = DerivedRegister<evaluation_function>::getMap()->begin(); myit != DerivedRegister<evaluation_function>::getMap()->end(); myit++   ) {
       cout << myit->first << " " << myit->second << endl;
    }

    cout << "getting data" << endl;
    try {
       eval = (DerivedRegister<evaluation_function>::getMap())->at(param->hName)();
       std::cout << "Using heuristic " << param->hName << std::endl;
       
    } catch(const std::out_of_range& oor ) {
       std::cerr << "No named heuristic found, defaulting back on -h flag" << std::endl;
    }

    apta* the_apta = new apta();
    merger = state_merger(eval,the_apta);

    cout << "creating apta " <<  "using " << eval_string << endl; 
    
    if(param->mode == "batch") {
       cout << "batch mode selected" << endl;  
       ifstream input_stream(param->dfa_file);
       merger.read_apta(input_stream);
   
       input_stream.close();
     } else {
       /* this is the outline for streaming mode  */ 
       cout << "stream mode selected" << endl;
    
       ifstream input_stream(param->dfa_file);

       // first line has alphabet size and 
       std::string line;
       std::getline(input_stream, line);
       merger.init_apta(line);

       // line by line processing 
       // add items       
       while (std::getline(input_stream, line)) {
          merger.advance_apta(line, &merger);
          // cout << merger.todot();
	  // cout << merger.dot_output;
          // do work 
       }
    }

    /* sanity check: stream and batch produce same output? */
    cout << "Read data finished, processing:" << endl;
    // run the state merger
    int solution = -1;
    
    std::ostringstream oss3;
    oss3 << "init_" << param->dot_file << ".dot";
    FILE* output = fopen(oss3.str().c_str(), "w");
    merger.todot();
    merger.print_dot(output);
    fclose(output);
    
    for(int i = 0; i < param->runs; ++i){
       std::ostringstream oss;
       oss << param->dot_file << (i+1) << ".aut";
       std::ostringstream oss2;
       oss2 << param->dot_file << (i+1) << ".dot";
       cout << merger.seen.size() << endl;
       solution = dfasat(merger, param->sat_program, oss2.str().c_str(), oss.str().c_str());
       //bestfirst(&merger);
       if(solution != -1)
          CLIQUE_BOUND = min(CLIQUE_BOUND, solution - OFFSET + EXTRA_STATES);
      }
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
/*        { "seed", 's', POPT_ARG_INT, &(param->seed), 's', "Seed for random merge heuristic; default=12345678", "integer" },
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
=======*/
        { "output file name", 'o', POPT_ARG_STRING, &(dot_file), 'o', "The filename in which to store the learned DFAs in .dot and .aut format, default: \"dfa\".", "string" },
        { "heuristic-name", 'h', POPT_ARG_STRING, &(hName), 'h', "Name of the merge heurstic to use; default count_driven. Use any heuristic in the evaluation directory. It is often beneficial to write your own, as heuristics are very application specific.", "string" },
        { "data-name", 'd', POPT_ARG_STRING, &(hData), 'd', "Name of the merge data class to use; default count_data. Use any heuristic in the evaluation directory.", "string" },
        { "mode", 'M', POPT_ARG_STRING, &(param->mode), 'M', "batch or stream depending on the mode of operation", "string" },
        { "method", 'm', POPT_ARG_INT, &(param->method), 'm', "Method to use when merging states, default value 1 is random greedy (used in Stamina winner), 2 is one standard (non-random) greedy.", "integer" },
        { "seed", 's', POPT_ARG_INT, &(param->seed), 's', "Seed for random merge heuristic; default=12345678", "integer" },
        { "runs", 'n', POPT_ARG_INT, &(param->runs), 'n', "Number of random greedy runs/iterations; default=1. Advice: when using random greedy, a higher value is recommended (100 was used in Stamina winner).\n\nSettings that modify the red-blue state-merging framework:", "integer" },
        { "extend", 'x', POPT_ARG_INT, &(param->extend), 'x', "When set to 1, any merge candidate (blue) that cannot be merged with any target (red) is immediately changed into a (red) target; default=1. If set to 0, a merge candidate is only changed into a target when no more merges are possible. Advice: unclear which strategy is best, when using statistical (or count-based) consistency checks, keep in mind that merge consistency between states may change due to other performed merges. This will especially influence low frequency states. When there are a lot of those, we therefore recommend setting x=0.", "integer" },
        { "shallowfirst", 'w', POPT_ARG_INT, &(param->shallowfirst), 'w', "When set to 1, the ordering of the nodes is changed from most frequent first (default) to most shallow (smallest depth) first; default=0. Advice: use depth-first when learning from characteristic samples.", "integer" },
        { "largestblue", 'a', POPT_ARG_INT, &(param->largestblue), 'a', "When set to 1, the algorithm only tries to merge the most frequent (or most shallow if w=1) candidate (blue) states with any target (red) state, instead of all candidates; default=0. Advice: this reduces run-time significantly but comes with a potential decrease in merge quality.", "integer" },
        { "blueblue", 'b', POPT_ARG_INT, &(param->blueblue), 'b', "When set to 1, the algorithm tries to merge candidate (blue) states with candiate (blue) states in addition to candidate (blue) target (red) merges; default=0. Advice: this adds run-time to the merging process in exchange for potential improvement in merge quality.", "integer" },
        { "finalred", 'f', POPT_ARG_INT, &(param->finalred), 'f', "When set to 1, merges that add new transitions to red states are considered inconsistent. Merges with red states will also not modify any of the counts used in evaluation functions. Once a red state has been learned, it is considered final and unmodifiable; default=0. Advice: setting this to 1 frequently results in easier to vizualize and more insightful models.\n\nSettings that influece the use of sinks:", "integer" },
        { "sinkson", 'I', POPT_ARG_INT, &(param->sinkson), 'I', "Set to 1 to use sink states; default=1. Advice: leads to much more concise and easier to vizualize models, but can cost predictive performance depending on the sink definitions.", "integer"},
        { "mergesinks", 'J', POPT_ARG_INT, &(param->mergesinks), 'J', "Sink nodes are candidates for merging during the greedy runs (setting 0 or 1); default=0. Advice: merging sinks typically only makes the learned model worse. Keep in mind that sinks can become non-sinks due to other merges that influcence the occurrence counts.", "integer" },
        { "satmergesinks", 'K', POPT_ARG_INT, &(param->satmergesinks), 'K', "Merge all sink nodes of the same type before sending the problem to the SAT solver (setting 0 or 1); default=1. Advice: radically improves runtime, only set to 0 when sinks of the same type can be different states in the final model.\n\nSettings that influence merge evaluations:", "integer" },
        { "testmerge", 't', POPT_ARG_INT, &(param->testmerge), 't', "When set to 1, merge tries in order to compute the evaluation scores do not actually perform the merges themselves. Thus the consistency and score evaluation for states in merges that add recursive loops are uninfluenced by earlier merges; default=0. Advice: setting this to 1 reduces run-time and can be useful when learning models using statistical evaluation functions, but can lead to inconsistencies when learning from labeled data.", "integer" },
        { "lowerbound", 'l', POPT_ARG_FLOAT, &(param->lower_bound), 'l', "Minimum value of the heuristic function, smaller values are treated as inconsistent, also used as the paramater value in any statistical tests; default=-1. Advice: state merging is forced to perform the merge with best heuristic value, it can sometimes be better to color a state red rather then performing a bad merge. This is achieved using a positive lower bound value. Models learned with positive lower bound are frequently more interpretable.", "float" },
        { "state_count", 'q', POPT_ARG_INT, &(param->state_count), 'q', "The minimum number of positive occurrences of a state for it to be included in overlap/statistical checks (see evaluation functions); default=25. Advice: low frequency states can have an undesired influence on statistical tests, set to at least 10. Note that different evaluation functions can use this parameter in different ways.", "integer" },
        { "symbol_count", 'y', POPT_ARG_INT, &(param->symbol_count), 'y', "The minimum number of positive occurrences of a symbol/transition for it to be included in overlap/statistical checks, symbols with less occurrences are binned together; default=10. Advice: low frequency transitions can have an undesired influence on statistical tests, set to at least 4. Note that different evaluation functions can use this parameter in different ways.", "integer" },
        { "correction", 'c', POPT_ARG_FLOAT, &(param->correction), 'c', "Value of a Laplace correction (smoothing) added to all symbol counts when computing statistical tests (in ALERGIA, LIKELIHOODRATIO, AIC, and KULLBACK-LEIBLER); default=0.0. Advice: unclear whether smoothing is needed for the different tests, more smoothing typically leads to smaller models.", "float" },
        { "extrapar", 'p', POPT_ARG_FLOAT, &(param->extrapar), 'p', "Extra parameter used during statistical tests, the significance level for the likelihood ratio test, the alpha value for ALERGIA; default=0.5. Advice: look up the statistical test performed, this parameter is not always the same as a p-value.\n\nSettings influencing the SAT solving procedures:", "float" },
        { "sataptabound", 'A', POPT_ARG_INT, &(param->sataptabound), 'A', "Maximum number of remaining states in the partially learned DFA before starting the SAT search process. The higher this value, the larger the problem sent to the SAT solver; default=2000. Advice: try sending problem instances that are as large as possible, since larger instances take more time, test what time is acceptable for you.", "integer" },
        { "satdfabound", 'D', POPT_ARG_INT, &(param->satdfabound), 'D', "Maximum size of the partially learned DFA before starting the SAT search process; default=50. Advice: when the merging process performs bad merges, it can blow-up the size of the learned model. This test ensres that models that are too large do not get solved.", "integer" },
        { "satextra", 'E', POPT_ARG_INT, &(param->satextra), 'E', "DFASAT runs a SAT solver to find a solution of size at most the size of the partially learned DFA + E; default=5. Advice: larger values greatly increases run-time. Setting it to 0 is frequently sufficient (when the merge heuristic works well).", "integer" },
        { "satplus", 'P', POPT_ARG_INT, &(param->satplus), 'P', "With every iteration, DFASAT tries to find solutions of size at most the best solution found + P, default=0. Advice: current setting only searches for better solutions. If a few extra states is OK, set it higher.", "int" },
        { "satfinalred", 'F', POPT_ARG_INT, &(param->satfinalred), 'F', "Make all transitions from red states without any occurrences force to have 0 occurrences (similar to targeting a rejecting sink), (setting 0 or 1) before sending the problem to the SAT solver; default=0. Advice: the same as finalred but for the SAT solver. Setting it to 1 greatly improves solving speed.", "integer" },
        { "satsymmetry", 'Y', POPT_ARG_INT, &(param->symmetry), 'Y', "Add symmetry breaking predicates to the SAT encoding (setting 0 or 1), based on Ulyantsev et al. BFS symmetry breaking; default=1. Advice: in our experience this only improves solving speed.", "integer" },
        { "satonlyinputs", 'O', POPT_ARG_INT, &(param->forcing), 'O', "Add predicates to the SAT encoding that force transitions in the learned DFA to be used by input examples (setting 0 or 1); default=0. Advice: leads to non-complete models. When the data is sparse, this should be set to 1. It does make the instance larger and can have a negative effect on the solving time.", "integer" },
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
    else
        param->sat_program = "";

    param->hName = hName;
    param->hData = hData;
   
    run(param); 
    
    delete param;
    
    return 0;    
}
