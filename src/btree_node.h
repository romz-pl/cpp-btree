#ifndef ROMZ_CPP_BTREE_BTREE_NODE_H
#define ROMZ_CPP_BTREE_BTREE_NODE_H

//
// A node in the btree holding. The same node type is used for both internal
// and leaf nodes in the btree, though the nodes are allocated in such a way
// that the children array is only valid in internal nodes.
//

#include "btree_compare.h"
#include "btree_swap_helper.h"
#include "btree_base_fields.h"
#include "btree_leaf_fields.h"

namespace btree
{
template < typename Params >
class btree_node
{
public:
    typedef Params params_type;
    typedef btree_node< Params > self_type;

    typedef typename Params::key_type           key_type;
    typedef typename Params::data_type          data_type;
    typedef typename Params::value_type         value_type;
    typedef typename Params::mutable_value_type mutable_value_type;
    typedef typename Params::pointer            pointer;
    typedef typename Params::const_pointer      const_pointer;
    typedef typename Params::reference          reference;
    typedef typename Params::const_reference    const_reference;
    typedef typename Params::key_compare        key_compare;
    typedef typename Params::size_type          size_type;
    typedef typename Params::difference_type    difference_type;

    typedef btree_leaf_fields<Params> leaf_fields;

    //
    // Typedefs for the various types of node searches.
    //
    typedef btree_linear_search_plain_compare< key_type, self_type, key_compare > linear_search_plain_compare_type;
    typedef btree_linear_search_compare_to   < key_type, self_type, key_compare > linear_search_compare_to_type;
    typedef btree_binary_search_plain_compare< key_type, self_type, key_compare > binary_search_plain_compare_type;
    typedef btree_binary_search_compare_to   < key_type, self_type, key_compare > binary_search_compare_to_type;

    //
    // If we have a valid key-compare-to type, use linear_search_compare_to, otherwise use linear_search_plain_compare.
    //
    typedef typename std::conditional<
                        Params::is_key_compare_to::value,
                        linear_search_compare_to_type,
                        linear_search_plain_compare_type >::type linear_search_type;

    //
    // If we have a valid key-compare-to type, use binary_search_compare_to, otherwise use binary_search_plain_compare.
    //
    typedef typename std::conditional<
                        Params::is_key_compare_to::value,
                        binary_search_compare_to_type,
                        binary_search_plain_compare_type>::type binary_search_type;

    //
    // If the key is an integral or floating point type, use linear search which is faster than binary search for such types.
    // Might be wise to also configure linear search based on node-size.
    //
    typedef typename std::conditional<
                        std::is_integral< key_type >::value || std::is_floating_point< key_type >::value,
                        linear_search_type,
                        binary_search_type >::type search_type;




    enum
    {
        kValueSize = params_type::kValueSize,
        kTargetNodeSize = params_type::kTargetNodeSize,

        // Compute how many values we can fit onto a leaf node.
        kNodeTargetValues = ( kTargetNodeSize - sizeof( btree_base_fields< Params > ) ) / kValueSize,

        // We need a minimum of 3 values per internal node in order to perform
        // splitting (1 value for the two nodes involved in the split and 1 value
        // propagated to the parent as the delimiter for the split).
        kNodeValues = kNodeTargetValues >= 3 ? kNodeTargetValues : 3,

        kExactMatch = 1 << 30,
        kMatchMask = kExactMatch - 1,
    };



    struct internal_fields : public leaf_fields
    {
        // The array of child pointers.
        // The keys in children[ i ] are all less than key( i ).
        // The keys in children[ i + 1 ] are all greater than key( i ).
        // There are always (count + 1) children.
        btree_node *children[ kNodeValues + 1 ];
    };

    struct root_fields : public internal_fields
    {
        btree_node *rightmost;
        size_type size;
    };

 public:
  // Getter/setter for whether this is a leaf node or not. This value doesn't
  // change after the node is created.
  bool leaf() const { return fields_.leaf; }

  // Getter for the position of this node in its parent.
  int position() const { return fields_.position; }
  void set_position(int v) { fields_.position = v; }

  // Getter/setter for the number of values stored in this node.
  int count() const { return fields_.count; }
  void set_count(int v) { fields_.count = v; }
  int max_count() const { return fields_.max_count; }

  // Getter for the parent of this node.
  btree_node* parent() const { return fields_.parent; }
  // Getter for whether the node is the root of the tree. The parent of the
  // root of the tree is the leftmost node in the tree which is guaranteed to
  // be a leaf.
  bool is_root() const { return parent()->leaf(); }
  void make_root() {
    assert(parent()->is_root());
    fields_.parent = fields_.parent->parent();
  }

