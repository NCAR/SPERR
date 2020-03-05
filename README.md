# SPECK2020
Attempt in 2019-2020 to port SPECK algorithm for VAPOR.

## Specification on 2D bitstream file format

Each output file contians a header and followed by the actual bitstream. The header has a fixed size of **18** bytes with the following elements: 

1. X dimension:  (4 byte:  `uint32_t`)
2. Y dimension:  (4 byte:  `uint32_t`)
3. Input mean:   (8 byte:  `double`)
4. Max coefficient bits:  (2 byte:  `uint16_t`)

