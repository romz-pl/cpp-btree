#ifndef ROMZ_CPP_BTREE_BTREE_H
#define ROMZ_CPP_BTREE_BTREE_H


#include "btree_compare.h"
#include "btree_iterator.h"
#include "btree_node.h"

namespace btree
{

template <typename Params>
class btree : public Params::key_compare {
  typedef btree<Params> self_type;
  typedef btree_node<Params> node_type;
  typedef btree_base_fields<Params> base_fields;
  typedef btree_leaf_fields<Params> leaf_fields;
  typedef btree_internal_fields<Params> internal_fields;
  typedef btree_root_fields<Params> root_fields;
  typedef typename Params::is_key_compare_to is_key_compare_to;

  friend class btree_internal_locate_plain_compare;
  friend class btree_internal_locate_compare_to;
  typedef typename std::conditional<
    is_key_compare_to::value,
    btree_internal_locate_compare_to,
    btree_internal_locate_plain_compare>::type internal_locate_type;

  enum {
    kNodeValues = node_type::kNodeValues,
    kMinNodeValues = kNodeValues / 2,
    kValueSize = node_type::kValueSize,
    kExactMatch = node_type::kExactMatch,
    kMatchMask = node_type::kMatchMask,
  };

  // A helper class to get the empty base class optimization for 0-size
  // allocators. Base is internal_allocator_type.
  // (e.g. empty_base_handle<internal_allocator_type, node_type*>). If Base is
  // 0-size, the compiler doesn't have to reserve any space for it and
  // sizeof(empty_base_handle) will simply be sizeof(Data). Google [empty base
  // class optimization] for more details.
  template <typename Base, typename Data>
  struct empty_base_handle : public Base {
    empty_base_handle(const Base &b, const Data &d)
        : Base(b),
          data(d) {
    }
    Data data;
  };

  struct node_stats {
    node_stats(ssize_t l, ssize_t i)
        : leaf_nodes(l),
          internal_nodes(i) {
    }

    node_stats& operator+=(const node_stats &x) {
      leaf_nodes += x.leaf_nodes;
      internal_nodes += x.internal_nodes;
      return *this;
    }

    ssize_t leaf_nodes;
    ssize_t internal_nodes;
  };

 public:
  typedef Params params_type;
  typedef typename Params::key_type key_type;
  typedef typename Params::data_type data_type;
  typedef typename Params::mapped_type mapped_type;
  typedef typename Params::value_type value_type;
  typedef typename Params::key_compare key_compare;
  typedef typename Params::pointer pointer;
  typedef typename Params::const_pointer const_pointer;
  typedef typename Params::reference reference;
  typedef typename Params::const_reference const_reference;
  typedef typename Params::size_type size_type;
  typedef typename Params::difference_type difference_type;
  typedef btree_iterator<node_type, reference, pointer> iterator;
  typedef typename iterator::const_iterator const_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;

  typedef typename Params::allocator_type allocator_type;
  typedef typename allocator_type::template rebind<char>::other
    internal_allocator_type;

 public:
  // Default constructor.
  btree(const key_compare &comp, const allocator_type &alloc);

  // Copy constructor.
  btree(const self_type &x);

  // Destructor.
  ~btree() {
    clear();
  }

