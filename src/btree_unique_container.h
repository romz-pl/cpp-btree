#ifndef ROMZ_CPP_BTREE_BTREE_UNIQUE_CONTAINER_H
#define ROMZ_CPP_BTREE_BTREE_UNIQUE_CONTAINER_H


#include "btree_container.h"

namespace btree
{
// A common base class for btree_set and safe_btree_set.
template <typename Tree>
class btree_unique_container : public btree_container<Tree> {
  typedef btree_unique_container<Tree> self_type;
  typedef btree_container<Tree> super_type;

 public:
  typedef typename Tree::key_type key_type;
  typedef typename Tree::value_type value_type;
  typedef typename Tree::size_type size_type;
  typedef typename Tree::key_compare key_compare;
  typedef typename Tree::allocator_type allocator_type;
  typedef typename Tree::iterator iterator;
  typedef typename Tree::const_iterator const_iterator;

 public:
  // Default constructor.
  btree_unique_container(const key_compare &comp = key_compare(),
                         const allocator_type &alloc = allocator_type())
      : super_type(comp, alloc) {
  }

  // Copy constructor.
  btree_unique_container(const self_type &x)
      : super_type(x) {
  }

  // Range constructor.
  template <class InputIterator>
  btree_unique_container(InputIterator b, InputIterator e,
                         const key_compare &comp = key_compare(),
                         const allocator_type &alloc = allocator_type())
      : super_type(comp, alloc) {
    insert(b, e);
  }

  // Lookup routines.
  iterator find(const key_type &key) {
    return this->tree_.find_unique(key);
  }
  const_iterator find(const key_type &key) const {
    return this->tree_.find_unique(key);
  }
  size_type count(const key_type &key) const {
    return this->tree_.count_unique(key);
  }

  // Insertion routines.
  std::pair<iterator,bool> insert(const value_type &x) {
    return this->tree_.insert_unique(x);
  }
  iterator insert(iterator position, const value_type &x) {
    return this->tree_.insert_unique(position, x);
  }
  template <typename InputIterator>
  void insert(InputIterator b, InputIterator e) {
    this->tree_.insert_unique(b, e);
  }

  // Deletion routines.
  int erase(const key_type &key) {
    return this->tree_.erase_unique(key);
  }
  // Erase the specified iterator from the btree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  iterator erase(const iterator &iter) {
    return this->tree_.erase(iter);
  }
  void erase(const iterator &first, const iterator &last) {
    this->tree_.erase(first, last);
  }
};

}

#endif
