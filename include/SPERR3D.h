#ifndef SPERR3D_H
#define SPERR3D_H

#include "SPERR_Driver.h"

namespace sperr {

class SPERR3D : public SPERR_Driver {
 public:
  //
  // Constructor
  //
  //SPERR3D();

 protected:
  virtual void m_instantiate_coders() override;

  virtual void m_wavelet_xform() override;
  virtual void m_inverse_wavelet_xform() override;

  virtual auto m_quantize() -> RTNType override;
  virtual auto m_inverse_quantize() -> RTNType override;
};

};  // namespace sperr

#endif
