#ifndef ROMZ_CPP_BTREE_INTERNAL_FIELDS_H
#define ROMZ_CPP_BTREE_INTERNAL_FIELDS_H


#include "btree_leaf_fields.h"

namespace btree
{

template < typename Params >
class btree_node;

template < typename Params >
struct btree_internal_fields : public btree_leaf_fields< Params >
{
    // The array of child pointers.
    // The keys in children[ i ] are all less than key( i ).
    // The keys in children[ i + 1 ] are all greater than key( i ).
    // There are always (count + 1) children.
    btree_node< Params > *children[ btree_node< Params >::kNodeValues + 1 ];
};


}


#endif
