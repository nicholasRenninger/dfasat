/// @file dfasat.cpp
/// @brief Reduction for the SAT solver, loop for the combined heuristic-SAT mode
/// @author Sicco Verwer


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
#include <ctime>

#include "parameters.h"

// apta/input state i has color j
int **x;
// color i has a transition with label a to color j
int ***y;
// color i is an accepting state
int *z;

// color i has a transition with label a to sink j
int ***sy;
// state i is a sink or one of the parents of apta/input state i is a sink
int *sp;

// literals used for symmetry breaking
int **yt;
int **yp;
int **ya;


void merger_context::reset_literals(bool init){
    int v, i, j, a;

    literal_counter = 1;
    for(v = 0; v < num_states; ++v)
        for(i = 0; i < dfa_size; ++i)
            if(init || x[v][i] > 0) x[v][i] = literal_counter++;
    
    for(a = 0; a < alphabet_size; ++a)
        for(i = 0; i < dfa_size; ++i)
            for(j = 0; j < dfa_size; ++j)
                if(init || y[a][i][j] > 0) y[a][i][j] = literal_counter++;

    for(a = 0; a < alphabet_size; ++a)
        for(i = 0; i < dfa_size; ++i)
            for(j = 0; j < sinks_size; ++j)
                if(init || sy[a][i][j] > 0) sy[a][i][j] = literal_counter++;

    for(i = 0; i < num_states; ++i)
        if(init || sp[i] > 0) sp[i] = literal_counter++;
    
    for(i = 0; i < dfa_size; ++i)
        if(init || z[i] > 0) z[i] = literal_counter++;

    for(i = 0; i < dfa_size; ++i)
        for(j = 0; j < new_states; ++j)
            if(init || yt[i][j] > 0) yt[i][j] = literal_counter++;
    
    for(i = 0; i < dfa_size; ++i)
        for(j = 0; j < new_states; ++j)
            if(init || yp[i][j] > 0) yp[i][j] = literal_counter++;
    
    for(a = 0; a < alphabet_size; ++a)
        for(i = 0; i < new_states; ++i)
            if(init || ya[a][i] > 0) ya[a][i] = literal_counter++;
}

void merger_context::create_literals(){
    int v, a, i;
    //X(STATE,COLOR)
    x = (int**) malloc( sizeof(int*) * num_states);
    for(v = 0; v < num_states; v++ )
        x[ v ] = (int*) malloc( sizeof(int) * dfa_size);
    
    //Y(LABEL,COLOR,COLOR)
    y = (int***) malloc( sizeof(int**) * alphabet_size);
    for(a = 0; a < alphabet_size; ++a){
        y[ a ] = (int**) malloc( sizeof(int*) * dfa_size);
        for(i = 0; i < dfa_size; ++i)
            y[ a ][ i ]  = (int*) malloc( sizeof(int) * dfa_size);
    }

    //SY(LABEL,COLOR,SINK)
    sy = (int***) malloc( sizeof(int**) * alphabet_size);
    for(a = 0; a < alphabet_size; ++a){
        sy[ a ] = (int**) malloc( sizeof(int*) * dfa_size);
        for(i = 0; i < dfa_size; ++i)
            sy[ a ][ i ]  = (int*) malloc( sizeof(int) * sinks_size);
    }

    //SP(STATE)
    sp = (int*) malloc( sizeof(int) * num_states);
    
    //Z(COLOR)
    z = (int*) malloc( sizeof(int) * dfa_size);
    
    //YT(COLOR,COLOR)
    yt = (int**) malloc( sizeof(int*) * dfa_size);
    for(i = 0; i < dfa_size; ++i)
        yt[ i ]  = (int*) malloc( sizeof(int) * new_states);
    
    //YP(COLOR,COLOR)
    yp = (int**) malloc( sizeof(int*) * dfa_size);
    for(i = 0; i < dfa_size; ++i)
        yp[ i ]  = (int*) malloc( sizeof(int) * new_states);
    
    //YA(LABEL,COLOR)
    ya = (int**) malloc( sizeof(int*) * alphabet_size);
    for(a = 0; a < alphabet_size; ++a)
        ya[ a ]  = (int*) malloc( sizeof(int) * new_states);
    
    // reset literal values
    reset_literals(true);
}

