#ifndef DFFI_MDARRAY_H
#define DFFI_MDARRAY_H

#include <vector>
#include <dffi/exports.h>
#include <dffi/types.h>
#include <dffi/compat.h>

namespace dffi {

struct DFFI_API MDArray
{
  using ShapeTy = std::vector<ssize_t>;

  MDArray(ShapeTy Shape, QualType BTy);
  MDArray();

  MDArray(MDArray const&);
  MDArray(MDArray&&);

  static MDArray fromArrayTy(ArrayType const& ATy);

  inline bool isValid() const { return Shape_.size() > 0 && BTy_; }
  inline QualType getElementType() const { return BTy_; }
  inline ShapeTy const& getShape() const { return Shape_; }

private:
  ShapeTy Shape_;
  QualType BTy_;
};

} // dffi

#endif
