#include "Base_Filter.h"

auto sperr::Base_Filter::apply_filter(vecd_type& buf, dims_type dims) -> vec8_type
{
  auto empty = vec8_type();
  return empty;
}

auto sperr::Base_Filter::inverse_filter(vecd_type& buf, dims_type dims, const void* header) -> RTNType
{
  return RTNType::Good;
}

auto sperr::Base_Filter::header_size(const void* header) const -> size_t
{
  return 0;
}
