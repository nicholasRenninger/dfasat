#include "dfasat.h"
#include "random_greedy.h"
#include "evaluate.h"
//#include <malloc.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <set>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

bool MERGE_SINKS_PRESOLVE = 0;
int OFFSET = 1;
int EXTRA_STATES = 0;
bool TARGET_REJECTING = 0;

int literal_counter = 1;
int clause_counter = 0;

bool computing_header = true;

int **x;
int ***y;
int *z;

state_merger merger;
state_set red_states;
state_set non_red_states;
state_set sink_states;

FILE* sat_stream;

int dfa_size;
int num_states;

set<int> trueliterals;

int best_solution = -1;

/* Print clauses without eliminated literals, -2 = false, -1 = true */
int print_clause(bool v1, int l1, bool v2, int l2, bool v3, int l3, bool v4, int l4){
    if(v1 && l1 == -1) return 0;
    if(!v1 && l1 == -2) return 0;
    if(v2  && l2 == -1) return 0;
    if(!v2 && l2 == -2) return 0;
    if(v3 && l3 == -1) return 0;
    if(!v3 && l3 == -2) return 0;
    if(v4 && l4 == -1) return 0;
    if(!v4 && l4 == -2) return 0;
    
    if(computing_header) return 1;
    
    if(v1 == true  && l1 != -2) fprintf(sat_stream, "%i ", l1);
    if(v1 == false && l1 != -1) fprintf(sat_stream, "-%i ", l1);
    if(v2 == true  && l2 != -2) fprintf(sat_stream, "%i ", l2);
    if(v2 == false && l2 != -1) fprintf(sat_stream, "-%i ", l2);
    if(v3 == true  && l3 != -2) fprintf(sat_stream, "%i ", l3);
    if(v3 == false && l3 != -1) fprintf(sat_stream, "-%i ", l3);
    if(v4 == true  && l4 != -2) fprintf(sat_stream, "%i ", l4);
    if(v4 == false && l4 != -1) fprintf(sat_stream, "-%i ", l4);
    
    fprintf(sat_stream, "0\n");
    return 1;
}

int print_clause(bool v1, int l1, bool v2, int l2, bool v3, int l3){
    if(v1 && l1 == -1) return 0;
    if(!v1 && l1 == -2) return 0;
    if(v2  && l2 == -1) return 0;
    if(!v2 && l2 == -2) return 0;
    if(v3 && l3 == -1) return 0;
    if(!v3 && l3 == -2) return 0;
    
    if(computing_header) return 1;
    
    if(v1 == true  && l1 != -2) fprintf(sat_stream, "%i ", l1);
    if(v1 == false && l1 != -1) fprintf(sat_stream, "-%i ", l1);
    if(v2 == true  && l2 != -2) fprintf(sat_stream, "%i ", l2);
    if(v2 == false && l2 != -1) fprintf(sat_stream, "-%i ", l2);
    if(v3 == true  && l3 != -2) fprintf(sat_stream, "%i ", l3);
    if(v3 == false && l3 != -1) fprintf(sat_stream, "-%i ", l3);
    
    fprintf(sat_stream, " 0\n");
    return 1;
}

int print_clause(bool v1, int l1, bool v2, int l2){
    if(v1 && l1 == -1) return 0;
    if(!v1 && l1 == -2) return 0;
    if(v2  && l2 == -1) return 0;
    if(!v2 && l2 == -2) return 0;
    
    if(computing_header) return 1;
    
    if(v1 == true  && l1 != -2) fprintf(sat_stream, "%i ", l1);
    if(v1 == false && l1 != -1) fprintf(sat_stream, "-%i ", l1);
    if(v2 == true  && l2 != -2) fprintf(sat_stream, "%i ", l2);
    if(v2 == false && l2 != -1) fprintf(sat_stream, "-%i ", l2);
    
    fprintf(sat_stream, " 0\n");
    return 1;
}

