## Custom Filter Support In SPERR

SPERR supports a custom filter to be placed in its data processing pipeline. 
This filter is invoked before any SPERR-defined data compression steps (e.g., wavelet
transform, outlier correction), and is also invoked as the last step of data *decompression*.
As an example, [Matthias_Filter.h]((https://github.com/NCAR/SPERR/blob/main/docker/Dockerfile)) 
here performs two operation: 1) calculate mean of a slice
and subtract mean for every value of that slice; and 2) calculate RMS of a slice and divide
every value by the RMS.

## Implement A Custom Filter

Implement a custom filter requires creating a custom filter class inherited from a
[Base_Filter](https://github.com/NCAR/SPERR/blob/main/docker/Dockerfile), and also 
implementing three mandatory functions:
```C++
virtual auto apply_filter(vecd_type& buf, dims_type dims) -> vec8_type;
virtual auto inverse_filter(vecd_type&, dims_type, const void* header, size_t header_len) -> bool;
virtual auto header_size(const void* header) const -> size_t
```
The custom filter is free to implement any other non-mandatory functions.


## Use A Custom Filter
