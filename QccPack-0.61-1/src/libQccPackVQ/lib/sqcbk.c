/*
 * 
 * QccPack: Quantization, compression, and coding libraries
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 * 
 */


#include "libQccPack.h"

int QccVQScalarQuantizerToVQCodebook(const
                                     QccSQScalarQuantizer *scalar_quantizer,
                                     QccVQCodebook *codebook)
{
  int num_codewords, codeword;

  if (scalar_quantizer == NULL)
    return(0);
  if (codebook == NULL)
    return(0);

  if ((scalar_quantizer->levels == NULL) ||
      (scalar_quantizer->probs == NULL))
    return(0);

  num_codewords = scalar_quantizer->num_levels;

  codebook->num_codewords = num_codewords;
  codebook->codeword_dimension = 1;

  if (QccVQCodebookAlloc(codebook))
    {
      QccErrorAddMessage("(QccVQScalarQuantizerToVQCodebook): Error calling QccVQCodebookAlloc()");
      return(1);
    }

  for (codeword = 0; codeword < num_codewords; codeword++)
    {
      codebook->codewords[codeword][0] = scalar_quantizer->levels[codeword];
      codebook->codeword_probs[codeword] = scalar_quantizer->probs[codeword];
    }

  QccVQCodebookSetIndexLength(codebook);
  QccVQCodebookSetCodewordLengths(codebook);

  return(0);
}
