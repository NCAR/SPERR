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


int QccECCReedSolomonCodeInitialize(QccECCReedSolomonCode *code)
{
  if (code == NULL)
    return(0);

  code->message_length = -1;
  code->code_length = -1;
  code->generator_matrix = NULL;
  if (QccECCFieldInitialize(&code->field))
    return(1);

  return(0);
}


int QccECCReedSolomonCodeAllocate(QccECCReedSolomonCode *code)
{
  int row, col;
  int field_size;

  if (code == NULL)
    return(0);

  if (QccECCFieldAllocate(&code->field))
    {
      QccErrorAddMessage("(QccECCReedSolomonCodeAllocate): Error calling QccECCFieldAllocate()");
      return(1);
    }

  field_size = (1 << code->field.size_exponent);
  if ((code->message_length < 1) || 
      (code->message_length >= field_size)  ||
      (code->code_length <= code->message_length) ||
      (code->code_length >= code->message_length + field_size))
    {
      QccErrorAddMessage("(QccECCReedSolomonCodeAllocate): (%d,%d) is not a valid code size",
                         code->code_length, code->message_length);
      return(1);
    }

  if ((code->generator_matrix = 
       QccECCFieldMatrixAllocate(code->code_length,
                                 code->message_length)) == NULL)
    {
      QccErrorAddMessage("(QccECCReedSolomonCodeAllocate): Error calling QccECCFieldMatrixAllocate()");
      return(1);
    }

  for (row = 0; row < code->message_length; row++)
    for (col = 0; col < code->message_length; col++)
      code->generator_matrix[row][col] = (row == col) ? 1 : 0;

  for (row = 0; row < (code->code_length - code->message_length); row++)
    for (col = 0; col < code->message_length; col++)
      code->generator_matrix[row + code->message_length][col] = 
        QccECCFieldExponential(row * col,
                               &code->field);

  return(0);
}


void QccECCReedSolomonCodeFree(QccECCReedSolomonCode *code)
{
  if (code == NULL)
    return;

  QccECCFieldMatrixFree(code->generator_matrix,
                        code->code_length);
  code->generator_matrix = NULL;
  QccECCFieldFree(&code->field);
}


void QccECCReedSolomonCodePrint(const QccECCReedSolomonCode *code)
{
  if (code == NULL)
    return;

  printf("Message length (k) = %d\n",
         code->message_length);
  printf("Code length (n) = %d\n",
         code->code_length);
  QccECCFieldPrint(&code->field);
  QccECCFieldMatrixPrint(code->generator_matrix,
                         code->code_length,
                         code->message_length,
                         &code->field,
                         QCCECCFIELDELEMENT_HEX);
}


int QccECCReedSolomonEncode(const QccECCFieldElement *message_symbols,
                            QccECCFieldElement *code_symbols,
                            const QccECCReedSolomonCode *code)
{
  int row, col;

  if (message_symbols == NULL)
    return(0);
  if (code_symbols == NULL)
    return(0);
  if (code == NULL)
    return(0);
  
  for (row = 0; row < code->message_length; row++)
    code_symbols[row] = message_symbols[row];

  for (row = code->message_length; row < code->code_length; row++)
    {
      code_symbols[row] = 0;
      for (col = 0; col < code->message_length; col++)
        code_symbols[row] =
          QccECCFieldAdd(code_symbols[row],
                         QccECCFieldMultiply(code->generator_matrix[row][col],
                                             message_symbols[col],
                                             &code->field),
                         &code->field);
    }

  return(0);
}


int QccECCReedSolomonDecode(const QccECCFieldElement *code_symbols,
                            QccECCFieldElement *message_symbols,
                            const int *erasure_vector,
                            const QccECCReedSolomonCode *code)
{
  int symbol_index;
  int num_erasures = 0;
  int decoding_needed = 0;
  QccECCFieldMatrix inversion_matrix = NULL;
  int row, col;
  int return_value;

  if (code_symbols == NULL)
    return(0);
  if (message_symbols == NULL)
    return(0);
  if (code == NULL)
    return(0);

  if (erasure_vector != NULL)
    for (symbol_index = 0; symbol_index < code->code_length; symbol_index++)
      if (erasure_vector[symbol_index])
        {
          num_erasures++;
          if (symbol_index < code->message_length)
            decoding_needed = 1;
        }

  if (!num_erasures || !decoding_needed)
    {
      for (symbol_index = 0; symbol_index < code->message_length;
           symbol_index++)
        message_symbols[symbol_index] = code_symbols[symbol_index];

      goto Done;
    }

  if (num_erasures > code->code_length - code->message_length)
    goto DecodeFailure;

  if ((inversion_matrix = 
       QccECCFieldMatrixAllocate(code->message_length,
                                 code->message_length + 1)) == NULL)
    {
      QccErrorAddMessage("(QccECCReedSolomonDecode): Error calling QccECCFieldMatrixAllocate()");
      goto Error;
    }
                                 
  for (symbol_index = 0, row = 0;
       row < code->message_length;
       symbol_index++)
    if (!erasure_vector[symbol_index])
      {
        for (col = 0; col < code->message_length; col++)
          inversion_matrix[row][col] = 
            code->generator_matrix[symbol_index][col];
        inversion_matrix[row][code->message_length] =
          code_symbols[symbol_index];
        row++;
      }

  if (QccECCFieldMatrixGaussianElimination(inversion_matrix,
                                           code->message_length,
                                           code->message_length + 1,
                                           &code->field))
    {
      QccErrorAddMessage("(QccECCReedSolomonDecode): Error calling QccECCFieldMatrixGaussianElimination()");
      goto Error;
    }

  for (row = 0; row < code->message_length; row++)
    message_symbols[row] = inversion_matrix[row][code->message_length];

 Done:
  return_value = 0;
  goto Return;
 DecodeFailure:
  return_value = 2;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccECCFieldMatrixFree(inversion_matrix, code->message_length);
  return(return_value);
}
