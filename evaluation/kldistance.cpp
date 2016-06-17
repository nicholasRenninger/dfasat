#include <stdlib.h>
#include <math.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <gsl/gsl_cdf.h>

#include "state_merger.h"
#include "evaluate.h"
#include "kldistance.h"

#include "parameters.h"

REGISTER_DEF_DATATYPE(kl_data);
REGISTER_DEF_TYPE(kldistance);

void kl_data::update(evaluation_data* right){
    alergia_data::update(right);
    kl_data* other = (kl_data*)right;
    for(prob_map::iterator it = other->original_probability_count.begin();it != other->original_probability_count.end(); ++it){
        original_probability_count[(*it).first] = opc((*it).first) + (*it).second;
    }
};

void kl_data::undo(evaluation_data* right){
    alergia_data::undo(right);
    kl_data* other = (kl_data*)right;
    for(prob_map::iterator it = other->original_probability_count.begin();it != other->original_probability_count.end(); ++it){
        original_probability_count[(*it).first] = opc((*it).first) - (*it).second;
    }
};

bool kldistance::consistent(state_merger *merger, apta_node* left, apta_node* right){
    return count_driven::consistent(merger, left, right);
};

void kldistance::update_perplexity(apta_node* left, double op_count_left, double op_count_right, double count_left, double count_right, double left_divider, double right_divider){
    //cerr << "\t" << op_count_left << " " << op_count_right << " " << count_left << " " << count_right << endl;
    //cerr << "\t" << count_left / left_divider << " " << count_right / right_divider << endl;
    if(already_merged(left) == false){
        if(count_left != 0){
            perplexity += op_count_left * log(count_left / left_divider);
            perplexity -= op_count_left * log((count_left + count_right) / (left_divider + right_divider));
        }
        if(count_right != 0){
            perplexity += op_count_right * log(count_right / right_divider);
            perplexity -= op_count_right * log((count_left + count_right) / (left_divider + right_divider));
        }
    } else {
        if(count_left != 0){
            perplexity += op_count_left * log(count_left / left_divider);
            perplexity -= op_count_left * log((count_left + count_right) / (left_divider + right_divider));
        }
        if(count_right != 0){
            perplexity += op_count_right * log(count_right / right_divider);
            perplexity -= op_count_right * log((count_left + count_right) / (left_divider + right_divider));
        }
    }
    if(count_left > 0.0 && count_right > 0.0) extra_parameters = extra_parameters + 1;
    //cerr << perplexity << endl;
};

/* Kullback-Leibler divergence (KL), MDI-like, computes the KL value/extra parameters and uses it as score and consistency */
void kldistance::update_score(state_merger *merger, apta_node* left, apta_node* right){
    evaluation_function::update_score(merger, left, right);
    kl_data* l = (kl_data*) left->data;
    kl_data* r = (kl_data*) right->data;

    if(r->accepting_paths < 0 || l->accepting_paths < 0) return;

    double left_divider = (double)l->accepting_paths;
    double right_divider = (double)r->accepting_paths;
    
    for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
        update_perplexity(left, l->opc((*it).first), r->opc((*it).first), (*it).second, r->pos((*it).first), left_divider, right_divider);
    }
};

bool kldistance::compute_consistency(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found) return false;
  if (extra_parameters == 0) return false;

  if ((perplexity / (double)extra_parameters) > CHECK_PARAMETER) return false;

  return true;
};

int kldistance::compute_score(state_merger *merger, apta_node* left, apta_node* right){
  if (inconsistency_found == true) return false;
  if (extra_parameters == 0) return -1;

  double val = (perplexity / (double)extra_parameters);
  
  cerr << val << endl;
  
  return 100000 - (int)(val * 100.0);
};

void kldistance::reset(state_merger *merger){
  alergia::reset(merger);
  inconsistency_found = false;
  perplexity = 0;
  extra_parameters = 0;
};

void kldistance::print_dot(FILE* output, state_merger* merger){
    count_driven::print_dot(output, merger);
};

void kldistance::initialize(state_merger* merger){
    state_set *state = &merger->aut->get_merged_states();
    state_set  states = *state;
    for(state_set::iterator it = states.begin(); it != states.end(); ++it){
        apta_node* node = *it;
        kl_data* l = (kl_data*) node->data;
        
        if(l->accepting_paths < STATE_COUNT) continue;

        for(num_map::iterator it = l->num_pos.begin(); it != l->num_pos.end(); ++it){
            int symbol = (*it).first;
            double count  = (double)(*it).second;
            l->original_probability_count[symbol] = count * (count / (double)l->accepting_paths);
        }
    }
    delete state;
};