void merger_context::delete_literals(){
    int v, a, i;
    for(v = 0; v < num_states; v++ )
        free(x[ v ]);
    free(x);
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < dfa_size; ++i)
            free(y[ a ][ i ]);
        free(y[ a ]);
    }
    free(y);
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < sinks_size; ++i)
            free(sy[ a ][ i ]);
        free(sy[ a ]);
    }
    free(sy);
    free(z);
    free(sp);
    for(i = 0; i < dfa_size; ++i)
        free(yt[ i ]);
    free(yt);
    for(i = 0; i < dfa_size; ++i)
        free(yp[ i ]);
    free(yp);
    for(a = 0; a < alphabet_size; ++a)
        free(ya[ a ]);
    free(ya);
}

/** Print clauses without eliminated literals, -2 = false, -1 = true */
int merger_context::print_clause(bool v1, int l1, bool v2, int l2, bool v3, int l3, bool v4, int l4){
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
    
    fprintf(sat_stream, " 0\n");
    return 1;
}

int merger_context::print_clause(bool v1, int l1, bool v2, int l2, bool v3, int l3){
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

int merger_context::print_clause(bool v1, int l1, bool v2, int l2){
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

bool merger_context::always_true(int number, bool flag){
    if(number == -1 && flag == true)  return true;
    if(number == -2 && flag == false) return true;
    return false;
}

void merger_context::print_lit(int number, bool flag){
    if(computing_header) return;
    if(number < 0) return;
    
    if(flag == true) fprintf(sat_stream, "%i ", number);
    else fprintf(sat_stream, "-%i ", number);
}

void merger_context::print_clause_end(){
    if(computing_header) return;
    fprintf(sat_stream, " 0\n");
}

/** fix values for red states -2 = false, -1 = true */
void merger_context::fix_red_values(){
    for(state_set::iterator it = red_states.begin();it != red_states.end();++it){
        apta_node* node = *it;
        
        for(int i = 0; i < dfa_size; ++i) x[node->satnumber][i] = -2;
        sp[node->satnumber] = -2;
        x[node->satnumber][node->colour] = -1;
        
        apta_node* source = *it;
        for(int label = 0; label < alphabet_size; ++label){
            apta_node* target = source->get_child(label);
            if(target != 0 && red_states.find(target) != red_states.end()){
                for(int i = 0; i < dfa_size; ++i) y[label][source->colour][i] = -2;
                for(int i = 0; i < sinks_size; ++i) sy[label][source->colour][i] = -2;
                y[label][source->colour][target->colour] = -1;
           }
        }
        
        //if(node->num_accepting != 0) z[node->colour] = -1;
        //if(node->num_rejecting != 0) z[node->colour] = -2;
        
        if(node->type == 1) z[node->colour] = -1;
        if(node->type != 1) z[node->colour] = -2;
    }
}

void merger_context::fix_sink_values(){
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        apta_node* node = *it;

        apta_node* source = *it;
        for(int label = 0; label < alphabet_size; ++label){
            apta_node* target = source->get_child(label);
            if(MERGE_SINKS_PRESOLVE && target != 0 && sink_states.find(target) != sink_states.end()){
                for(int i = 0; i < dfa_size; ++i) y[label][source->colour][i] = -2;
                for(int i = 0; i < sinks_size; ++i) sy[label][source->colour][i] = -2;
                sy[label][source->colour][merger->sink_type(target)] = -1;
            } else if(TARGET_REJECTING && target == 0){
                for(int i = 0; i < dfa_size; ++i) y[label][source->colour][i] = -2;
                for(int i = 0; i < sinks_size; ++i) sy[label][source->colour][i] = -2;
                sy[label][source->colour][0] = -1;
            }
        }
    }
}

/** erase possible colors due to symmetry reduction
 * should be compatible with BFS symmtry breaking, unchecked
 */
int merger_context::set_symmetry(){
    int num = 0;
    int max_value = new_init;
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        if(max_value + 1>= dfa_size)
            break;
        
        apta_node* node = *it;
        for(int a = 0; a < alphabet_size; ++a){
            if(max_value + 1>= dfa_size)
                break;
            
            apta_node* child = (*it)->get_child(a);
            if(child != 0 && red_states.find(child) == red_states.end()){
                if(MERGE_SINKS_PRESOLVE && sink_states.find(child) != sink_states.end())
                    continue;
                
                for(int i = max_value + 1; i < dfa_size; ++i){
                    x[child->satnumber][i] = -2;
                }
                max_value++;
            }
        }
    }
    return num;
}

