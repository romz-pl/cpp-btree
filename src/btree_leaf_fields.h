#ifndef ROMZ_CPP_BTREE_LEAF_FIELDS_H
#define ROMZ_CPP_BTREE_LEAF_FIELDS_H


#include "btree_base_fields.h"

namespace btree
{

template < typename Params >
class btree_node;

template < typename Params >
struct btree_leaf_fields : public btree_base_fields< Params >
{
    typedef typename Params::mutable_value_type mutable_value_type;

    // The array of values.
    // Only the first count of these values have been constructed and are valid.
    mutable_value_type values[ btree_node< Params >::kNodeValues ];
};

}


#endif
