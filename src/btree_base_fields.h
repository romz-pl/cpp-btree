#ifndef ROMZ_CPP_BTREE_BASE_FIELDS_H
#define ROMZ_CPP_BTREE_BASE_FIELDS_H


namespace btree
{

template < typename Params >
class btree_node;

template < typename Params >
struct btree_base_fields
{
    typedef typename Params::node_count_type field_type;

    // A boolean indicating whether the node is a leaf or not.
    bool leaf;

    // The position of the node in the node's parent.
    field_type position;

    // The maximum number of values the node can hold.
    field_type max_count;

    // The count of the number of values in the node.
    field_type count;

    // A pointer to the node's parent.
    btree_node< Params > *parent;
};

}


#endif
