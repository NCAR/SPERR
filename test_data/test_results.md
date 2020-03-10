# Accuracy Measures in decibel

## Lena512

*bpp*      *QccPack*       *Sam*

*0.25*       **32.74**          **32.52** 

*0.5*         **35.85**          **35.63**

*1*            **39.02**          **38.80**

*2*            **43.61**          **43.29** 



## Turbulence1024

*bpp*        *QccPack*         *Sam*

*0.25*        **34.90**            **34.63** 

*0.5*          **38.80**            **38.47**

*1*             **43.89**            **43.44**

*2*             **50.90**            **50.37**



## Turbulence128

*bpp*        *QccPack*         *Sam*

*0.25*        **33.46**            **33.21** 

*0.5*          **37.77**            **37.50**

*1*             **42.79**            **42.49**

*2*             **49.53**            **49.17**



# Speed Measures in Second

Note: the reported speed for QccPack include 1) Wavelet Transform, 2) SPECK Encoding, and 3) Arithmetic Coding, while for Sam, it includes only the first two steps.

Also note, these tests were performed on `cis-vapor` (Intel Core i7, Ubuntu 16.04 ) with `gcc-5.3` compiler. 