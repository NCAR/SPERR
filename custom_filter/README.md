## Custom Filter Support In SPERR

SPERR supports a custom filter to be placed in its data processing pipeline. 
This filter is invoked before any SPERR-defined data compression steps (e.g., wavelet
transform, outlier correction), and is also invoked as the last step of data *decompression*.
As an example, [Matthias_Filter.h](https://github.com/NCAR/SPERR/blob/main/custom_filter/Matthias_Filter.h) 
here performs two operation: 1) calculate mean of a slice
and subtract mean for every value of that slice; and 2) calculate RMS of a slice and divide
every value by the RMS.


## Use A Custom Filter

To use a custom filter, one needs to bring the filter implementation to the SPERR compilation.
Once SPERR is compiled with a filter, SPERR will always include operations defined by the filter in its data processing
pipeline. In other words, once compiled, the filter is always on; there is no API to turn it on or off.

There are three steps to bring a filter implementation to the SPERR compilation:
(use `Matthias_Filter` here as an example)

1. Copy the implementation files to their respective directories. For example, copy the header, `Matthias_Filter.h`,
   to the [`include`](https://github.com/NCAR/SPERR/tree/main/include) directory, and 
   `Matthias_Filter.cpp` to the [`src`](https://github.com/NCAR/SPERR/tree/main/src) directory.
2. Include the implementation files in [`src/CMakeLists.txt`](https://github.com/NCAR/SPERR/blob/main/src/CMakeLists.txt). 
   For example, uncomment the following two lines by removing the leading pound signs:
   ```cmake
   # Matthias_Filter.cpp
   # include/Matthias_Filter.h;\
   ```
3. Specify to use the custom filter in [`include/Conditioner.h`](https://github.com/NCAR/SPERR/blob/main/include/Conditioner.h). 
   For example, include the `Matthias_Filter` header, and change the filter declaration to be `Matthias_Filter`:
   ```C++
   #include "Matthias_Filter.h"  // Add this line.
   Matthias_Filter m_filter;     // This line was previously declaring a Base_Filter.
   ```
At this point, one can reconfigure and rebuild SPERR, which will include the custom filter `Matthias_Filter`.

## Implement A Custom Filter

Implement a custom filter requires creating a custom filter class inherited from a
[Base_Filter](https://github.com/NCAR/SPERR/blob/main/include/Base_Filter.h), and also 
implementing three mandatory functions:
```C++
virtual auto apply_filter(vecd_type& buf, dims_type dims) -> vec8_type;
virtual auto inverse_filter(vecd_type&, dims_type, const void* header, size_t header_len) -> bool;
virtual auto header_size(const void* header) const -> size_t
```
The custom filter is free to implement any other non-mandatory functions. 
Again, `Matthias_Filter` in this directory is a good example. 