  // Getter for the rightmost root node field. Only valid on the root node.
  btree_node* rightmost() const { return fields_.rightmost; }
  btree_node** mutable_rightmost() { return &fields_.rightmost; }

  // Getter for the size root node field. Only valid on the root node.
  size_type size() const { return fields_.size; }
  size_type* mutable_size() { return &fields_.size; }

  // Getters for the key/value at position i in the node.
  const key_type& key(int i) const {
    return params_type::key(fields_.values[i]);
  }
  reference value(int i) {
    return reinterpret_cast<reference>(fields_.values[i]);
  }
  const_reference value(int i) const {
    return reinterpret_cast<const_reference>(fields_.values[i]);
  }
  mutable_value_type* mutable_value(int i) {
    return &fields_.values[i];
  }

  // Swap value i in this node with value j in node x.
  void value_swap(int i, btree_node *x, int j) {
    params_type::swap(mutable_value(i), x->mutable_value(j));
  }

  // Getters/setter for the child at position i in the node.
  btree_node* child(int i) const { return fields_.children[i]; }
  btree_node** mutable_child(int i) { return &fields_.children[i]; }
  void set_child(int i, btree_node *c) {
    *mutable_child(i) = c;
    c->fields_.parent = this;
    c->fields_.position = i;
  }

  // Returns the position of the first value whose key is not less than k.
  template <typename Compare>
  int lower_bound(const key_type &k, const Compare &comp) const {
    return search_type::lower_bound(k, *this, comp);
  }
  // Returns the position of the first value whose key is greater than k.
  template <typename Compare>
  int upper_bound(const key_type &k, const Compare &comp) const {
    return search_type::upper_bound(k, *this, comp);
  }

  // Returns the position of the first value whose key is not less than k using
  // linear search performed using plain compare.
  template <typename Compare>
  int linear_search_plain_compare(
      const key_type &k, int s, int e, const Compare &comp) const {
    while (s < e) {
      if (!btree_compare_keys(comp, key(s), k)) {
        break;
      }
      ++s;
    }
    return s;
  }

  // Returns the position of the first value whose key is not less than k using
  // linear search performed using compare-to.
  template <typename Compare>
  int linear_search_compare_to(
      const key_type &k, int s, int e, const Compare &comp) const {
    while (s < e) {
      int c = comp(key(s), k);
      if (c == 0) {
        return s | kExactMatch;
      } else if (c > 0) {
        break;
      }
      ++s;
    }
    return s;
  }

  // Returns the position of the first value whose key is not less than k using
  // binary search performed using plain compare.
  template <typename Compare>
  int binary_search_plain_compare(
      const key_type &k, int s, int e, const Compare &comp) const {
    while (s != e) {
      int mid = (s + e) / 2;
      if (btree_compare_keys(comp, key(mid), k)) {
        s = mid + 1;
      } else {
        e = mid;
      }
    }
    return s;
  }

  // Returns the position of the first value whose key is not less than k using
  // binary search performed using compare-to.
  template <typename CompareTo>
  int binary_search_compare_to(
      const key_type &k, int s, int e, const CompareTo &comp) const {
    while (s != e) {
      int mid = (s + e) / 2;
      int c = comp(key(mid), k);
      if (c < 0) {
        s = mid + 1;
      } else if (c > 0) {
        e = mid;
      } else {
        // Need to return the first value whose key is not less than k, which
        // requires continuing the binary search. Note that we are guaranteed
        // that the result is an exact match because if "key(mid-1) < k" the
        // call to binary_search_compare_to() will return "mid".
        s = binary_search_compare_to(k, s, mid, comp);
        return s | kExactMatch;
      }
    }
    return s;
  }

  // Inserts the value x at position i, shifting all existing values and
  // children at positions >= i to the right by 1.
  void insert_value(int i, const value_type &x);

  // Removes the value at position i, shifting all existing values and children
  // at positions > i to the left by 1.
  void remove_value(int i);

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(btree_node *sibling, int to_move);
  void rebalance_left_to_right(btree_node *sibling, int to_move);

  // Splits a node, moving a portion of the node's values to its right sibling.
  void split(btree_node *sibling, int insert_position);

  // Merges a node with its right sibling, moving all of the values and the
  // delimiting key in the parent node onto itself.
  void merge(btree_node *sibling);