/* fix values for red states -2 = false, -1 = true */
void fix_red_values(){
    for(state_set::iterator it = red_states.begin();it != red_states.end();++it){
        apta_node* node = *it;
        
        for(int i = 0; i < dfa_size; ++i) x[node->satnumber][i] = -2;
        x[node->satnumber][node->colour] = -1;
        
        apta_node* source = *it;
        for(int label = 0; label < alphabet_size; ++label){
            apta_node* target = source->get_child(label);
            if(target != 0 && red_states.find(target) != red_states.end()){
                for(int i = 0; i < dfa_size; ++i) y[label][source->colour][i] = -2;
                y[label][source->colour][target->colour] = -1;
            } else if(MERGE_SINKS_PRESOLVE && target != 0 && sink_states.find(target) != sink_states.end()){
                for(int i = 0; i < dfa_size; ++i) y[label][source->colour][i] = -2;
                y[label][source->colour][target->colour] = -1;
            } else if(TARGET_REJECTING && target == 0){
                for(int i = 0; i < dfa_size; ++i) y[label][source->colour][i] = -2;
                y[label][source->colour][0] = -1;
            }
        }
        
        if(node->num_accepting != 0) z[node->colour] = -1;
        if(node->num_rejecting != 0) z[node->colour] = -2;
    }
    
    if(MERGE_SINKS_PRESOLVE){
        for(state_set::iterator it = sink_states.begin();it != sink_states.end();++it){
            apta_node* node = *it;
            
            for(int i = 0; i < dfa_size; ++i) x[node->satnumber][i] = -2;
            if(is_rejecting_sink(node)){
                x[node->satnumber][0] = -1;
            }
            if(is_accepting_sink(node)){
                x[node->satnumber][1] = -1;
            }
        }
    }
    
    for(int i = 0; i < dfa_size; ++i) x[0][i] = -2;
    for(int i = 0; i < dfa_size; ++i) x[1][i] = -2;
    x[0][0] = -1;
    x[1][1] = -1;
    
    for(int label = 0; label < alphabet_size; ++label){
        for(int i = 0; i < dfa_size; ++i) y[label][0][i] = -2;
        for(int i = 0; i < dfa_size; ++i) y[label][1][i] = -2;
        y[label][0][0] = -1;
        y[label][1][1] = -1;
    }
    
    z[1] = -1;
    z[0] = -2;
}

/* erase possible colors due to symmetry reduction */
int set_symmetry(){
    int num = 0;
    int max_value = merger.red_states.size() + 2;
    for(state_set::iterator it = non_red_states.begin();
        it != non_red_states.end();
        ++it){
        apta_node* node = *it;
        if(max_value + 1>= dfa_size)
            break;
        
        for(int i = max_value + 1; i < dfa_size; ++i){
            x[node->satnumber][i] = -2;
        }
        max_value++;
    }
    return num;
}

/* eliminate literals for merges that conflict with the red states */
void erase_red_conflict_colours(){
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        apta_node* left = *it;
        for(state_set::iterator it2 = non_red_states.begin(); it2 != non_red_states.end(); ++it2){
            apta_node* right = *it2;
            if(merger.testmerge(left,right) == -1) x[right->satnumber][left->colour] = -2;
            if(right->accepting_paths != 0 || right->num_accepting != 0) x[right->satnumber][0] = -2;
            if(right->rejecting_paths != 0 || right->num_rejecting != 0) x[right->satnumber][1] = -2;
        }
    }
}

/* print the at least one en at most one clauses for x */
int print_colours(){
    int num = 0;
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* node = *it;
        bool always_true = false;
        for(int k = 0; k < dfa_size; ++k)
            if(x[node->satnumber][k] == -1) always_true = true;
        if(!always_true){
            if(!computing_header){
                for(int k = 0; k < dfa_size; ++k)
                    if(x[node->satnumber][k] >= 0) fprintf(sat_stream, "%i ", x[node->satnumber][k]);
                fprintf(sat_stream, "0\n");
            }
            num++;
        }
        for(int a = 0; a < dfa_size; ++a)
            for(int b = a+1; b < dfa_size; ++b)
                num += print_clause(false, x[node->satnumber][a], false, x[node->satnumber][b]);
    }
    return num;
}