  // Iterator routines.
  iterator begin() {
    return iterator(leftmost(), 0);
  }
  const_iterator begin() const {
    return const_iterator(leftmost(), 0);
  }
  iterator end() {
    return iterator(rightmost(), rightmost() ? rightmost()->count() : 0);
  }
  const_iterator end() const {
    return const_iterator(rightmost(), rightmost() ? rightmost()->count() : 0);
  }
  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() {
    return reverse_iterator(begin());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  // Finds the first element whose key is not less than key.
  iterator lower_bound(const key_type &key) {
    return internal_end(
        internal_lower_bound(key, iterator(root(), 0)));
  }
  const_iterator lower_bound(const key_type &key) const {
    return internal_end(
        internal_lower_bound(key, const_iterator(root(), 0)));
  }

  // Finds the first element whose key is greater than key.
  iterator upper_bound(const key_type &key) {
    return internal_end(
        internal_upper_bound(key, iterator(root(), 0)));
  }
  const_iterator upper_bound(const key_type &key) const {
    return internal_end(
        internal_upper_bound(key, const_iterator(root(), 0)));
  }

  // Finds the range of values which compare equal to key. The first member of
  // the returned pair is equal to lower_bound(key). The second member pair of
  // the pair is equal to upper_bound(key).
  std::pair<iterator,iterator> equal_range(const key_type &key) {
    return std::make_pair(lower_bound(key), upper_bound(key));
  }
  std::pair<const_iterator,const_iterator> equal_range(const key_type &key) const {
    return std::make_pair(lower_bound(key), upper_bound(key));
  }

  // Inserts a value into the btree only if it does not already exist. The
  // boolean return value indicates whether insertion succeeded or failed. The
  // ValuePointer type is used to avoid instatiating the value unless the key
  // is being inserted. Value is not dereferenced if the key already exists in
  // the btree. See btree_map::operator[].
  template <typename ValuePointer>
  std::pair<iterator,bool> insert_unique(const key_type &key, ValuePointer value);

  // Inserts a value into the btree only if it does not already exist. The
  // boolean return value indicates whether insertion succeeded or failed.
  std::pair<iterator,bool> insert_unique(const value_type &v) {
    return insert_unique(params_type::key(v), &v);
  }

  // Insert with hint. Check to see if the value should be placed immediately
  // before position in the tree. If it does, then the insertion will take
  // amortized constant time. If not, the insertion will take amortized
  // logarithmic time as if a call to insert_unique(v) were made.
  iterator insert_unique(iterator position, const value_type &v);

  // Insert a range of values into the btree.
  template <typename InputIterator>
  void insert_unique(InputIterator b, InputIterator e);

  // Inserts a value into the btree. The ValuePointer type is used to avoid
  // instatiating the value unless the key is being inserted. Value is not
  // dereferenced if the key already exists in the btree. See
  // btree_map::operator[].
  template <typename ValuePointer>
  iterator insert_multi(const key_type &key, ValuePointer value);

  // Inserts a value into the btree.
  iterator insert_multi(const value_type &v) {
    return insert_multi(params_type::key(v), &v);
  }

  // Insert with hint. Check to see if the value should be placed immediately
  // before position in the tree. If it does, then the insertion will take
  // amortized constant time. If not, the insertion will take amortized
  // logarithmic time as if a call to insert_multi(v) were made.
  iterator insert_multi(iterator position, const value_type &v);

  // Insert a range of values into the btree.
  template <typename InputIterator>
  void insert_multi(InputIterator b, InputIterator e);

  void assign(const self_type &x);

  // Erase the specified iterator from the btree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  iterator erase(iterator iter);

  // Erases range. Returns the number of keys erased.
  int erase(iterator begin, iterator end);

  // Erases the specified key from the btree. Returns 1 if an element was
  // erased and 0 otherwise.
  int erase_unique(const key_type &key);

  // Erases all of the entries matching the specified key from the
  // btree. Returns the number of elements erased.
  int erase_multi(const key_type &key);

  // Finds the iterator corresponding to a key or returns end() if the key is
  // not present.
  iterator find_unique(const key_type &key) {
    return internal_end(
        internal_find_unique(key, iterator(root(), 0)));
  }
  const_iterator find_unique(const key_type &key) const {
    return internal_end(
        internal_find_unique(key, const_iterator(root(), 0)));
  }
  iterator find_multi(const key_type &key) {
    return internal_end(
        internal_find_multi(key, iterator(root(), 0)));
  }
  const_iterator find_multi(const key_type &key) const {
    return internal_end(
        internal_find_multi(key, const_iterator(root(), 0)));
  }

  // Returns a count of the number of times the key appears in the btree.
  size_type count_unique(const key_type &key) const {
    const_iterator begin = internal_find_unique(
        key, const_iterator(root(), 0));
    if (!begin.node) {
      // The key doesn't exist in the tree.
      return 0;
    }
    return 1;
  }
  // Returns a count of the number of times the key appears in the btree.
  size_type count_multi(const key_type &key) const {
    return distance(lower_bound(key), upper_bound(key));
  }

  // Clear the btree, deleting all of the values it contains.
  void clear();

  // Swap the contents of *this and x.
  void swap(self_type &x);

  // Assign the contents of x to *this.
  self_type& operator=(const self_type &x) {
    if (&x == this) {
      // Don't copy onto ourselves.
      return *this;
    }
    assign(x);
    return *this;
  }

  key_compare* mutable_key_comp() {
    return this;
  }
  const key_compare& key_comp() const {
    return *this;
  }
  bool compare_keys(const key_type &x, const key_type &y) const {
    return btree_compare_keys(key_comp(), x, y);
  }

  // Dump the btree to the specified ostream. Requires that operator<< is
  // defined for Key and Value.
  void dump(std::ostream &os) const {
    if (root() != NULL) {
      internal_dump(os, root(), 0);
    }
  }

  // Verifies the structure of the btree.
  void verify() const;

  // Size routines. Note that empty() is slightly faster than doing size()==0.
  size_type size() const {
    if (empty()) return 0;
    if (root()->leaf()) return root()->count();
    return root()->size();
  }
  size_type max_size() const { return std::numeric_limits<size_type>::max(); }
  bool empty() const { return root() == NULL; }

  // The height of the btree. An empty tree will have height 0.
  size_type height() const {
    size_type h = 0;
    if (root()) {
      // Count the length of the chain from the leftmost node up to the
      // root. We actually count from the root back around to the level below
      // the root, but the calculation is the same because of the circularity
      // of that traversal.
      const node_type *n = root();
      do {
        ++h;
        n = n->parent();
      } while (n != root());
    }
    return h;
  }

  // The number of internal, leaf and total nodes used by the btree.
  size_type leaf_nodes() const {
    return internal_stats(root()).leaf_nodes;
  }
  size_type internal_nodes() const {
    return internal_stats(root()).internal_nodes;
  }
  size_type nodes() const {
    node_stats stats = internal_stats(root());
    return stats.leaf_nodes + stats.internal_nodes;
  }

  // The total number of bytes used by the btree.
  size_type bytes_used() const {
    node_stats stats = internal_stats(root());
    if (stats.leaf_nodes == 1 && stats.internal_nodes == 0) {
      return sizeof(*this) +
          sizeof(base_fields) + root()->max_count() * sizeof(value_type);
    } else {
      return sizeof(*this) +
          sizeof(root_fields) - sizeof(internal_fields) +
          stats.leaf_nodes * sizeof(leaf_fields) +
          stats.internal_nodes * sizeof(internal_fields);
    }
  }

  // The average number of bytes used per value stored in the btree.
  static double average_bytes_per_value() {
    // Returns the number of bytes per value on a leaf node that is 75%
    // full. Experimentally, this matches up nicely with the computed number of
    // bytes per value in trees that had their values inserted in random order.
    return sizeof(leaf_fields) / (kNodeValues * 0.75);
  }

  // The fullness of the btree. Computed as the number of elements in the btree
  // divided by the maximum number of elements a tree with the current number
  // of nodes could hold. A value of 1 indicates perfect space
  // utilization. Smaller values indicate space wastage.
  double fullness() const {
    return double(size()) / (nodes() * kNodeValues);
  }
  // The overhead of the btree structure in bytes per node. Computed as the
  // total number of bytes used by the btree minus the number of bytes used for
  // storing elements divided by the number of elements.
  double overhead() const {
    if (empty()) {
      return 0.0;
    }
    return (bytes_used() - size() * kValueSize) / double(size());
  }

 private:
  // Internal accessor routines.
  node_type* root() { return root_.data; }
  const node_type* root() const { return root_.data; }
  node_type** mutable_root() { return &root_.data; }

  // The rightmost node is stored in the root node.
  node_type* rightmost() {
    return (!root() || root()->leaf()) ? root() : root()->rightmost();
  }
  const node_type* rightmost() const {
    return (!root() || root()->leaf()) ? root() : root()->rightmost();
  }
  node_type** mutable_rightmost() { return root()->mutable_rightmost(); }

  // The leftmost node is stored as the parent of the root node.
  node_type* leftmost() { return root() ? root()->parent() : NULL; }
  const node_type* leftmost() const { return root() ? root()->parent() : NULL; }

  // The size of the tree is stored in the root node.
  size_type* mutable_size() { return root()->mutable_size(); }

  // Allocator routines.
  internal_allocator_type* mutable_internal_allocator() {
    return static_cast<internal_allocator_type*>(&root_);
  }
  const internal_allocator_type& internal_allocator() const {
    return *static_cast<const internal_allocator_type*>(&root_);
  }

  // Node creation/deletion routines.
  node_type* new_internal_node(node_type *parent) {
    internal_fields *p = reinterpret_cast<internal_fields*>(
        mutable_internal_allocator()->allocate(sizeof(internal_fields)));
    return node_type::init_internal(p, parent);
  }
  node_type* new_internal_root_node() {
    root_fields *p = reinterpret_cast<root_fields*>(
        mutable_internal_allocator()->allocate(sizeof(root_fields)));
    return node_type::init_root(p, root()->parent());
  }
  node_type* new_leaf_node(node_type *parent) {
    leaf_fields *p = reinterpret_cast<leaf_fields*>(
        mutable_internal_allocator()->allocate(sizeof(leaf_fields)));
    return node_type::init_leaf(p, parent, kNodeValues);
  }
  node_type* new_leaf_root_node(int max_count) {
    leaf_fields *p = reinterpret_cast<leaf_fields*>(
        mutable_internal_allocator()->allocate(
            sizeof(base_fields) + max_count * sizeof(value_type)));
    return node_type::init_leaf(p, reinterpret_cast<node_type*>(p), max_count);
  }
  void delete_internal_node(node_type *node) {
    node->destroy();
    assert(node != root());
    mutable_internal_allocator()->deallocate(
        reinterpret_cast<char*>(node), sizeof(internal_fields));
  }
  void delete_internal_root_node() {
    root()->destroy();
    mutable_internal_allocator()->deallocate(
        reinterpret_cast<char*>(root()), sizeof(root_fields));
  }
  void delete_leaf_node(node_type *node) {
    node->destroy();
    mutable_internal_allocator()->deallocate(
        reinterpret_cast<char*>(node),
        sizeof(base_fields) + node->max_count() * sizeof(value_type));
  }

  // Rebalances or splits the node iter points to.
  void rebalance_or_split(iterator *iter);

  // Merges the values of left, right and the delimiting key on their parent
  // onto left, removing the delimiting key and deleting right.
  void merge_nodes(node_type *left, node_type *right);

  // Tries to merge node with its left or right sibling, and failing that,
  // rebalance with its left or right sibling. Returns true if a merge
  // occurred, at which point it is no longer valid to access node. Returns
  // false if no merging took place.
  bool try_merge_or_rebalance(iterator *iter);

  // Tries to shrink the height of the tree by 1.
  void try_shrink();

  iterator internal_end(iterator iter) {
    return iter.node ? iter : end();
  }
  const_iterator internal_end(const_iterator iter) const {
    return iter.node ? iter : end();
  }

  // Inserts a value into the btree immediately before iter. Requires that
  // key(v) <= iter.key() and (--iter).key() <= key(v).
  iterator internal_insert(iterator iter, const value_type &v);

  // Returns an iterator pointing to the first value >= the value "iter" is
  // pointing at. Note that "iter" might be pointing to an invalid location as
  // iter.position == iter.node->count(). This routine simply moves iter up in
  // the tree to a valid location.
  template <typename IterType>
  static IterType internal_last(IterType iter);

  // Returns an iterator pointing to the leaf position at which key would
  // reside in the tree. We provide 2 versions of internal_locate. The first
  // version (internal_locate_plain_compare) always returns 0 for the second
  // field of the pair. The second version (internal_locate_compare_to) is for
  // the key-compare-to specialization and returns either kExactMatch (if the
  // key was found in the tree) or -kExactMatch (if it wasn't) in the second
  // field of the pair. The compare_to specialization allows the caller to
  // avoid a subsequent comparison to determine if an exact match was made,
  // speeding up string keys.
  template <typename IterType>
  std::pair<IterType, int> internal_locate(
      const key_type &key, IterType iter) const;
  template <typename IterType>
  std::pair<IterType, int> internal_locate_plain_compare(
      const key_type &key, IterType iter) const;
  template <typename IterType>
  std::pair<IterType, int> internal_locate_compare_to(
      const key_type &key, IterType iter) const;

  // Internal routine which implements lower_bound().
  template <typename IterType>
  IterType internal_lower_bound(
      const key_type &key, IterType iter) const;

  // Internal routine which implements upper_bound().
  template <typename IterType>
  IterType internal_upper_bound(
      const key_type &key, IterType iter) const;

  // Internal routine which implements find_unique().
  template <typename IterType>
  IterType internal_find_unique(
      const key_type &key, IterType iter) const;

  // Internal routine which implements find_multi().
  template <typename IterType>
  IterType internal_find_multi(
      const key_type &key, IterType iter) const;

  // Deletes a node and all of its children.
  void internal_clear(node_type *node);

  // Dumps a node and all of its children to the specified ostream.
  void internal_dump(std::ostream &os, const node_type *node, int level) const;

  // Verifies the tree structure of node.
  int internal_verify(const node_type *node,
                      const key_type *lo, const key_type *hi) const;

  node_stats internal_stats(const node_type *node) const {
    if (!node) {
      return node_stats(0, 0);
    }
    if (node->leaf()) {
      return node_stats(1, 0);
    }
    node_stats res(0, 1);
    for (int i = 0; i <= node->count(); ++i) {
      res += internal_stats(node->child(i));
    }
    return res;
  }

 private:
  empty_base_handle<internal_allocator_type, node_type*> root_;

 private:
  // Types small_ and big_ are promise that sizeof(small_) < sizeof(big_)
  typedef char small_;

  struct big_ {
    char dummy[2];
  };

  // A never instantiated helper function that returns big_ if we have a
  // key-compare-to functor or if R is bool and small_ otherwise.
  template <typename R>
  static typename std::conditional<
   std::conditional<is_key_compare_to::value,
             std::is_same<R, int>,
             std::is_same<R, bool> >::type::value,
   big_, small_>::type key_compare_checker(R);

  // A never instantiated helper function that returns the key comparison
  // functor.
  static key_compare key_compare_helper();

  // Verify that key_compare returns a bool. This is similar to the way
  // is_convertible in base/type_traits.h works. Note that key_compare_checker
  // is never actually invoked. The compiler will select which
  // key_compare_checker() to instantiate and then figure out the size of the
  // return type of key_compare_checker() at compile time which we then check
  // against the sizeof of big_.
  static_assert(
      sizeof(key_compare_checker(key_compare_helper()(key_type(), key_type()))) ==
      sizeof(big_),
      "key comparison function must return bool");

  // Note: We insist on kTargetValues, which is computed from
  // Params::kTargetNodeSize, must fit the base_fields::field_type.
  static_assert(kNodeValues <
                 (1 << (8 * sizeof(typename base_fields::field_type))),
                 "target node size too large");

  // Test the assumption made in setting kNodeValueSpace.
  static_assert(sizeof(base_fields) >= 2 * sizeof(void*),
                 "node space assumption incorrect" );
};


////
// btree methods
template <typename P>
btree<P>::btree(const key_compare &comp, const allocator_type &alloc)
    : key_compare(comp),
      root_(alloc, NULL) {
}

template <typename P>
btree<P>::btree(const self_type &x)
    : key_compare(x.key_comp()),
      root_(x.internal_allocator(), NULL) {
  assign(x);
}

template <typename P> template <typename ValuePointer>
std::pair<typename btree<P>::iterator, bool>
btree<P>::insert_unique(const key_type &key, ValuePointer value) {
  if (empty()) {
    *mutable_root() = new_leaf_root_node(1);
  }

  std::pair<iterator, int> res = internal_locate(key, iterator(root(), 0));
  iterator &iter = res.first;
  if (res.second == kExactMatch) {
    // The key already exists in the tree, do nothing.
    return std::make_pair(internal_last(iter), false);
  } else if (!res.second) {
    iterator last = internal_last(iter);
    if (last.node && !compare_keys(key, last.key())) {
      // The key already exists in the tree, do nothing.
      return std::make_pair(last, false);
    }
  }

  return std::make_pair(internal_insert(iter, *value), true);
}

template <typename P>
inline typename btree<P>::iterator
btree<P>::insert_unique(iterator position, const value_type &v) {
  if (!empty()) {
    const key_type &key = params_type::key(v);
    if (position == end() || compare_keys(key, position.key())) {
      iterator prev = position;
      if (position == begin() || compare_keys((--prev).key(), key)) {
        // prev.key() < key < position.key()
        return internal_insert(position, v);
      }
    } else if (compare_keys(position.key(), key)) {
      iterator next = position;
      ++next;
      if (next == end() || compare_keys(key, next.key())) {
        // position.key() < key < next.key()
        return internal_insert(next, v);
      }
    } else {
      // position.key() == key
      return position;
    }
  }
  return insert_unique(v).first;
}

template <typename P> template <typename InputIterator>
void btree<P>::insert_unique(InputIterator b, InputIterator e) {
  for (; b != e; ++b) {
    insert_unique(end(), *b);
  }
}

template <typename P> template <typename ValuePointer>
typename btree<P>::iterator
btree<P>::insert_multi(const key_type &key, ValuePointer value) {
  if (empty()) {
    *mutable_root() = new_leaf_root_node(1);
  }

  iterator iter = internal_upper_bound(key, iterator(root(), 0));
  if (!iter.node) {
    iter = end();
  }
  return internal_insert(iter, *value);
}

template <typename P>
typename btree<P>::iterator
btree<P>::insert_multi(iterator position, const value_type &v) {
  if (!empty()) {
    const key_type &key = params_type::key(v);
    if (position == end() || !compare_keys(position.key(), key)) {
      iterator prev = position;
      if (position == begin() || !compare_keys(key, (--prev).key())) {
        // prev.key() <= key <= position.key()
        return internal_insert(position, v);
      }
    } else {
      iterator next = position;
      ++next;
      if (next == end() || !compare_keys(next.key(), key)) {
        // position.key() < key <= next.key()
        return internal_insert(next, v);
      }
    }
  }
  return insert_multi(v);
}

template <typename P> template <typename InputIterator>
void btree<P>::insert_multi(InputIterator b, InputIterator e) {
  for (; b != e; ++b) {
    insert_multi(end(), *b);
  }
}

template <typename P>
void btree<P>::assign(const self_type &x) {
  clear();

  *mutable_key_comp() = x.key_comp();
  *mutable_internal_allocator() = x.internal_allocator();

  // Assignment can avoid key comparisons because we know the order of the
  // values is the same order we'll store them in.
  for (const_iterator iter = x.begin(); iter != x.end(); ++iter) {
    if (empty()) {
      insert_multi(*iter);
    } else {
      // If the btree is not empty, we can just insert the new value at the end
      // of the tree!
      internal_insert(end(), *iter);
    }
  }
}

template <typename P>
typename btree<P>::iterator btree<P>::erase(iterator iter) {
  bool internal_delete = false;
  if (!iter.node->leaf()) {
    // Deletion of a value on an internal node. Swap the key with the largest
    // value of our left child. This is easy, we just decrement iter.
    iterator tmp_iter(iter--);
    assert(iter.node->leaf());
    assert(!compare_keys(tmp_iter.key(), iter.key()));
    iter.node->value_swap(iter.position, tmp_iter.node, tmp_iter.position);
    internal_delete = true;
    --*mutable_size();
  } else if (!root()->leaf()) {
    --*mutable_size();
  }

  // Delete the key from the leaf.
  iter.node->remove_value(iter.position);

  // We want to return the next value after the one we just erased. If we
  // erased from an internal node (internal_delete == true), then the next
  // value is ++(++iter). If we erased from a leaf node (internal_delete ==
  // false) then the next value is ++iter. Note that ++iter may point to an
  // internal node and the value in the internal node may move to a leaf node
  // (iter.node) when rebalancing is performed at the leaf level.

  // Merge/rebalance as we walk back up the tree.
  iterator res(iter);
  for (;;) {
    if (iter.node == root()) {
      try_shrink();
      if (empty()) {
        return end();
      }
      break;
    }
    if (iter.node->count() >= kMinNodeValues) {
      break;
    }
    bool merged = try_merge_or_rebalance(&iter);
    if (iter.node->leaf()) {
      res = iter;
    }
    if (!merged) {
      break;
    }
    iter.node = iter.node->parent();
  }

  // Adjust our return value. If we're pointing at the end of a node, advance
  // the iterator.
  if (res.position == res.node->count()) {
    res.position = res.node->count() - 1;
    ++res;
  }
  // If we erased from an internal node, advance the iterator.
  if (internal_delete) {
    ++res;
  }
  return res;
}

template <typename P>
int btree<P>::erase(iterator begin, iterator end) {
  int count = distance(begin, end);
  for (int i = 0; i < count; i++) {
    begin = erase(begin);
  }
  return count;
}

template <typename P>
int btree<P>::erase_unique(const key_type &key) {
  iterator iter = internal_find_unique(key, iterator(root(), 0));
  if (!iter.node) {
    // The key doesn't exist in the tree, return nothing done.
    return 0;
  }
  erase(iter);
  return 1;
}

template <typename P>
int btree<P>::erase_multi(const key_type &key) {
  iterator begin = internal_lower_bound(key, iterator(root(), 0));
  if (!begin.node) {
    // The key doesn't exist in the tree, return nothing done.
    return 0;
  }
  // Delete all of the keys between begin and upper_bound(key).
  iterator end = internal_end(
      internal_upper_bound(key, iterator(root(), 0)));
  return erase(begin, end);
}

template <typename P>
void btree<P>::clear() {
  if (root() != NULL) {
    internal_clear(root());
  }
  *mutable_root() = NULL;
}

template <typename P>
void btree<P>::swap(self_type &x) {
  std::swap(static_cast<key_compare&>(*this), static_cast<key_compare&>(x));
  std::swap(root_, x.root_);
}

template <typename P>
void btree<P>::verify() const {
  if (root() != NULL) {
    assert(size() == internal_verify(root(), NULL, NULL));
    assert(leftmost() == (++const_iterator(root(), -1)).node);
    assert(rightmost() == (--const_iterator(root(), root()->count())).node);
    assert(leftmost()->leaf());
    assert(rightmost()->leaf());
  } else {
    assert(size() == 0);
    assert(leftmost() == NULL);
    assert(rightmost() == NULL);
  }
}

template <typename P>
void btree<P>::rebalance_or_split(iterator *iter) {
  node_type *&node = iter->node;
  int &insert_position = iter->position;
  assert(node->count() == node->max_count());

  // First try to make room on the node by rebalancing.
  node_type *parent = node->parent();
  if (node != root()) {
    if (node->position() > 0) {
      // Try rebalancing with our left sibling.
      node_type *left = parent->child(node->position() - 1);
      if (left->count() < left->max_count()) {
        // We bias rebalancing based on the position being inserted. If we're
        // inserting at the end of the right node then we bias rebalancing to
        // fill up the left node.
        int to_move = (left->max_count() - left->count()) /
            (1 + (insert_position < left->max_count()));
        to_move = std::max(1, to_move);

        if (((insert_position - to_move) >= 0) ||
            ((left->count() + to_move) < left->max_count())) {
          left->rebalance_right_to_left(node, to_move);

          assert(node->max_count() - node->count() == to_move);
          insert_position = insert_position - to_move;
          if (insert_position < 0) {
            insert_position = insert_position + left->count() + 1;
            node = left;
          }

          assert(node->count() < node->max_count());
          return;
        }
      }
    }

    if (node->position() < parent->count()) {
      // Try rebalancing with our right sibling.
      node_type *right = parent->child(node->position() + 1);
      if (right->count() < right->max_count()) {
        // We bias rebalancing based on the position being inserted. If we're
        // inserting at the beginning of the left node then we bias rebalancing
        // to fill up the right node.
        int to_move = (right->max_count() - right->count()) /
            (1 + (insert_position > 0));
        to_move = std::max(1, to_move);

        if ((insert_position <= (node->count() - to_move)) ||
            ((right->count() + to_move) < right->max_count())) {
          node->rebalance_left_to_right(right, to_move);

          if (insert_position > node->count()) {
            insert_position = insert_position - node->count() - 1;
            node = right;
          }

          assert(node->count() < node->max_count());
          return;
        }
      }
    }

    // Rebalancing failed, make sure there is room on the parent node for a new
    // value.
    if (parent->count() == parent->max_count()) {
      iterator parent_iter(node->parent(), node->position());
      rebalance_or_split(&parent_iter);
    }
  } else {
    // Rebalancing not possible because this is the root node.
    if (root()->leaf()) {
      // The root node is currently a leaf node: create a new root node and set
      // the current root node as the child of the new root.
      parent = new_internal_root_node();
      parent->set_child(0, root());
      *mutable_root() = parent;
      assert(*mutable_rightmost() == parent->child(0));
    } else {
      // The root node is an internal node. We do not want to create a new root
      // node because the root node is special and holds the size of the tree
      // and a pointer to the rightmost node. So we create a new internal node
      // and move all of the items on the current root into the new node.
      parent = new_internal_node(parent);
      parent->set_child(0, parent);
      parent->swap(root());
      node = parent;
    }
  }

  // Split the node.
  node_type *split_node;
  if (node->leaf()) {
    split_node = new_leaf_node(parent);
    node->split(split_node, insert_position);
    if (rightmost() == node) {
      *mutable_rightmost() = split_node;
    }
  } else {
    split_node = new_internal_node(parent);
    node->split(split_node, insert_position);
  }

  if (insert_position > node->count()) {
    insert_position = insert_position - node->count() - 1;
    node = split_node;
  }
}

template <typename P>
void btree<P>::merge_nodes(node_type *left, node_type *right) {
  left->merge(right);
  if (right->leaf()) {
    if (rightmost() == right) {
      *mutable_rightmost() = left;
    }
    delete_leaf_node(right);
  } else {
    delete_internal_node(right);
  }
}

template <typename P>
bool btree<P>::try_merge_or_rebalance(iterator *iter) {
  node_type *parent = iter->node->parent();
  if (iter->node->position() > 0) {
    // Try merging with our left sibling.
    node_type *left = parent->child(iter->node->position() - 1);
    if ((1 + left->count() + iter->node->count()) <= left->max_count()) {
      iter->position += 1 + left->count();
      merge_nodes(left, iter->node);
      iter->node = left;
      return true;
    }
  }
  if (iter->node->position() < parent->count()) {
    // Try merging with our right sibling.
    node_type *right = parent->child(iter->node->position() + 1);
    if ((1 + iter->node->count() + right->count()) <= right->max_count()) {
      merge_nodes(iter->node, right);
      return true;
    }
    // Try rebalancing with our right sibling. We don't perform rebalancing if
    // we deleted the first element from iter->node and the node is not
    // empty. This is a small optimization for the common pattern of deleting
    // from the front of the tree.
    if ((right->count() > kMinNodeValues) &&
        ((iter->node->count() == 0) ||
         (iter->position > 0))) {
      int to_move = (right->count() - iter->node->count()) / 2;
      to_move = std::min(to_move, right->count() - 1);
      iter->node->rebalance_right_to_left(right, to_move);
      return false;
    }
  }
  if (iter->node->position() > 0) {
    // Try rebalancing with our left sibling. We don't perform rebalancing if
    // we deleted the last element from iter->node and the node is not
    // empty. This is a small optimization for the common pattern of deleting
    // from the back of the tree.
    node_type *left = parent->child(iter->node->position() - 1);
    if ((left->count() > kMinNodeValues) &&
        ((iter->node->count() == 0) ||
         (iter->position < iter->node->count()))) {
      int to_move = (left->count() - iter->node->count()) / 2;
      to_move = std::min(to_move, left->count() - 1);
      left->rebalance_left_to_right(iter->node, to_move);
      iter->position += to_move;
      return false;
    }
  }
  return false;
}

template <typename P>
void btree<P>::try_shrink() {
  if (root()->count() > 0) {
    return;
  }
  // Deleted the last item on the root node, shrink the height of the tree.
  if (root()->leaf()) {
    assert(size() == 0);
    delete_leaf_node(root());
    *mutable_root() = NULL;
  } else {
    node_type *child = root()->child(0);
    if (child->leaf()) {
      // The child is a leaf node so simply make it the root node in the tree.
      child->make_root();
      delete_internal_root_node();
      *mutable_root() = child;
    } else {
      // The child is an internal node. We want to keep the existing root node
      // so we move all of the values from the child node into the existing
      // (empty) root node.
      child->swap(root());
      delete_internal_node(child);
    }
  }
}

template <typename P> template <typename IterType>
inline IterType btree<P>::internal_last(IterType iter) {
  while (iter.node && iter.position == iter.node->count()) {
    iter.position = iter.node->position();
    iter.node = iter.node->parent();
    if (iter.node->leaf()) {
      iter.node = NULL;
    }
  }
  return iter;
}

template <typename P>
inline typename btree<P>::iterator
btree<P>::internal_insert(iterator iter, const value_type &v) {
  if (!iter.node->leaf()) {
    // We can't insert on an internal node. Instead, we'll insert after the
    // previous value which is guaranteed to be on a leaf node.
    --iter;
    ++iter.position;
  }
  if (iter.node->count() == iter.node->max_count()) {
    // Make room in the leaf for the new item.
    if (iter.node->max_count() < kNodeValues) {
      // Insertion into the root where the root is smaller that the full node
      // size. Simply grow the size of the root node.
      assert(iter.node == root());
      iter.node = new_leaf_root_node(
          std::min<int>(kNodeValues, 2 * iter.node->max_count()));
      iter.node->swap(root());
      delete_leaf_node(root());
      *mutable_root() = iter.node;
    } else {
      rebalance_or_split(&iter);
      ++*mutable_size();
    }
  } else if (!root()->leaf()) {
    ++*mutable_size();
  }
  iter.node->insert_value(iter.position, v);
  return iter;
}

template <typename P> template <typename IterType>
inline std::pair<IterType, int> btree<P>::internal_locate(
    const key_type &key, IterType iter) const {
  return internal_locate_type::dispatch(key, *this, iter);
}

template <typename P> template <typename IterType>
inline std::pair<IterType, int> btree<P>::internal_locate_plain_compare(
    const key_type &key, IterType iter) const {
  for (;;) {
    iter.position = iter.node->lower_bound(key, key_comp());
    if (iter.node->leaf()) {
      break;
    }
    iter.node = iter.node->child(iter.position);
  }
  return std::make_pair(iter, 0);
}

template <typename P> template <typename IterType>
inline std::pair<IterType, int> btree<P>::internal_locate_compare_to(
    const key_type &key, IterType iter) const {
  for (;;) {
    int res = iter.node->lower_bound(key, key_comp());
    iter.position = res & kMatchMask;
    if (res & kExactMatch) {
      return std::make_pair(iter, static_cast<int>(kExactMatch));
    }
    if (iter.node->leaf()) {
      break;
    }
    iter.node = iter.node->child(iter.position);
  }
  return std::make_pair(iter, -kExactMatch);
}

template <typename P> template <typename IterType>
IterType btree<P>::internal_lower_bound(
    const key_type &key, IterType iter) const {
  if (iter.node) {
    for (;;) {
      iter.position =
          iter.node->lower_bound(key, key_comp()) & kMatchMask;
      if (iter.node->leaf()) {
        break;
      }
      iter.node = iter.node->child(iter.position);
    }
    iter = internal_last(iter);
  }
  return iter;
}

template <typename P> template <typename IterType>
IterType btree<P>::internal_upper_bound(
    const key_type &key, IterType iter) const {
  if (iter.node) {
    for (;;) {
      iter.position = iter.node->upper_bound(key, key_comp());
      if (iter.node->leaf()) {
        break;
      }
      iter.node = iter.node->child(iter.position);
    }
    iter = internal_last(iter);
  }
  return iter;
}

template <typename P> template <typename IterType>
IterType btree<P>::internal_find_unique(
    const key_type &key, IterType iter) const {
  if (iter.node) {
    std::pair<IterType, int> res = internal_locate(key, iter);
    if (res.second == kExactMatch) {
      return res.first;
    }
    if (!res.second) {
      iter = internal_last(res.first);
      if (iter.node && !compare_keys(key, iter.key())) {
        return iter;
      }
    }
  }
  return IterType(NULL, 0);
}

template <typename P> template <typename IterType>
IterType btree<P>::internal_find_multi(
    const key_type &key, IterType iter) const {
  if (iter.node) {
    iter = internal_lower_bound(key, iter);
    if (iter.node) {
      iter = internal_last(iter);
      if (iter.node && !compare_keys(key, iter.key())) {
        return iter;
      }
    }
  }
  return IterType(NULL, 0);
}

template <typename P>
void btree<P>::internal_clear(node_type *node) {
  if (!node->leaf()) {
    for (int i = 0; i <= node->count(); ++i) {
      internal_clear(node->child(i));
    }
    if (node == root()) {
      delete_internal_root_node();
    } else {
      delete_internal_node(node);
    }
  } else {
    delete_leaf_node(node);
  }
}

template <typename P>
void btree<P>::internal_dump(
    std::ostream &os, const node_type *node, int level) const {
  for (int i = 0; i < node->count(); ++i) {
    if (!node->leaf()) {
      internal_dump(os, node->child(i), level + 1);
    }
    for (int j = 0; j < level; ++j) {
      os << "  ";
    }
    os << node->key(i) << " [" << level << "]\n";
  }
  if (!node->leaf()) {
    internal_dump(os, node->child(node->count()), level + 1);
  }
}

template <typename P>
int btree<P>::internal_verify(
    const node_type *node, const key_type *lo, const key_type *hi) const {
  assert(node->count() > 0);
  assert(node->count() <= node->max_count());
  if (lo) {
    assert(!compare_keys(node->key(0), *lo));
  }
  if (hi) {
    assert(!compare_keys(*hi, node->key(node->count() - 1)));
  }
  for (int i = 1; i < node->count(); ++i) {
    assert(!compare_keys(node->key(i), node->key(i - 1)));
  }
  int count = node->count();
  if (!node->leaf()) {
    for (int i = 0; i <= node->count(); ++i) {
      assert(node->child(i) != NULL);
      assert(node->child(i)->parent() == node);
      assert(node->child(i)->position() == i);
      count += internal_verify(
          node->child(i),
          (i == 0) ? lo : &node->key(i - 1),
          (i == node->count()) ? hi : &node->key(i));
    }
  }
  return count;
}



}

#endif
