#ifndef __ALERGIA__
#define __ALERGIA__

#include "evaluate.h"
#include "num_count.h"

typedef map<int, int> num_map;

/* The data contained in every node of the prefix tree or DFA */
class alergia_data: public count_data {

protected:
  REGISTER_DEC_DATATYPE(alergia_data);

public:
    /* counts of positive and negative transition uses */
    num_map num_pos;
    num_map num_neg;
    
    inline int pos(int i){
        num_map::iterator it = num_pos.find(i);
        if(it == num_pos.end()) return 0;
        return (*it).second;
    }

    inline int neg(int i){
        num_map::iterator it = num_neg.find(i);
        if(it == num_neg.end()) return 0;
        return (*it).second;
    }
    
    alergia_data();

    virtual void read_from(int type, int index, int length, int symbol, string data);
    virtual void update(evaluation_data* right);
    virtual void undo(evaluation_data* right);
    
    virtual bool is_low_count_sink();
    virtual int sink_type();
    virtual bool sink_consistent(int type);
};


class alergia: public count_driven {

protected:
  REGISTER_DEC_TYPE(alergia);

public:
  static bool alergia_consistency(double right_count, double left_count, double right_total, double left_total);

  virtual bool data_consistent(alergia_data* l, alergia_data* r);
  virtual bool consistent(state_merger *merger, apta_node* left, apta_node* right);
  //virtual void print_dot(iostream&, state_merger *);

  //virtual int sink_type(apta_node* node);
  //virtual bool sink_consistent(apta_node* node, int type);
  virtual int num_sink_types();
};

#endif
