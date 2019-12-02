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


static const unsigned int QccECCFieldMinimumPolynomials[] =
{
  /*  2^0, not a field  */
  0x00000000,
  /*  2^1, polynomial = 1 + x  */
  0x00000003,
  /*  2^2, polynomial = 1 + x + x^2  */
  0x00000007,
  /*  2^3, polynomial = 1 + x + x^3  */
  0x0000000b,
  /*  2^4, polynomial = 1 + x + x^4  */
  0x00000013,
  /*  2^5, polynomial = 1 + x^2 + x^5  */
  0x00000025,
  /*  2^6, polynomial = 1 + x + x^6  */
  0x00000043,
  /*  2^7, polynomial = 1 + x^3 + x^7  */
  0x00000089,
  /*  2^8, polynomial = 1 + x^2 + x^3 + x^4 + x^8  */
  0x0000011d,
  /*  2^9, polynomial = 1 + x^4 + x^9  */
  0x00000211,
  /*  2^10, polynomial = 1 + x^3 + x^10  */
  0x00000409,
  /*  2^11, polynomial = 1 + x^2 + x^11  */
  0x00000805,
  /*  2^12, polynomial = 1 + x + x^4 + x^6 + x^12  */
  0x00001053,
  /*  2^13, polynomial = 1 + x + x^3 + x^4 + x^13  */
  0x0000201b,
  /*  2^14, polynomial = 1 + x + x^6 + x^10 + x^14  */
  0x00004443,
  /*  2^15, polynomial = 1 + x + x^15  */
  0x00008003,
  /*  2^16, polynomial = 1 + x + x^3 + x^12 + x^16  */
  0x0001100b,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};


int QccECCFieldInitialize(QccECCField *field)
{
  if (field == NULL)
    return(0);

  field->size_exponent = -1;
  field->mask = 0;
  field->high_bit = 0;
  field->minimum_polynomial = 0;

  return(0);
}


static QccECCFieldElement QccECCFieldMultiplyByAlpha(QccECCFieldElement
                                                     element,
                                                     const QccECCField *field)
{
  if (element & field->high_bit)
    {
      element = QccECCFieldAdd(element, 
                               (field->minimum_polynomial >> 1),
                               field);
      element = ((element << 1) | 1) & field->mask;
    }
  else
    element = (element << 1) & field->mask;

  return(element);
}


int QccECCFieldAllocate(QccECCField *field)
{
  int field_size;
  QccECCFieldElement element;
  int exponent;

  if ((field->size_exponent < 1) ||
      (field->size_exponent > QCCECCFIELD_MAX_SIZEEXPONENT))
    {
      QccErrorAddMessage("(QccECCFieldAllocate): Invalid field size");
      return(1);
    }

  field_size = (1 << field->size_exponent);

  field->mask = field_size - 1;
  field->high_bit = 1 << (field->size_exponent - 1);
  field->minimum_polynomial = 
    QccECCFieldMinimumPolynomials[field->size_exponent];

  if ((field->log_table = (int *)malloc(sizeof(int) *
                                        field_size)) == NULL)
    {
      QccErrorAddMessage("(QccECCFieldAllocate): Error allocating memory");
      return(1);
    }

  if ((field->exponential_table =
       (QccECCFieldElement *)malloc(sizeof(QccECCFieldElement) *
                                    field_size)) == NULL)
    {
      QccErrorAddMessage("(QccECCFieldAllocate): Error allocating memory");
      return(1);
    }
    
  field->log_table[0] = field_size - 1;
  field->exponential_table[field_size - 1] = 0;

  element = 1;
  exponent = 0;
  do
    {
      field->log_table[element] = exponent;
      field->exponential_table[exponent] = element;
      element = QccECCFieldMultiplyByAlpha(element, field);
      exponent++;
    }
  while (exponent < field_size - 1);

  return(0);
}


void QccECCFieldFree(QccECCField *field)
{
  if (field == NULL)
    return;

  if (field->log_table != NULL)
    {
      QccFree(field->log_table);
      field->log_table = NULL;
    }
  if (field->exponential_table != NULL)
    {
      QccFree(field->exponential_table);
      field->exponential_table = NULL;
    }
}


