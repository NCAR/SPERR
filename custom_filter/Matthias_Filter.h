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
  virtual auto inverse_filter(vecd_type&, dims_type, const void* header, size_t header_len)
      -> bool override;
  virtual auto header_size(const void* header) const -> size_t override;

 private:
  vecd_type m_slice_buf;

  void m_extract_YZ_slice(vecd_type& buf, dims_type dims, size_t x);
  void m_restore_YZ_slice(vecd_type& buf, dims_type dims, size_t x);
  auto m_calc_RMS() const -> double;  // RMS of all values in `m_slice_buf`
};

};  // namespace sperr

#endif
