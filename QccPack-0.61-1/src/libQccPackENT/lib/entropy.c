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


double QccENTEntropy(const QccVector probs, int num_symbols)
{
  int symbol;
  double sum = 0.0;

  for (symbol = 0; symbol < num_symbols; symbol++)
    if (probs[symbol] > 0.0)
      sum -= probs[symbol] * log(probs[symbol]);

  return(sum/log((double)2.0));
}


double QccENTJointEntropy(const QccMatrix probs_array,
                          int num_symbols1, int num_symbols2)
{
  int symbol2;
  int symbol1;
  double sum = 0.0;

  for (symbol1 = 0; symbol1 < num_symbols1; symbol1++)
    for (symbol2 = 0; symbol2 < num_symbols2; symbol2++)
      if (probs_array[symbol1][symbol2] > 0.0)
        sum -= probs_array[symbol1][symbol2] *
          log(probs_array[symbol1][symbol2]);

  return(sum/log((double)2.0));
}


double QccENTConditionalEntropy(const QccMatrix probs_array,
                                int num_contexts, int num_symbols)
{
  QccVector conditional_probs = NULL;
  double entropy = 0.0;
  int symbol;
  int context;
  double marginal_prob;
  double return_value;

  if ((conditional_probs = QccVectorAlloc(num_symbols)) == NULL)
    {
      QccErrorAddMessage("(QccENTConditionalEntropy): Error calling QccVectorAlloc()");
      goto Error;
    }

  for (context = 0; context < num_contexts; context++)
    {
      marginal_prob = 
        QccVectorSumComponents(probs_array[context], num_symbols);
      for (symbol = 0; symbol < num_symbols; symbol++)
        conditional_probs[symbol] =
          probs_array[context][symbol] / marginal_prob;
      entropy +=
        marginal_prob * QccENTEntropy(conditional_probs, num_symbols);
    }
  
  return_value = entropy;
  goto Return;
 Error:
  return_value = -1.0;
  goto Return;
 Return:
  QccVectorFree(conditional_probs);
  return(return_value);
}
