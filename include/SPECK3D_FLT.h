#ifndef SPECK3D_FLT_H
#define SPECK3D_FLT_H

#include "SPECK_FLT.h"

namespace sperr {

class SPECK3D_FLT : public SPECK_FLT {

 protected:
  virtual void m_instantiate_encoder() override;
  virtual void m_instantiate_decoder() override;

  virtual void m_wavelet_xform() override;
  virtual void m_inverse_wavelet_xform() override;

  virtual auto m_quantize() -> RTNType override;
  virtual auto m_inverse_quantize() -> RTNType override;
};

};  // namespace sperr

#endif