int merger_context::print_symmetry(){
    int num = 0;
    for(int i = 0; i < dfa_size; ++i){
        for(int k = 0; k < new_states; ++k){
            for(int j = 0; j < i; ++j){
                for(int l = k + 1; l < new_states; l++){
                    num += print_clause(false, yp[i][k], false, yp[j][l]);
                }
            }
        }
    }
    for(int i = 0; i < dfa_size; ++i){
        for(int k = 0; k < new_states; ++k){
            for(int l = k + 1; l < new_states; l++){
                for(int a = 0; a < alphabet_size; ++a){
                    for(int b = 0; b < a; ++b){
                        num += print_clause(false, yp[i][k], false, yp[i][l], false, ya[a][k], false, ya[b][l]);
                    }
                }
            }
        }
    }
    return num;
}

/** eliminate literals for merges that conflict with the red states */
void merger_context::erase_red_conflict_colours(){
    for(state_set::iterator it = red_states.begin(); it != red_states.end(); ++it){
        apta_node* left = *it;
        for(state_set::iterator it2 = non_red_states.begin(); it2 != non_red_states.end(); ++it2){
            apta_node* right = *it2;
            if(merger->test_merge(left,right).first == false) x[right->satnumber][left->colour] = -2;
            //if(merger.test_local_merge(left,right) == -1) x[right->satnumber][left->colour] = -2;
            //if(right->accepting_paths != 0 || right->num_accepting != 0) x[right->satnumber][0] = -2;
            //if(right->rejecting_paths != 0 || right->num_rejecting != 0) x[right->satnumber][1] = -2;
        }
    }
}

void erase_sink_conflict_colours(){
    /*for(int i = 0; i < num_sink_types; ++ i){
        for(state_set::iterator it2 = non_red_states.begin(); it2 != non_red_states.end(); ++it2){
            apta_node* right = *it2;
            if(merger.sink_consistent(right,i) == -1) x[right->satnumber][left->colour] = -2;
        }
    }*/
}

/* print the at least one en at most one clauses for x */
int merger_context::print_colours(){
    int num = 0;
    bool altr = false;
    // at least one
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* node = *it;
        altr = always_true(sp[node->satnumber], true);
        for(int k = 0; k < dfa_size; ++k){
            if(altr) break;
            altr = always_true(x[node->satnumber][k], true);
        }
        if(altr == false){
            for(int k = 0; k < dfa_size; ++k)
                print_lit(x[node->satnumber][k], true);
            print_lit(sp[node->satnumber], true);
            print_clause_end();
            num += 1;
        }
    }
    // at most one
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* node = *it;
        for(int a = 0; a < dfa_size; ++a)
            for(int b = a+1; b < dfa_size; ++b)
                num += print_clause(false, x[node->satnumber][a], false, x[node->satnumber][b]);
        for(int a = 0; a < dfa_size; ++a)
            num += print_clause(false, x[node->satnumber][a], false, sp[node->satnumber]);
    }
    return num;
}

/* print clauses restricting two unmergable states to have the same color *
 * excludes pairs of states that are covered by the z literals            */
