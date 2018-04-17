#ifndef ROMZ_CPP_BTREE_BTREE_MULTISET_H
#define ROMZ_CPP_BTREE_BTREE_MULTISET_H


#include "btree_multi_container.h"

namespace btree
{
// The btree_multiset class is needed mainly for its constructors.
template <typename Key,
          typename Compare = std::less<Key>,
          typename Alloc = std::allocator<Key>,
          int TargetNodeSize = 256>
class btree_multiset : public btree_multi_container<
  btree<btree_set_params<Key, Compare, Alloc, TargetNodeSize> > > {

  typedef btree_multiset<Key, Compare, Alloc, TargetNodeSize> self_type;
  typedef btree_set_params<Key, Compare, Alloc, TargetNodeSize> params_type;
  typedef btree<params_type> btree_type;
  typedef btree_multi_container<btree_type> super_type;

 public:
  typedef typename btree_type::key_compare key_compare;
  typedef typename btree_type::allocator_type allocator_type;

 public:
  // Default constructor.
  btree_multiset(const key_compare &comp = key_compare(),
                 const allocator_type &alloc = allocator_type())
      : super_type(comp, alloc) {
  }

  // Copy constructor.
  btree_multiset(const self_type &x)
      : super_type(x) {
  }

  // Range constructor.
  template <class InputIterator>
  btree_multiset(InputIterator b, InputIterator e,
                 const key_compare &comp = key_compare(),
                 const allocator_type &alloc = allocator_type())
      : super_type(b, e, comp, alloc) {
  }
};

template <typename K, typename C, typename A, int N>
inline void swap(btree_multiset<K, C, A, N> &x,
                 btree_multiset<K, C, A, N> &y) {
  x.swap(y);
}

}

#endif
