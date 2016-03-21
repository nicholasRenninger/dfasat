
#ifndef _EVALUATE_H_
#define _EVALUATE_H_

#include <vector>
#include <set>
#include <list>
#include <map>

#include "state_merger.h"

using namespace std;

extern int HEURISTIC;

const int EDSM = 1;
const int OVERLAP = 2;
const int COUNT_DRIVEN = 3;
const int LIKELIHOODRATIO = 4;
const int AIC = 5;
const int KULLBACKLEIBLER = 6;

extern int STATE_COUNT;
extern int SYMBOL_COUNT;
extern float CORRECTION;
extern float CHECK_PARAMETER;
extern bool USE_SINKS;
extern float LOWER_BOUND;

/* Return a sink type, or -1 if no sink 
 * Sinks are special states that optionally are not considered as merge candidates, 
 * and are optionally merged into one (for every type) before starting exact solving */
int sink_type(apta_node* node);
bool sink_consistent(apta_node* node, int type);
int num_sink_types();

class evaluation_data{
    /* counts of positive and negative transition uses */
    num_map num_pos;
    num_map num_neg;
    
    /* depth of the node in the apta */
    int depth;
    int old_depth;
    
    /* counts of positive and negative endings */
    int num_accepting;
    int num_rejecting;
    
    /* counts of positive and negative traversals */
    int accepting_paths;
    int rejecting_paths;

    merge_update(apta_node* right);
    unmerge_update(apta_node* right);
}

class evaluation_function{
public:
  int num_merges;
  bool inconsistency_found;
  
/* Boolean indicating the evaluation function type;
   there are two kinds: computed before or after/during a merge.
   When computed before a merge, a merge is only tried for consistency. 
   Functions computed before merging (typically) do not take loops that
   the merge creates into account.
   Functions computed after/during a merge rely heavily on the determinization
   process for computation, this is a strong assumption. */
  bool compute_before_merge;

/* An evaluation function needs to implement all of these functions */
  
/* Called when performing a merge, for every pair of merged nodes, 
* compute the local consistency of a merge 
* 
* huge influence on performance, needs to be simple */
  virtual bool consistent(state_merger*, apta_node* left, apta_node* right);
  virtual void update_score(state_merger*, apta_node* left, apta_node* right);
  
/* Called when testing a merge
* compute the score and consistency of a merge, and reset counters/structures
* 
* influence on performance, needs to be somewhat simple */
  virtual bool compute_consistency(state_merger *, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger *, apta_node* left, apta_node* right);
  virtual void reset(state_merger *);

/* Called after an update, 
* when a merge has been performed successfully
* updates the structures used for computing heuristics/consistency 
* 
* not called when testing merges, can therefore be somewhat complex
* without a huge influence on performance*/
  virtual void update(state_merger *);

/* Called after initialization of the APTA, 
* creates structures and initializes values used for computing heuristics/consistency 
* 
* called only once for every run, can be complex */
  virtual void initialize(state_merger *);
};

class evidence_driven: public evaluation_function{
public:
  int num_pos;
  int num_neg;
  
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

class depth_driven: public evaluation_function{
public:
  int depth;
  
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

class overlap_driven: public evaluation_function{
public:
  int overlap;
  
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};

class series_driven: public overlap_driven{
  virtual void score_right(apta_node* right, int depth);
  virtual void score_left(apta_node* left, int depth);

public:
  vector< state_set > left_dist;
  vector< state_set > right_dist;

  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void initialize(state_merger *);
  virtual void reset(state_merger *merger);
};

/* metric driven merging added for netflow by chrham */
class metric_driven: public evaluation_function{
public:
  int overlap;
  
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
};
/* end addition */

class alergia: public depth_driven{
public:
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
};

class likelihoodratio: public evaluation_function{
public:
  double loglikelihood_orig;
  double loglikelihood_merged;
  int extra_parameters;
  
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

class aic: public likelihoodratio{
public:
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual int  compute_score(state_merger*, apta_node* left, apta_node* right);
};

class kldistance: public depth_driven{
public:
  double perplexity;
  int extra_parameters;
  
  virtual void update_score(state_merger *merger, apta_node* left, apta_node* right);
  virtual bool compute_consistency(state_merger *merger, apta_node* left, apta_node* right);
  virtual void reset(state_merger *merger);
};

#endif /* _EVALUATE_H_ */