int merger_context::print_conflicts(){
    int num = 0;
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* left = *it;
        state_set::iterator it2 = it;
        ++it2;
        while(it2 != non_red_states.end()){
            apta_node* right = *it2;
            ++it2;
            //if(left->num_accepting != 0 && right->num_rejecting != 0) continue;
            //if(left->num_rejecting != 0 && right->num_accepting != 0) continue;
            //if(left->type == 1 && right->type != 1) continue;
            //if(left->type != 1 && right->type == 1) continue;
            
            if(merger->test_merge(left, right).first == false){
                for(int k = 0; k < dfa_size; ++k)
                    num += print_clause(false, x[left->satnumber][k], false, x[right->satnumber][k]);
            }
            /*if(merger.test_local_merge(left, right) == -1){
                for(int k = 0; k < dfa_size; ++k)
                    num += print_clause(false, x[left->satnumber][k], false, x[right->satnumber][k]);
            }*/
        }
    }
    return num;
}

/* print de clauses voor z literals */
int merger_context::print_accept(){
    int num = 0;
    for(state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it){
        apta_node* node = *it;
        for(int k = 0; k < dfa_size; ++k){
            //if(node->num_accepting != 0) num += print_clause(false, x[node->satnumber][k], true, z[k]);
            //if(node->num_rejecting != 0) num += print_clause(false, x[node->satnumber][k], false, z[k]);
            if(node->type == 1) num += print_clause(false, x[node->satnumber][k], true, z[k]);
            if(node->type != 1) num += print_clause(false, x[node->satnumber][k], false, z[k]);
        }
    }
    return num;
}

/* print de clauses voor y literals */
int merger_context::print_transitions(){
    int num = 0;
    for(int a = 0; a < alphabet_size; ++a)
        for(int i = 0; i < dfa_size; ++i)
            for(int j = 0; j < dfa_size; ++j)
                for(int h = 0; h < j; ++h)
                    num += print_clause(false, y[a][i][h], false, y[a][i][j]);
    return num;
}

int merger_context::print_sink_transitions(){
    int num = 0;
    for(int a = 0; a < alphabet_size; ++a)
        for(int i = 0; i < dfa_size; ++i)
            for(int j = 0; j < sinks_size; ++j)
                for(int h = 0; h < dfa_size; ++h)
                    num += print_clause(false, y[a][i][h], false, sy[a][i][j]);
    for(int a = 0; a < alphabet_size; ++a)
        for(int i = 0; i < dfa_size; ++i)
            for(int j = 0; j < sinks_size; ++j)
                for(int h = 0; h < j; ++h)
                    num += print_clause(false, sy[a][i][h], false, sy[a][i][j]);
    return num;
}

/* print transitions for any label yt */
int merger_context::print_t_transitions(){
    int num = 0;
    for(int i = 0; i < dfa_size; ++i)
        for(int j = 0; j < new_states; ++j)
            for(int a = 0; a < alphabet_size; ++a)
                num += print_clause(false, y[a][i][new_init+j], true, yt[i][j]);
    
    for(int i = 0; i < dfa_size; ++i){
        for(int j = 0; j < new_states; ++j){
            bool altr = false;
            for(int a = 0; a < alphabet_size; ++a)
                if(y[a][i][new_init+j] == -1) altr = true;
            if(!altr){
                if(!computing_header){
                    print_lit(yt[i][j], false);
                    for(int a = 0; a < alphabet_size; ++a){
                        print_lit(y[a][i][new_init+j], true);
                    }
                    print_clause_end();
                }
                num++;
            }
        }
    }
    
    return num;
}

/* print BFS tree transitions */
int merger_context::print_p_transitions(){
    int num = 0;
    for(int i = 0; i < dfa_size; ++i){
        for(int j = 0; j < new_states; ++j){
            for(int k = 0; k < i; ++k){
                num += print_clause(false, yp[i][j], false, yt[k][j]);
            }
            num += print_clause(false, yp[i][j], true, yt[i][j]);
        }
    }
    for(int i = 0; i < new_states; ++i){
        bool altr = false;
        for(int j = 0; j < new_init+i; ++j)
            if(yp[j][i] == -1) altr = true;
        if(!altr){
            if(!computing_header){
                for(int j = 0; j < new_init+i; ++j){
                    print_lit(yp[j][i], true);
                }
                print_clause_end();
            }
            num++;
        }
    }
    return num;
}

