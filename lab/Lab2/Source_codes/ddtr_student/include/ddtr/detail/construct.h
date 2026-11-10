#ifndef DDT_UTIL_HPP
#define DDT_UTIL_HPP
#include <new>

template <typename Tp>
inline void Constructor(Tp * p) {
  ::new(static_cast<void*>(p)) Tp();
}

template <typename Tp, typename Tval>
inline void Constructor(Tp * p, const Tval& value) {
  ::new(static_cast<void*>(p)) Tp(value);
}

template <typename Tp>
inline void Destructor(Tp * p) {
  p->~Tp();
}

template <typename Iter>
inline void Destructor(Iter pstart, Iter pend) {
  while (pstart != pend) {
    Destructor(&*pstart);
    ++pstart;
  }
}

#endif
