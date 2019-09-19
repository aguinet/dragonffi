#ifndef DFFI_ITERATORS_H
#define DFFI_ITERATORS_H

#include <dffi/iterator.h>

namespace dffi {

// Adapted from LLVM's STLExtras.h

// mapped_iterator - This is a simple iterator adapter that causes a function to
// be applied whenever operator* is invoked on the iterator.

template <typename ItTy, typename FuncTy,
          typename FuncReturnTy =
            decltype(std::declval<FuncTy>()(*std::declval<ItTy>()))>
class mapped_iterator
    : public iterator_adaptor_base<
             mapped_iterator<ItTy, FuncTy>, ItTy,
             typename std::iterator_traits<ItTy>::iterator_category,
             typename std::remove_reference<FuncReturnTy>::type> {
public:
  mapped_iterator(ItTy U, FuncTy F)
    : mapped_iterator::iterator_adaptor_base(std::move(U)), F(std::move(F)) {}

  ItTy getCurrent() { return this->I; }

  FuncReturnTy operator*() { return F(*this->I); }

private:
  FuncTy F;
};

// map_iterator - Provide a convenient way to create mapped_iterators, just like
// make_pair is useful for creating pairs...
template <class ItTy, class FuncTy>
inline mapped_iterator<ItTy, FuncTy> map_iterator(ItTy I, FuncTy F) { 
  return mapped_iterator<ItTy, FuncTy>(std::move(I), std::move(F));
}

} // dffi

#endif