  // Swap the contents of "this" and "src".
  void swap(btree_node *src);

  // Node allocation/deletion routines.
  static btree_node* init_leaf(
      leaf_fields *f, btree_node *parent, int max_count) {
    btree_node *n = reinterpret_cast<btree_node*>(f);
    f->leaf = 1;
    f->position = 0;
    f->max_count = max_count;
    f->count = 0;
    f->parent = parent;
#ifndef NDEBUG
      memset(&f->values, 0, max_count * sizeof(value_type));
#endif
    return n;
  }
  static btree_node* init_internal(internal_fields *f, btree_node *parent) {
    btree_node *n = init_leaf(f, parent, kNodeValues);
    f->leaf = 0;
#ifndef NDEBUG
      memset(f->children, 0, sizeof(f->children));
#endif
    return n;
  }
  static btree_node* init_root(root_fields *f, btree_node *parent) {
    btree_node *n = init_internal(f, parent);
    f->rightmost = parent;
    f->size = parent->count();
    return n;
  }
  void destroy() {
    for (int i = 0; i < count(); ++i) {
      value_destroy(i);
    }
  }

 private:
  void value_init(int i) {
    new (&fields_.values[i]) mutable_value_type;
  }
  void value_init(int i, const value_type &x) {
    new (&fields_.values[i]) mutable_value_type(x);
  }
  void value_destroy(int i) {
    fields_.values[i].~mutable_value_type();
  }

 private:
  root_fields fields_;