/* print BFS tree labels */
int merger_context::print_a_transitions(){
    int num = 0;
    for(int i = 0; i < new_states; ++i){
        for(int a = 0; a < alphabet_size; ++a){
            for(int j = 0; j < dfa_size; ++j){
                for(int b = 0; b < a; ++b){
                    num += print_clause(false, ya[a][i], false, yp[j][i], false, y[b][j][new_init+i]);
                }
                num += print_clause(false, ya[a][i], false, yp[j][i], true, y[a][j][new_init+i]);
            }
        }
    }
    for(int i = 0; i < new_states; ++i){
        bool altr = false;
        for(int a = 0; a < alphabet_size; ++a){
            if(ya[a][i] == -1) altr = true;
        }
        if(!altr){
            if(!computing_header){
                for(int a = 0; a < alphabet_size; ++a){
                    print_lit(ya[a][i], true);
                }
                print_clause_end();
            }
            num++;
        }
    }
    return num;
}

/* print de clauses voor y literals */
int merger_context::print_forcing_transitions(){
    int num = 0;
    bool altr = false;
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
            for(int j = 0; j < dfa_size; ++j){
                altr = always_true(y[label][i][j], false);
                for (state_set::iterator it = label_states.begin(); it != label_states.end(); ++it) {
                    if(altr) break;
                    apta_node* source = *it;
                    altr = always_true(x[source->satnumber][i], true);
                }
                if(altr == false){
                    for (state_set::iterator it = label_states.begin(); it != label_states.end(); ++it) {
                        apta_node* source = *it;
                        print_lit(x[source->satnumber][i], true);
                    }
                    print_lit(y[label][i][j], false);
                    print_clause_end();
                    num += 1;
                }
            }
        }
        
        for(int i = 0; i < dfa_size; ++i){
            for(int j = 0; j < sinks_size; ++j){
                altr = always_true(sy[label][i][j], false);
                for (state_set::iterator it = label_states.begin(); it != label_states.end(); ++it) {
                    if(altr) break;
                    apta_node* source = *it;
                    altr = always_true(x[source->satnumber][i], true);
                }
                if(altr == false){
                    for (state_set::iterator it = label_states.begin(); it != label_states.end(); ++it) {
                        apta_node* source = *it;
                        print_lit(x[source->satnumber][i], true);
                    }
                    print_lit(sy[label][i][j], false);
                    print_clause_end();
                    num += 1;
                }
            }
        }
    }
    return num;
}

/* print de determinization constraint */
int merger_context::print_paths(){
    int num = 0;
    for (state_set::iterator it = red_states.begin(); it != red_states.end(); ++it) {
        apta_node* source = *it;
        for (int label = 0; label < alphabet_size; ++label) {
            apta_node* target = source->get_child(label);
            if (target != 0 && sink_states.find(target) == sink_states.end()) {
                for (int i = 0; i < dfa_size; ++i)
                    for (int j = 0; j < dfa_size; ++j)
                        num += print_clause(true, y[label][i][j], false, x[source->satnumber][i], false, x[target->satnumber][j]);
            }
        }
    }
    for (state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it) {
        apta_node* source = *it;
        for (int label = 0; label < alphabet_size; ++label) {
            apta_node* target = source->get_child(label);
            if (target != 0) {
                for (int i = 0; i < dfa_size; ++i)
                    for (int j = 0; j < dfa_size; ++j)
                        num += print_clause(true, y[label][i][j], false, x[source->satnumber][i], false, x[target->satnumber][j]);
            }
        }
    }
    return num;
}

