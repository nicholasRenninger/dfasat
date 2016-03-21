#ifndef _EVALUATE_H_
#define _EVALUATE_H_

#include <vector>
#include <set>
#include <list>
#include <map>

#include "evaluation_factory.h"
#include "apta.h"
#include "state_merger.h"

using namespace std;

#define REGISTER_DEC_TYPE(NAME) \
    static DerivedRegister<NAME> reg

#define REGISTER_DEF_TYPE(NAME) \
    DerivedRegister<NAME> NAME::reg(#NAME)

/* The data contained in every node of the prefix tree or DFA */
class evaluation_data {
  public:
  
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

	double_list occs;
    double_list::iterator occ_merge_point;

	node_list conflicts;
    node_list::iterator merge_point;
};

class evaluation_function  {

protected:
  static DerivedRegister<evaluation_function> reg;

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
  
/* Return a sink type, or -1 if no sink
 * Sinks are special states that optionally are not considered as merge candidates,
 * and are optionally merged into one (for every type) before starting exact solving */
  virtual int sink_type(apta_node* node);
  virtual bool sink_consistent(apta_node* node, int type);
  virtual int num_sink_types();

  virtual void read_file(FILE*, state_merger *);
  virtual void print_dot(FILE*, state_merger *);
};


#endif /* _EVALUATE_H_ */
