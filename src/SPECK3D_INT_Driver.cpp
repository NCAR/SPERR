#include "SPECK3D_INT_Driver.h"

#include "SPECK3D_INT_ENC.h"
#include "SPECK3D_INT_DEC.h"

sperr::SPECK3D_INT_Driver::SPECK3D_INT_Driver()
{
  m_encoder = std::make_unique<SPECK3D_INT_ENC>();
  m_decoder = std::make_unique<SPECK3D_INT_DEC>();
}