/* print sink paths */
int merger_context::print_sink_paths(){
    int num = 0;
    bool altr = false;
    for (state_set::iterator it = red_states.begin(); it != red_states.end(); ++it) {
        apta_node* source = *it;
        for (int label = 0; label < alphabet_size; ++label) {
            apta_node* target = source->get_child(label);
            if (target != 0 && sink_states.find(target) == sink_states.end()) {
                for (int i = 0; i < dfa_size; ++i)
                    for (int j = 0; j < sinks_size; ++j)
                        if(merger->sink_consistent(target, j) == false)
                            num += print_clause(false, sy[label][i][j], false, x[source->satnumber][i]);
                
                for (int i = 0; i < dfa_size; ++i){
                    altr = always_true(x[source->satnumber][i], false);
                    if(!altr) altr = always_true(sp[target->satnumber], false);
                    for(int j = 0; j < sinks_size; ++j){
                        if(altr) break;
                        if(merger->sink_consistent(target, j) == true) altr = always_true(sy[label][i][j], true);
                    }
                    
                    if(altr == false){
                        for(int j = 0; j < sinks_size; ++j)
                            if(merger->sink_consistent(target, j) == true) print_lit(sy[label][i][j], true);
                        print_lit(x[source->satnumber][i], false);
                        print_lit(sp[target->satnumber], false);
                        print_clause_end();
                        num += 1;
                    }
                }
                num += print_clause(false, sp[source->satnumber], true, sp[target->satnumber]);
            }
        }
    }
    for (state_set::iterator it = non_red_states.begin(); it != non_red_states.end(); ++it) {
        apta_node* source = *it;
        for (int label = 0; label < alphabet_size; ++label) {
            apta_node* target = source->get_child(label);
            if (target != 0) {
                for (int i = 0; i < dfa_size; ++i)
                    for (int j = 0; j < sinks_size; ++j)
                        if(merger->sink_consistent(target, j) == false)
                            num += print_clause(false, sy[label][i][j], false, x[source->satnumber][i]);
                for (int i = 0; i < dfa_size; ++i){
                    altr = always_true(x[source->satnumber][i], false);
                    if(!altr) altr = always_true(sp[target->satnumber], false);
                    for(int j = 0; j < sinks_size; ++j){
                        if(altr) break;
                        if(merger->sink_consistent(target, j) == true) altr = always_true(sy[label][i][j], true);
                    }
                    
                    if(altr == false){
                        for(int j = 0; j < sinks_size; ++j)
                            if(merger->sink_consistent(target, j) == true) print_lit(sy[label][i][j], true);
                        print_lit(x[source->satnumber][i], false);
                        print_lit(sp[target->satnumber], false);
                        print_clause_end();
                        num += 1;
                    }
                }
                num += print_clause(false, sp[source->satnumber], true, sp[target->satnumber]);
            }
        }
    }
    return num;
}