/* print clauses restricting two unmergable states to have the same color *
 * excludes pairs of states that are covered by the z literals            */
int print_conflicts(){
    int num = 0;
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* left = *it;
        state_set::iterator it2 = it;
        ++it2;
        while(it2 != non_red_states.end()){
            apta_node* right = *it2;
            ++it2;
            if(left->num_accepting != 0 && right->num_rejecting != 0) continue;
            if(left->num_rejecting != 0 && right->num_accepting != 0) continue;
            
            if(merger.testmerge(left, right) == -1)
                for(int k = 0; k < dfa_size; ++k)
                    num += print_clause( false, x[left->satnumber][k], false, x[right->satnumber][k]);
        }
    }
    return num;
}

/* print de clauses voor z literals */
int print_accept(){
    int num = 0;
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* node = *it;
        for(int k = 0; k < dfa_size; ++k){
            if(node->num_accepting != 0) num += print_clause(false, x[node->satnumber][k], true, z[k]);
            if(node->num_rejecting != 0) num += print_clause(false, x[node->satnumber][k], false, z[k]);
        }
    }
    return num;
}

/* print de clauses voor y literals */
int print_transitions(){
    int num = 0;
    for(int a = 0; a < alphabet_size; ++a)
        for(int i = 0; i < dfa_size; ++i)
            for(int j = 0; j < dfa_size; ++j)
                for(int h = 0; h < j; ++h)
                    num += print_clause(false, y[a][i][h], false, y[a][i][j]);
    return num;
}

/* print de clauses voor y literals */
int print_forcing_transitions(){
    int num = 0;
    for (int label = 0; label < alphabet_size; ++label) {
        state_set label_states;
        for (state_set::iterator it = red_states.begin(); it != red_states.end(); ++it) {
            apta_node* source = *it;
            if(source->get_child(label) != 0) label_states.insert(source);
        }
        for (state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it) {
            apta_node* source = *it;
            if(source->get_child(label) != 0) label_states.insert(source);
        }
        
        for(int i = 0; i < dfa_size; ++i){
            bool always_true = false;
            for (state_set::iterator it = label_states.begin(); it != label_states.end(); ++it) {
                apta_node* source = *it;
                if(x[source->satnumber][i] == -1) always_true = true;
            }
            if(!always_true){
                for(int j = 0; j < dfa_size; ++j){
                    if(y[label][i][j] >= 0){
                        if(!computing_header){
                            for (state_set::iterator it = label_states.begin(); it != label_states.end(); ++it) {
                                apta_node* source = *it;
                                if(x[source->satnumber][i] >= 0) fprintf(sat_stream, "%i ", x[source->satnumber][i]);
                            }
                            fprintf(sat_stream, "-%i 0\n", y[label][i][j]);
                        }
                        num++;
                    }
                }
            }
        }
    }

    return num;
}

/* print de determinization constraint */
int print_paths(){
    int num = 0;
    for (state_set::iterator it = red_states.begin(); it != red_states.end(); ++it) {
        apta_node* source = *it;
        for (int label = 0; label < alphabet_size; ++label) {
            apta_node* node = source->get_child(label);
            if (node != 0) {
                for (int i = 0; i < dfa_size; ++i)
                    for (int j = 0; j < dfa_size; ++j)
                        num += print_clause(true, y[label][i][j], false, x[source->satnumber][i], false, x[node->satnumber][j]);
            }
        }
    }
    for (state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it) {
        apta_node* source = *it;
        for (int label = 0; label < alphabet_size; ++label) {
            apta_node* node = source->get_child(label);
            if (node != 0) {
                for (int i = 0; i < dfa_size; ++i)
                    for (int j = 0; j < dfa_size; ++j)
                        num += print_clause(true, y[label][i][j], false, x[source->satnumber][i], false, x[node->satnumber][j]);
            }
        }
    }
    return num;
}

