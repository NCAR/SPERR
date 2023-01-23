#ifndef BASE_FILTER_H
#define BASE_FILTER_H

//
// This is the base class for all custom filters, which does not perform any meaningful task.
//

#include "sperr_helper.h"

namespace sperr {

class Base_Filter{
 public:
  virtual ~Base_Filter() = default;

  // Action items
  virtual auto apply_filter(vecd_type& buf) -> vec8_type;
  virtual auto inverse_filter(vecd_type& buf, const void* header) -> RTNType;
  virtual auto header_size(const void* header) const -> size_t;

};

}; // namespace sperr

#endif
