#ifndef MATTHIAS_FILTER_H
#define MATTHIAS_FILTER_H

//
// Custom filter for Matthias Rempel.
// It performs the following two operations on a per-slice basis:
// 1) subtract mean of the slice;
// 2) divide by RMS of the slice.
// Note that a slice is on the Y-Z plane.
//

#include "Base_Filter.h"

namespace sperr {

class Matthias_Filter : Base_Filter {
 public:
  virtual ~Matthias_Filter() = default;

  // Action items
  virtual auto apply_filter(vecd_type& buf, dims_type dims) -> vec8_type override;
  virtual auto inverse_filter(vecd_type&, dims_type, const void* header) -> RTNType override;
  virtual auto header_size(const void* header) const -> size_t override;

 private:
  vecd_type m_tmp_buf;
};

}; // namespace sperr

#endif

