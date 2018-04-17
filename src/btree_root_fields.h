#ifndef ROMZ_CPP_BTREE_ROOT_FIELDS_H
#define ROMZ_CPP_BTREE_ROOT_FIELDS_H


#include "btree_internal_fields.h"

namespace btree
{

template < typename Params >
class btree_node;

template < typename Params >
struct btree_root_fields : public btree_internal_fields< Params >
{
    typedef typename Params::size_type size_type;

    btree_node< Params > *rightmost;
    size_type size;
};



}


#endif
