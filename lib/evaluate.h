#ifndef _EVALUATE_H_
#define _EVALUATE_H_

#include <vector>
#include <set>
#include <list>
#include <map>
#include <sstream>

class evaluation_data;
class evaluation_function;

class apta_node;

#include "evaluation_factory.h"
#include "apta.h"
#include "state_merger.h"

using namespace std;

bool is_stream_sink(apta_node*);


// for registering evaluation data objects
#define REGISTER_DEC_DATATYPE(NAME) \
    static DerivedDataRegister<NAME> reg

#define REGISTER_DEF_DATATYPE(NAME) \
    DerivedDataRegister<NAME> NAME::reg(#NAME)


// for registering evaluation function objects
#define REGISTER_DEC_TYPE(NAME) \
    static DerivedRegister<NAME> reg

#define REGISTER_DEF_TYPE(NAME) \
    DerivedRegister<NAME> NAME::reg(#NAME)


// for auto-generating names
inline std::string className(const std::string& prettyFunction)
{
    size_t colons = prettyFunction.find("::");
    if (colons == std::string::npos)
        return "::";
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = colons - begin;

    return prettyFunction.substr(begin,end);
}

#define __CLASS_NAME__ className(__PRETTY_FUNCTION__)

/**
 * @brief Extra data attached to each symbol, stored in each node.
 *
 * Stores all the extra data attached to symbols in a node. The 
 * class is used by the evaluation_function class.
 * It also provides the sink functions determining when to ignore
 * a node due to user-defined criteria, i.e. low frquency counts.
 * Important: Each evaluation_data type has to be registered by
 * calling REGISTER_DEF_DATATYPE(<unique name>).
 * @see evaluation_function
 */
class evaluation_data {

protected:
    static DerivedDataRegister<evaluation_data> reg;

public:

    int node_type;
    evaluation_data* undo_pointer;
    
    evaluation_data();
    
/* Set values from input string */
    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void read_to(int type, int index, int length, int symbol, string data);
/* Update values when merging */
    virtual void update(evaluation_data* other);
/* Undo updates when undoing merge */
    virtual void undo(evaluation_data* other);

/* Printing of nodes and transitions in dot output */
    virtual void print_state_label(iostream& output, apta* aptacontext);
    virtual void print_state_style(iostream& output, apta* aptacontext);
    virtual void print_transition_label(iostream& output, int symbol, apta* aptacontext);
    virtual void print_transition_style(iostream& output, set<int> symbols, apta* aptacontext);

/* what to ignore, and why */    
    
    virtual bool sink_consistent(int type);

/* Return a sink type, or -1 if no sink
 * Sinks are special states that optionally are not considered as merge candidates,
 * and are optionally merged into one (for every type) before starting exact solving */
  virtual int sink_type(apta_node* node);
  virtual bool sink_consistent(apta_node* node, int type);
  virtual int num_sink_types();

};


/**
 * @brief User-defined merge heuristic function.
 *
 * Important: Each evaluation_data type has to be registered by
 * calling REGISTER_DEF_TYPE(<unique name>).
 * @see evaluation_data
 */
class evaluation_function  {

protected:
  static DerivedRegister<evaluation_function> reg;

public:

/* Global data */
  bool inconsistency_found;
  int num_merges;
    
/* Boolean indicating the evaluation function type;
   are are two kinds: computed before or after/during a merge.
   When computed before a merge, a merge is only tried for consistency.
   Functions computed before merging (typically) do not take loops that
   the merge creates into account.
   Functions computed after/during a merge rely heavily on the determinization
   process for computation, this is a strong assumption. */
  bool compute_before_merge;

/* A set containing the left states that have been merged already
   some evaluation methods use it for making different calculations */
  state_set merged_left_states;
  inline bool already_merged(apta_node* left){
    return merged_left_states.find(left) != merged_left_states.end();
  };

/* An evaluation function needs to implement all of these functions */

/* Called when performing a merge, for every pair of merged nodes,
* compute the local consistency of a merge and update stored data values
*
* huge influence on performance, needs to be simple */
  virtual bool consistency_check(evaluation_data* l, evaluation_data* r);
  virtual bool consistent(state_merger*, apta_node* left, apta_node* right);
  virtual void update_score(state_merger*, apta_node* left, apta_node* right);
  virtual void undo_update(state_merger*, apta_node* left, apta_node* right);

/* Called when testing a merge
* compute the score and consistency of a merge, and reset global counters/structures
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
  
 //virtual void read_file(istream &input_stream, state_merger *);
  virtual void init(string data, state_merger* merger);
  virtual void add_sample(string data, state_merger* merger);
  //virtual void print_dot(iostream&, state_merger *);
};


#endif /* _EVALUATE_H_ */