 private:
  btree_node(const btree_node&);
  void operator=(const btree_node&);
};





////
// btree_node methods
template <typename P>
inline void btree_node<P>::insert_value(int i, const value_type &x) {
  assert(i <= count());
  value_init(count(), x);
  for (int j = count(); j > i; --j) {
    value_swap(j, this, j - 1);
  }
  set_count(count() + 1);

  if (!leaf()) {
    ++i;
    for (int j = count(); j > i; --j) {
      *mutable_child(j) = child(j - 1);
      child(j)->set_position(j);
    }
    *mutable_child(i) = NULL;
  }
}

template <typename P>
inline void btree_node<P>::remove_value(int i) {
  if (!leaf()) {
    assert(child(i + 1)->count() == 0);
    for (int j = i + 1; j < count(); ++j) {
      *mutable_child(j) = child(j + 1);
      child(j)->set_position(j);
    }
    *mutable_child(count()) = NULL;
  }

  set_count(count() - 1);
  for (; i < count(); ++i) {
    value_swap(i, this, i + 1);
  }
  value_destroy(i);
}

template <typename P>
void btree_node<P>::rebalance_right_to_left(btree_node *src, int to_move) {
  assert(parent() == src->parent());
  assert(position() + 1 == src->position());
  assert(src->count() >= count());
  assert(to_move >= 1);
  assert(to_move <= src->count());

  // Make room in the left node for the new values.
  for (int i = 0; i < to_move; ++i) {
    value_init(i + count());
  }

  // Move the delimiting value to the left node and the new delimiting value
  // from the right node.
  value_swap(count(), parent(), position());
  parent()->value_swap(position(), src, to_move - 1);

  // Move the values from the right to the left node.
  for (int i = 1; i < to_move; ++i) {
    value_swap(count() + i, src, i - 1);
  }
  // Shift the values in the right node to their correct position.
  for (int i = to_move; i < src->count(); ++i) {
    src->value_swap(i - to_move, src, i);
  }
  for (int i = 1; i <= to_move; ++i) {
    src->value_destroy(src->count() - i);
  }

  if (!leaf()) {
    // Move the child pointers from the right to the left node.
    for (int i = 0; i < to_move; ++i) {
      set_child(1 + count() + i, src->child(i));
    }
    for (int i = 0; i <= src->count() - to_move; ++i) {
      assert(i + to_move <= src->max_count());
      src->set_child(i, src->child(i + to_move));
      *src->mutable_child(i + to_move) = NULL;
    }
  }

  // Fixup the counts on the src and dest nodes.
  set_count(count() + to_move);
  src->set_count(src->count() - to_move);
}

template <typename P>
void btree_node<P>::rebalance_left_to_right(btree_node *dest, int to_move) {
  assert(parent() == dest->parent());
  assert(position() + 1 == dest->position());
  assert(count() >= dest->count());
  assert(to_move >= 1);
  assert(to_move <= count());

  // Make room in the right node for the new values.
  for (int i = 0; i < to_move; ++i) {
    dest->value_init(i + dest->count());
  }
  for (int i = dest->count() - 1; i >= 0; --i) {
    dest->value_swap(i, dest, i + to_move);
  }

  // Move the delimiting value to the right node and the new delimiting value
  // from the left node.
  dest->value_swap(to_move - 1, parent(), position());
  parent()->value_swap(position(), this, count() - to_move);
  value_destroy(count() - to_move);

  // Move the values from the left to the right node.
  for (int i = 1; i < to_move; ++i) {
    value_swap(count() - to_move + i, dest, i - 1);
    value_destroy(count() - to_move + i);
  }

  if (!leaf()) {
    // Move the child pointers from the left to the right node.
    for (int i = dest->count(); i >= 0; --i) {
      dest->set_child(i + to_move, dest->child(i));
      *dest->mutable_child(i) = NULL;
    }
    for (int i = 1; i <= to_move; ++i) {
      dest->set_child(i - 1, child(count() - to_move + i));
      *mutable_child(count() - to_move + i) = NULL;
    }
  }

  // Fixup the counts on the src and dest nodes.
  set_count(count() - to_move);
  dest->set_count(dest->count() + to_move);
}

template <typename P>
void btree_node<P>::split(btree_node *dest, int insert_position) {
  assert(dest->count() == 0);

  // We bias the split based on the position being inserted. If we're
  // inserting at the beginning of the left node then bias the split to put
  // more values on the right node. If we're inserting at the end of the
  // right node then bias the split to put more values on the left node.
  if (insert_position == 0) {
    dest->set_count(count() - 1);
  } else if (insert_position == max_count()) {
    dest->set_count(0);
  } else {
    dest->set_count(count() / 2);
  }
  set_count(count() - dest->count());
  assert(count() >= 1);

  // Move values from the left sibling to the right sibling.
  for (int i = 0; i < dest->count(); ++i) {
    dest->value_init(i);
    value_swap(count() + i, dest, i);
    value_destroy(count() + i);
  }

  // The split key is the largest value in the left sibling.
  set_count(count() - 1);
  parent()->insert_value(position(), value_type());
  value_swap(count(), parent(), position());
  value_destroy(count());
  parent()->set_child(position() + 1, dest);

  if (!leaf()) {
    for (int i = 0; i <= dest->count(); ++i) {
      assert(child(count() + i + 1) != NULL);
      dest->set_child(i, child(count() + i + 1));
      *mutable_child(count() + i + 1) = NULL;
    }
  }
}

template <typename P>
void btree_node<P>::merge(btree_node *src) {
  assert(parent() == src->parent());
  assert(position() + 1 == src->position());

  // Move the delimiting value to the left node.
  value_init(count());
  value_swap(count(), parent(), position());

  // Move the values from the right to the left node.
  for (int i = 0; i < src->count(); ++i) {
    value_init(1 + count() + i);
    value_swap(1 + count() + i, src, i);
    src->value_destroy(i);
  }

  if (!leaf()) {
    // Move the child pointers from the right to the left node.
    for (int i = 0; i <= src->count(); ++i) {
      set_child(1 + count() + i, src->child(i));
      *src->mutable_child(i) = NULL;
    }
  }

  // Fixup the counts on the src and dest nodes.
  set_count(1 + count() + src->count());
  src->set_count(0);

  // Remove the value on the parent node.
  parent()->remove_value(position());
}

template <typename P>
void btree_node<P>::swap(btree_node *x) {
  assert(leaf() == x->leaf());

  // Swap the values.
  for (int i = count(); i < x->count(); ++i) {
    value_init(i);
  }
  for (int i = x->count(); i < count(); ++i) {
    x->value_init(i);
  }
  int n = std::max(count(), x->count());
  for (int i = 0; i < n; ++i) {
    value_swap(i, x, i);
  }
  for (int i = count(); i < x->count(); ++i) {
    x->value_destroy(i);
  }
  for (int i = x->count(); i < count(); ++i) {
    value_destroy(i);
  }

  if (!leaf()) {
    // Swap the child pointers.
    for (int i = 0; i <= n; ++i) {
      btree_swap_helper(*mutable_child(i), *x->mutable_child(i));
    }
    for (int i = 0; i <= count(); ++i) {
      x->child(i)->fields_.parent = x;
    }
    for (int i = 0; i <= x->count(); ++i) {
      child(i)->fields_.parent = this;
    }
  }

  // Swap the counts.
  btree_swap_helper(fields_.count, x->fields_.count);
}

}

#endif