/* output result to dot */
void print_dot_output(const char* dot_output){
    FILE* output = fopen(dot_output, "w");
    apta* aut = merger.aut;
    int v,i,a,j;
    
    fprintf(output,"digraph DFA {\n");
    fprintf(output,"\t\tI -> %i;\n", aut->root->find()->satnumber);
    
    set<int>::iterator it = trueliterals.begin();
    for(v = 0; v < num_states; ++v)
        for(i = 0; i < dfa_size; ++i)
            if(x[v][i] == *it) ++it;
    
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < dfa_size; ++i){
            for(j = 0; j < dfa_size; ++j) {
                if(y[a][i][j] == *it){
                    if(j != 0)
                        fprintf(output,"\t\t%i -> %i [label=\"%i\"];\n", i, j, a);
                    ++it;
                }
                if(y[a][i][j] == -1){
                    if(j != 0)
                        fprintf(output,"\t\t%i -> %i [label=\"%i\"];\n", i, j, a);
                }
            }
        }
    }
    
    fprintf(output,"\t%i [label=\"start\" shape=box];\n", aut->root->find()->satnumber);
    //fprintf(output,"\t0 [label=\"fail\" shape=box];\n");
    fprintf(output,"\t1 [label=\"pass\" shape=box];\n");
    for(i = 2; i < dfa_size; ++i){
        if(z[i] == *it){
            ++it;
            fprintf(output,"\t%i [shape=doublecircle];\n", i);
        } else if(z[i] == -1){
            fprintf(output,"\t%i [shape=doublecircle];\n", i);
        } else {
            fprintf(output,"\t%i [shape=Mcircle];\n", i);
        }
    }
    fprintf(output,"}\n");
    fclose(output);
}

/* output result to aut, for later processing in i.e. ensembles */
void print_aut_output(const char* aut_output){
    FILE* output = fopen(aut_output, "w");
    apta* aut = merger.aut;
    int v,i,a,j;
    
    fprintf(output,"%i %i\n", dfa_size, alphabet_size);
    fprintf(output,"%i\n", aut->root->find()->satnumber);
    
    set<int>::iterator it = trueliterals.begin();
    for(v = 0; v < num_states; ++v)
        for(i = 0; i < dfa_size; ++i)
            if(x[v][i] == *it) ++it;
    
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < dfa_size; ++i){
            for(j = 0; j < dfa_size; ++j) {
                if(y[a][i][j] == *it){
                    fprintf(output,"t %i %i %i\n", i, a, j);
                    ++it;
                }
                if(y[a][i][j] == -1){
                    fprintf(output,"t %i %i %i\n", i, a, j);
                }
            }
        }
    }
    
    fprintf(output,"a 0 0\n");
    fprintf(output,"a 1 1\n");
    for(i = 2; i < dfa_size; ++i){
        if(z[i] == *it){
            ++it;
            fprintf(output,"a %i 1\n", i);
        } else if(z[i] == -1){
            fprintf(output,"a %i 1\n", i);
        } else {
            fprintf(output,"a %i 0\n", i);
        }
    }
    fclose(output);
}

/* the main routine:
 * run greedy state merging runs
 * convert result to satisfiability formula
 * run sat solver
 * translate result to a DFA
 * print result
 * */
