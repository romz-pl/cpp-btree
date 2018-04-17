#ifndef ROMZ_CPP_BTREE_BTREE_MULTIMAP_H
#define ROMZ_CPP_BTREE_BTREE_MULTIMAP_H


#include "btree_multi_container.h"

namespace btree
{
// The btree_multimap class is needed mainly for its constructors.
template <typename Key, typename Value,
          typename Compare = std::less<Key>,
          typename Alloc = std::allocator<std::pair<const Key, Value> >,
          int TargetNodeSize = 256>
class btree_multimap : public btree_multi_container<
  btree<btree_map_params<Key, Value, Compare, Alloc, TargetNodeSize> > > {

  typedef btree_multimap<Key, Value, Compare, Alloc, TargetNodeSize> self_type;
  typedef btree_map_params<
    Key, Value, Compare, Alloc, TargetNodeSize> params_type;
  typedef btree<params_type> btree_type;
  typedef btree_multi_container<btree_type> super_type;

 public:
  typedef typename btree_type::key_compare key_compare;
  typedef typename btree_type::allocator_type allocator_type;
  typedef typename btree_type::data_type data_type;
  typedef typename btree_type::mapped_type mapped_type;

 public:
  // Default constructor.
  btree_multimap(const key_compare &comp = key_compare(),
                 const allocator_type &alloc = allocator_type())
      : super_type(comp, alloc) {
  }

  // Copy constructor.
  btree_multimap(const self_type &x)
      : super_type(x) {
  }

  // Range constructor.
  template <class InputIterator>
  btree_multimap(InputIterator b, InputIterator e,
                 const key_compare &comp = key_compare(),
                 const allocator_type &alloc = allocator_type())
      : super_type(b, e, comp, alloc) {
  }
};

template <typename K, typename V, typename C, typename A, int N>
inline void swap(btree_multimap<K, V, C, A, N> &x,
                 btree_multimap<K, V, C, A, N> &y) {
  x.swap(y);
}

}

#endif