/* output result to dot */
void merger_context::print_dot_output(const char* dot_output){
    FILE* output = fopen(dot_output, "w");
    apta* aut = merger->aut;
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
    
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < dfa_size; ++i){
            for(j = 0; j < sinks_size; ++j){
                if(sy[a][i][j] == *it){
                    ++it;
                    fprintf(output,"\t\t %i -> s%i [label=\"%i\"];\n", i, j, a);
                }
            }
        }
    }

    for(j = 0; j < sinks_size; ++j){
        fprintf(output,"\ts%i [shape=box];\n", j);
    }
    
    for(i = 0; i < num_states; ++i){
        if(sp[i] == *it){
            //cerr << "sp " << i << endl;
            ++it;
        }
    }
    
    for(i = 0; i < dfa_size; ++i){
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
void merger_context::print_aut_output(const char* aut_output){
    FILE* output = fopen(aut_output, "w");
    apta* aut = merger->aut;
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
    
    for(a = 0; a < alphabet_size; ++a){
        for(i = 0; i < dfa_size; ++i){
            for(j = 0; j < sinks_size; ++j){
                if(sy[a][i][j] == *it){
                    ++it;
                    fprintf(output,"t %i %i %i;\n", i, a, dfa_size+j);
                }
            }
        }
    }
    
    for(i = 0; i < num_states; ++i){
        if(sp[i] == *it){
            //cerr << "sp " << i << endl;
            ++it;
        }
    }
    
    for(i = 0; i < dfa_size; ++i){
        if(z[i] == *it){
            ++it;
            fprintf(output,"a %i 1\n", i);
        } else if(z[i] == -1){
            fprintf(output,"a %i 1\n", i);
        } else {
            fprintf(output,"a %i 0\n", i);
        }
    }

    for(i = 0; i < sinks_size; ++i){
        fprintf(output,"a %i s%i\n", i+dfa_size, i);
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
int dfasat(state_merger &merger, string sat_program, const char* dot_output_file, const char* aut_output){
    int i,j,l,v,a,h,k;
//    merger = m;
    apta* the_apta = merger.aut;

    refinement_list* refs = random_greedy_bounded_run(&merger);
    
    merger.todot();

    if (dot_output_file != NULL) {
      std::ostringstream oss2;
      oss2 << "pre_" << dot_output_file;
      ofstream output(oss2.str().c_str());
      output << merger.dot_output;
      output.close();
    }

    
    merger.context.non_red_states = merger.get_candidate_states();
    merger.context.red_states     = merger.red_states;
    merger.context.sink_states    = merger.get_sink_states();
    
    if(merger.context.best_solution != -1 && merger.red_states.size() >= merger.context.best_solution + EXTRA_STATES){
        cerr << "Greedy preprocessing resulted in too many red states." << endl;
        while(!refs->empty()){
            refinement* ref = refs->front();
            refs->pop_front();
            ref->undo(&merger);
            delete ref;
        }
        delete refs;
        return -1;
    }
    
    merger.context.dfa_size = min(merger.red_states.size() + OFFSET, merger.context.red_states.size() + merger.context.non_red_states.size());
    merger.context.sinks_size = 0;
    if(USE_SINKS) merger.context.sinks_size = merger.aut->root->data->num_sink_types();
    
    if(!MERGE_SINKS_PRESOLVE) merger.context.non_red_states.insert(merger.context.sink_states.begin(), merger.context.sink_states.end());
    merger.context.num_states = merger.context.red_states.size() + merger.context.non_red_states.size();
    
    if(merger.context.best_solution != -1) merger.context.dfa_size = min(merger.context.dfa_size, merger.context.best_solution);
    merger.context.new_states = merger.context.dfa_size - merger.red_states.size();
    merger.context.new_init = merger.red_states.size();
    

    /* run reduction code IF valid solver was specified */
    struct stat buffer;
    bool sat_program_exists = (stat(sat_program.c_str(), &buffer) == 0);
    if (sat_program != "" && sat_program_exists) {

        // assign a unique number to every state
        i = 0;
        for(state_set::iterator it = merger.context.red_states.begin(); it != merger.context.red_states.end(); ++it){
            apta_node* node = *it;
            node->satnumber = i;
            node->colour = i;
            i++;
        }
        for(state_set::iterator it = merger.context.non_red_states.begin(); it != merger.context.non_red_states.end(); ++it){
            apta_node* node = *it;
            node->satnumber = i;
            i++;
        }
    
        merger.context.clause_counter = 0;
        merger.context.literal_counter = 1;
    
        cerr << "creating literals..." << endl;
        merger.context.create_literals();
    
        cerr << "number of red states: " << merger.context.red_states.size() << endl;
        cerr << "number of non_red states: " << merger.context.non_red_states.size() << endl;
        cerr << "number of sink states: " << merger.context.sink_states.size() << endl;
        cerr << "dfa size: " << merger.context.dfa_size << endl;
        cerr << "sink types: " << merger.context.sinks_size << endl;
        cerr << "new states: " << merger.context.new_states << endl;
        cerr << "new init: " << merger.context.new_init << endl;
    
        merger.context.merger = &merger;

        merger.context.fix_red_values();
        if(USE_SINKS) merger.context.fix_sink_values();
        merger.context.erase_red_conflict_colours();
        merger.context.set_symmetry();
    
        // renumber literals to account for eliminated ones
        merger.context.reset_literals(false);

        merger.context.computing_header = true;
    
        merger.context.clause_counter = 0;
        merger.context.clause_counter += merger.context.print_colours();
        merger.context.clause_counter += merger.context.print_conflicts();
        merger.context.clause_counter += merger.context.print_accept();
        merger.context.clause_counter += merger.context.print_transitions();
        cerr << "total clauses before symmetry: " << merger.context.clause_counter << endl;
        if(SYMMETRY_BREAKING){
            cerr << "Breaking symmetry in SAT" << endl;
            merger.context.clause_counter += merger.context.print_t_transitions();
            merger.context.clause_counter += merger.context.print_p_transitions();
            merger.context.clause_counter += merger.context.print_a_transitions();
            merger.context.clause_counter += merger.context.print_symmetry();
        }
    if(FORCING){
        cerr << "Forcing in SAT" << endl;
        merger.context.clause_counter += merger.context.print_forcing_transitions();
    }

    merger.context.clause_counter += merger.context.print_paths();
    if(USE_SINKS){
        merger.context.clause_counter += merger.context.print_sink_transitions();
        merger.context.clause_counter += merger.context.print_sink_paths();
    }
    cerr << "header: p cnf " << merger.context.literal_counter - 1 << " " << merger.context.clause_counter << endl;
    merger.context.computing_header = false;
    
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
            
            char* copy_sat = strdup(sat_program.c_str());
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
            
            merger.context.sat_stream = (FILE*) fdopen(pipetosat[1], "w");
            //sat_stream = (FILE*) fopen("test.out", "w");
            if (merger.context.sat_stream == 0){
                cerr << "Cannot open pipe to SAT solver: " << strerror(errno) << endl;
                exit(1);
            }
            fprintf(merger.context.sat_stream, "p cnf %i %i\n", merger.context.literal_counter - 1, merger.context.clause_counter);
            
            merger.context.print_colours();
            merger.context.print_conflicts();
            merger.context.print_accept();
            merger.context.print_transitions();
            if(SYMMETRY_BREAKING){
                merger.context.print_symmetry();
                merger.context.print_t_transitions();
                merger.context.print_p_transitions();
                merger.context.print_a_transitions();
            }
            if(FORCING){
                merger.context.print_forcing_transitions();
            }
            merger.context.print_paths();
            if(USE_SINKS) {
                merger.context.print_sink_transitions();
                merger.context.print_sink_paths();
             }
            
            fclose(merger.context.sat_stream);
            
            cerr << "sent problem to SAT solver" << endl;
            
            time_t begin_time = time(nullptr);
            
            merger.context.trueliterals = set<int>();
            
            char line[500];
            merger.context.sat_stream = fdopen ( pipefromsat[0], "r" );
            
            bool improved = false;
            while(fgets ( line, sizeof line, merger.context.sat_stream ) != NULL){
                char* pch = strtok (line," ");
                if(strcmp(pch,"s") == 0){
                    pch = strtok (NULL, " ");
                    cerr << pch << endl;
                    if(strcmp(pch,"SATISFIABLE\n")==0){
                        cerr << "new solution, size = " << merger.context.dfa_size << endl;
                        if(merger.context.best_solution ==-1 || merger.context.best_solution > merger.context.dfa_size){
                            cerr << "new best solution, size = " << merger.context.dfa_size << endl;
                            merger.context.best_solution = merger.context.dfa_size;
                            improved = true;
                        }
                    }
                }
                if(strcmp(pch,"v") == 0){
                    pch = strtok (NULL, " ");
                    while(pch != NULL){
                        int val = atoi(pch);
                        if(val > 0) merger.context.trueliterals.insert(val);
                        pch = strtok (NULL, " ");
                    }
                }
            }
            fclose(merger.context.sat_stream);
            
            cerr << "solving took " << (time(nullptr) - begin_time) << " seconds" << endl;
            
            if(improved){
                merger.context.print_dot_output(dot_output_file);
                merger.context.print_aut_output(aut_output);
            }
            
            while(!refs->empty()){
                refinement* ref = refs->front();
                refs->pop_front();
                ref->undo(&merger);
                delete ref;
            }
            delete refs;
        }
        merger.context.delete_literals();
    }
    else {
        cout << "No valid solver specified, skipping..." << endl;
    }
 
    return merger.context.best_solution;
};