int dfasat(state_merger &m, const char* sat_program, const char* dot_output, const char* aut_output){
    int i,j,l,v,a,h,k;
    merger = m;
    apta* the_apta = merger.aut;
    
    merge_list merges = random_greedy_bounded_run(&merger);
    
    std::ostringstream oss;
    oss << "pre_" << dot_output;
    FILE* output = fopen(oss.str().c_str(), "w");
    merger.todot(output);
    fclose(output);
    
    non_red_states = merger.get_candidate_states();
    red_states     = merger.red_states;
    sink_states    = merger.get_sink_states();
    
    if(best_solution != -1 && merger.red_states.size() + 2 >= best_solution + EXTRA_STATES){
        cerr << "Greedy preprocessing resulted in too many red states." << endl;
        while(!merges.empty()){
            merge_pair performed_merge = merges.front();
            merges.pop_front();
            merger.undo_merge(performed_merge.first, performed_merge.second);
        }
        return -1;
    }
    
    dfa_size = merger.red_states.size() + 2 + OFFSET;
    if(best_solution != -1) dfa_size = min(dfa_size, best_solution);
    
    // assign a unique number to every state
    if(MERGE_SINKS_PRESOLVE){
        for(state_set::iterator it = sink_states.begin(); it != sink_states.end(); ++it){
            apta_node* node = *it;
            if(is_rejecting_sink(node)){
                node->colour = 0;
            }
            if(is_accepting_sink(node)){
                node->colour = 1;
            }
        }
    }
    
    i = 2;
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        apta_node* node = *it;
        node->satnumber = i;
        node->colour = i;
        i++;
    }
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* node = *it;
        node->satnumber = i;
        i++;
    }
    for(state_set::iterator it = sink_states.begin(); it != sink_states.end(); ++it){
        apta_node* node = *it;
        node->satnumber = i;
        i++;
    }
    num_states = red_states.size() + non_red_states.size() + sink_states.size() + 2;
    
    clause_counter = 0;
    literal_counter = 1;
    
    cerr << "creating literals..." << endl;
    //X(STATE,COLOR)
    x = (int**) malloc( sizeof(int*) * num_states);
    for(v = 0; v < num_states; v++ )
        x[ v ] = (int*) malloc( sizeof(int) * dfa_size );
    for(v = 0; v < num_states; ++v)
        for(i = 0; i < dfa_size; ++i)
            x[v][i] = literal_counter++;
    
    //Y(LABEL,COLOR,COLOR)
    y = (int***) malloc( sizeof(int**) * alphabet_size);
    for(a = 0; a < alphabet_size; ++a){
        y[ a ] = (int**) malloc( sizeof(int*) * dfa_size );
        for(i = 0; i < dfa_size; ++i)
            y[ a ][ i ]  = (int*) malloc( sizeof(int) * dfa_size );
    }
    for(a = 0; a < alphabet_size; ++a)
        for(i = 0; i < dfa_size; ++i)
            for(j = 0; j < dfa_size; ++j)
                y[a][i][j] = literal_counter++;
    
    //Z(STATE)
    z = (int*) malloc( sizeof(int) * dfa_size);
    for(i = 0; i < dfa_size; ++i)
        z[i] = literal_counter++;
    
    cerr << "number of states: " << the_apta->get_states().size() << endl;
    cerr << "number of red states: " << red_states.size() << endl;
    cerr << "number of non_red states: " << non_red_states.size() << endl;
    cerr << "number of sink states: " << sink_states.size() << endl;
    
    fix_red_values();
    erase_red_conflict_colours();
    set_symmetry();
    
    // renumber literals to account for eliminated ones
    clause_counter = 0;
    literal_counter = 1;
    
    for(v = 0; v < num_states; ++v)
        for(i = 0; i < dfa_size; ++i)
            if(x[v][i] >= 0) x[v][i] = literal_counter++;
    
    for(a = 0; a < alphabet_size; ++a)
        for(i = 0; i < dfa_size; ++i)
            for(j = 0; j < dfa_size; ++j)
                if(y[a][i][j] >= 0)
                    y[a][i][j] = literal_counter++;
    
    for(i = 0; i < dfa_size; ++i)
        if(z[i] >= 0)
            z[i] = literal_counter++;
    
    computing_header = true;
    
    clause_counter += print_colours();
    clause_counter += print_conflicts();
    clause_counter += print_accept();
    clause_counter += print_transitions();
    clause_counter += print_forcing_transitions();
    clause_counter += print_paths();
    
    cerr << "header: p cnf " << literal_counter - 1 << " " << clause_counter << endl;
    computing_header = false;
    
    int pipetosat[2];
    int pipefromsat[2];
    if (pipe(pipetosat) < 0 || pipe(pipefromsat) < 0){
        cerr << "Unable to create pipe for SAT solver: " << strerror(errno) << endl;
        exit(1);
    }
    pid_t pid = fork();
    if (pid == 0){
        close(pipetosat[1]);
        dup2(pipetosat[0], STDIN_FILENO);
        close(pipetosat[0]);
        
        close(pipefromsat[0]);
        dup2(pipefromsat[1], STDOUT_FILENO);
        close(pipefromsat[1]);
        
        cerr << "starting SAT solver " << sat_program << endl;
        char* copy_sat = strdup(sat_program);
        char* pch = strtok (copy_sat," ");
        vector<char*> args;
        while (pch != NULL){
            args.push_back(strdup(pch));
            pch = strtok (NULL," ");
        }
        free(copy_sat);
        free(pch);
        args.push_back((char*)NULL);
        execvp(args[0], &args[0]);
        cerr << "finished SAT solver" << endl;
        for(int argi = 0; argi < args.size(); ++argi) free(args[argi]);
        int* status;
        WIFEXITED(status);
        wait(status);
    }
    else
    {
        close(pipetosat[0]);
        close(pipefromsat[1]);
        
        sat_stream = (FILE*) fdopen(pipetosat[1], "w");
        if (sat_stream == 0){
            cerr << "Cannot open pipe to SAT solver: " << strerror(errno) << endl;
            exit(1);
        }
        fprintf(sat_stream, "p cnf %i %i\n", literal_counter - 1, clause_counter);
        
        print_colours();
        print_conflicts();
        print_accept();
        print_transitions();
        print_forcing_transitions();
        print_paths();
        
        fclose(sat_stream);
        
        cerr << "sent problem to SAT solver" << endl;
        
        trueliterals = set<int>();
        
        char line[500];
        sat_stream = fdopen ( pipefromsat[0], "r" );
        
        bool improved = false;
        while(fgets ( line, sizeof line, sat_stream ) != NULL){
            char* pch = strtok (line," ");
            if(strcmp(pch,"s") == 0){
                pch = strtok (NULL, " ");
                cerr << pch << endl;
                if(strcmp(pch,"SATISFIABLE\n")==0){
                    cerr << "new solution, size = " << dfa_size << endl;
                    if(best_solution ==-1 || best_solution > dfa_size){
                        cerr << "new best solution, size = " << dfa_size << endl;
                        best_solution = dfa_size;
                        improved = true;
                    }
                }
            }
            if(strcmp(pch,"v") == 0){
                pch = strtok (NULL, " ");
                while(pch != NULL){
                    int val = atoi(pch);
                    if(val > 0) trueliterals.insert(val);
                    pch = strtok (NULL, " ");
                }
            }
        }
        fclose(sat_stream);
        
        if(improved){
            print_dot_output(dot_output);
            print_aut_output(aut_output);
        }
        
        while(!merges.empty()){
            merge_pair performed_merge = merges.front();
            merges.pop_front();
            merger.undo_merge(performed_merge.first, performed_merge.second);
        }
    }
    
    for(v = 0; v < num_states; v++ )
        free(x[ v ]);
    free(x);
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < dfa_size; ++i)
            free(y[ a ][ i ]);
        free(y[ a ]);
    }
    free(y);
    free(z);
    
    return best_solution;
};