QccECCFieldMatrix QccECCFieldMatrixAllocate(int num_rows, int num_cols)
{
  QccECCFieldMatrix matrix;
  int row;

  if ((matrix = 
       (QccECCFieldMatrix)malloc(sizeof(QccECCFieldElement *) * num_rows)) == 
      NULL)
    {
      QccErrorAddMessage("(QccECCFieldMatrixAllocate): Error allocating memory");
      return(NULL);
    }

  for (row = 0; row < num_rows; row++)
    if ((matrix[row] = 
         (QccECCFieldElement *)malloc(sizeof(QccECCFieldElement) * num_cols)) 
        == NULL)
      {
        QccErrorAddMessage("(QccECCFieldMatrixAllocate): Error allocating memory");
        QccFree(matrix);
        return(NULL);
      }

  return(matrix);
}


void QccECCFieldMatrixFree(QccECCFieldMatrix matrix, int num_rows)
{
  int row;

  if (matrix != NULL)
    {
      for (row = 0; row < num_rows; row++)
        QccFree(matrix[row]);
      QccFree(matrix);
    }
}


void QccECCFieldElementPrint(QccECCFieldElement element,
                             const QccECCField *field,
                             int representation)
{
  int power = 0;
  int nibble = 0;
  QccString string;
  QccString string1;
  QccECCFieldElement mask;
  int first_term = 1;

  QccStringMakeNull(string);

  mask = field->mask;

  switch (representation)
    {
    case QCCECCFIELDELEMENT_DECIMAL:
      printf("%u", element);
      break;
      
    default:
    case QCCECCFIELDELEMENT_BINARY:
      while (mask)
        {
          if (element & 1)
            QccStringSprintf(string1, "1%s", string);
          else
            QccStringSprintf(string1, "0%s", string);
          QccStringSprintf(string, "%s", string1);
          mask >>= 1;
          element >>= 1;
        }
      if (field->size_exponent == QCCECCFIELD_MAX_SIZEEXPONENT)
        printf("1%s", string);
      else
        printf("%s", string);
      break;
      
    case QCCECCFIELDELEMENT_HEX:
      while (mask)
        {
          nibble++;
          mask >>= 4;
        }
      printf("%0*x",
             nibble, element);
      break;

    case QCCECCFIELDELEMENT_POLYNOMIAL:
      if (!element)
        printf("0");
      while (element)
        {
          if (element & 1)
            {
              if (!power)
                printf("1");
              else
                if (power == 1)
                  if (first_term)
                    printf("x");
                  else
                    printf(" + x");
                else
                  if (first_term)
                    printf("x^%d", power);
                  else
                    printf(" + x^%d", power);
              first_term = 0;
            }
          power++;
          element >>= 1;
        }
      if (field->size_exponent == QCCECCFIELD_MAX_SIZEEXPONENT)
        printf(" + x^%d", field->size_exponent);
      break;
    }
}


void QccECCFieldMatrixPrint(const QccECCFieldMatrix matrix,
                            int num_rows, int num_cols,
                            const QccECCField *field,
                            int representation)
{
  int row, col;

  if (matrix == NULL)
    return;

  for (row = 0; row < num_rows; row++)
    {
      for (col = 0; col < num_cols; col++)
        {
          QccECCFieldElementPrint(matrix[row][col],
                                  field, representation);
          printf(" ");
        }
      printf("\n");
    }
}

                            
void QccECCFieldPrint(const QccECCField *field)
{
  if (field == NULL)
    return;

  if (field->size_exponent <= 16)
    printf("Field size: 2^%d = %d\n", 
           field->size_exponent,
           (int)pow((double)2, (double)field->size_exponent));
  else
    printf("Field size: 2^%d\n", 
           field->size_exponent);
  printf("Field mask: 0x");
  QccECCFieldElementPrint(field->mask,
                          field,
                          QCCECCFIELDELEMENT_HEX);
  printf("\nField high bit: 0x");
  QccECCFieldElementPrint(field->high_bit,
                          field,
                          QCCECCFIELDELEMENT_HEX);
  printf("\nMinimum polynomial:\n  ");
  QccECCFieldElementPrint(field->minimum_polynomial, field,
                          QCCECCFIELDELEMENT_POLYNOMIAL);
  printf("\n");
}
