#include "Matthias_Filter.h"

auto sperr::Matthias_Filter::apply_filter(vecd_type& buf, dims_type dims) -> vec8_type
{
  auto empty = vec8_type();
  return empty;
}

auto sperr::Matthias_Filter::inverse_filter(vecd_type& buf, dims_type dims, const void* header) -> RTNType
{
  return RTNType::Good;
}

auto sperr::Matthias_Filter::header_size(const void* header) const -> size_t
{
  return 0;
}
