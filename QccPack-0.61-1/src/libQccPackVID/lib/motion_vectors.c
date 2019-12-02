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


/*
 * This is the H.261 variable-length code table for motion-vector data (MVD)
 * 
 * mvd_bits is the list of the number of codewords for each codeword
 *   length, starting at length 1 and going to length 11
 *
 * mvd_symbols is the list of symbols represented as a 2s-complement byte;
 *   that is, 0x00 = 0, 0x01 = 1, 0xff = -1, 0xf0 = -16.  This table stores
 *   only the symbols in the range -16 to 15; however, each codeword (except
 *   those for symbols -1, 0, 1) is in reality associated with another
 *   symbol. This second symbol is 32 + symbol, if symbol < 0, or -32 + symbol,
 *   if symbol > 0.  In decoding, use whichever symbol of the two possible
 *   symbols that results in the motion-vector component in the valid range of
 *   -15 to 15.
 */
static int QccVIDMotionVectorsMVDBits[11] =
{ 1, 0, 2, 2, 2, 0, 2, 6, 0, 6, 11 };

static int QccVIDMotionVectorsMVDSymbols[32] =
{
0x00, 0xff, 0x01, 0xfe, 0x02, 0xfd, 0x03, 0xfc,
0x04, 0xfb, 0x05, 0xfa, 0x06, 0xf9, 0x07, 0xf8,
0x08, 0xf7, 0x09, 0xf6, 0x0a, 0xf5, 0x0b, 0xf4,
0x0c, 0xf3, 0x0d, 0xf2, 0x0e, 0xf1, 0x0f, 0xf0
};


void QccVIDMotionVectorsTableInitialize(QccVIDMotionVectorsTable *table)
{
  QccENTHuffmanTableInitialize(&table->encode_table);
  QccENTHuffmanTableInitialize(&table->decode_table);
}


int QccVIDMotionVectorsTableCreate(QccVIDMotionVectorsTable *table)
{
  int i;

  table->encode_table.num_codewords_list_length = 11;
  if ((table->encode_table.num_codewords_list =
       (int *)malloc(sizeof(int) * 11)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsTableCreate): Error allocating memory");
      return(1);
    }
  for (i = 0; i < 11; i++)
    table->encode_table.num_codewords_list[i] = QccVIDMotionVectorsMVDBits[i];

  table->encode_table.symbol_list_length = 32;
  if ((table->encode_table.symbol_list = 
       (int *)malloc(sizeof(int) * 32)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsTableCreate): Error allocating memory");
      return(1);
    }
  for (i = 0; i < 32; i++)
    table->encode_table.symbol_list[i] = QccVIDMotionVectorsMVDSymbols[i];

  if (QccENTHuffmanTableCreateEncodeTable(&table->encode_table))
    {
      QccErrorAddMessage("(QccVIDMotionVectorsTableCreate): Error calling QccENTHuffmanTableCreateEncodeTable()");
      return(1);
    }

  table->decode_table.num_codewords_list_length = 11;
  if ((table->decode_table.num_codewords_list =
       (int *)malloc(sizeof(int) * 11)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsTableCreate): Error allocating memory");
      return(1);
    }
  for (i = 0; i < 11; i++)
    table->decode_table.num_codewords_list[i] = QccVIDMotionVectorsMVDBits[i];

  table->decode_table.symbol_list_length = 32;
  if ((table->decode_table.symbol_list = 
       (int *)malloc(sizeof(int) * 32)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsTableCreate): Error allocating memory");
      return(1);
    }
  for (i = 0; i < 32; i++)
    table->decode_table.symbol_list[i] = QccVIDMotionVectorsMVDSymbols[i];

  if (QccENTHuffmanTableCreateDecodeTable(&table->decode_table))
    {
      QccErrorAddMessage("(QccVIDMotionVectorsTableCreate): Error calling QccENTHuffmanTableCreateDecodeTable()");
      return(1);
    }

  return(0);
}


void QccVIDMotionVectorsTableFree(QccVIDMotionVectorsTable *table)
{
  if (table == NULL)
    return;

  QccENTHuffmanTableFree(&table->encode_table);
  QccENTHuffmanTableFree(&table->decode_table);
}


static int QccVIDMotionVectorsEncodeMVD(QccBitBuffer *output_buffer,
                                        int motion_component,
                                        const QccVIDMotionVectorsTable *table)
{
  int code;
  
  if (abs(motion_component) > 30)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsEncodeMVD): Vector component %d outside of allowable range of [-30, +30]",
                         motion_component);
      return(1);
    }

  if (motion_component == 16)
    code = -16;
  else
    if (abs(motion_component) > 16)
      code =
	(motion_component > 0) ?
	-32 + motion_component : 32 + motion_component;
    else
      code = motion_component;
  
  code &= 0xff;
  
  if (QccENTHuffmanEncode(output_buffer, &code, 1,
                          &table->encode_table))
    {
      QccErrorAddMessage("(QccVIDMotionVectorsEncodeMVD): Error calling QccENTHuffmanEncode()");
      return(1);
    }
  
  return(0);
}


static int QccVIDMotionVectorsDecodeMVD(QccBitBuffer *input_buffer,
                                        int *motion_component,
                                        int previous_motion_component,
                                        const QccVIDMotionVectorsTable
                                        *table)
{
  int u;

  if (QccENTHuffmanDecode(input_buffer, &u, 1, &table->decode_table))
    {
      QccErrorAddMessage("(QccVIDMotionVectorsDecodeMVD): Error calling QccENTHuffmanDecode()");
      return(1);
    }

  u = (u << 24) >> 24;
  *motion_component = u + previous_motion_component;

  if (abs(*motion_component) > 15)
    {
      if (u < 0)
	u += 32;
      else
	u -= 32;

      *motion_component = u + previous_motion_component;
    }

  return(0);
}


static int QccVIDMotionVectorsEncodeVector(double motion_horizontal,
                                           double motion_vertical,
                                           double *previous_motion_horizontal,
                                           double *previous_motion_vertical,
                                           const QccVIDMotionVectorsTable
                                           *table,
                                           int subpixel_accuracy,
                                           QccBitBuffer *output_buffer)
{
  int u, v;
  int subpixel_u, subpixel_v;
  int num_subpixels;

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Unrecognized subpixel accuracy");
      return(1);
    }

  if (table != NULL)
    {
      u = (int)motion_horizontal - (int)(*previous_motion_horizontal);
      v = (int)motion_vertical - (int)(*previous_motion_vertical);
      
      *previous_motion_horizontal = motion_horizontal;
      *previous_motion_vertical = motion_vertical;
      
      if (QccVIDMotionVectorsEncodeMVD(output_buffer,
                                       u,
                                       table))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Error calling QccVIDMotionVectorsEncodeMVD()");
          return(1);
        }
      if (QccVIDMotionVectorsEncodeMVD(output_buffer,
                                       v,
                                       table))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Error calling QccVIDMotionVectorsEncodeMVD()");
          return(1);
        }
      
      if (num_subpixels > 1)
        {
          subpixel_u =
            (int)((motion_horizontal -
                   (int)(motion_horizontal)) * num_subpixels);
          subpixel_v =
            (int)((motion_vertical -
                   (int)(motion_vertical)) * num_subpixels);
          
          if (QccVIDMotionVectorsEncodeMVD(output_buffer,
                                           subpixel_u,
                                           table))
            {
              QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Error calling QccVIDMotionVectorsEncodeMVD()");
              return(1);
            }
          if (QccVIDMotionVectorsEncodeMVD(output_buffer,
                                           subpixel_v,
                                           table))
            {
              QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Error calling QccVIDMotionVectorsEncodeMVD()");
              return(1);
            }
        }
    }
  else
    {
      u = (int)((motion_horizontal -
                 (*previous_motion_horizontal)) * num_subpixels);
      v = (int)((motion_vertical -
                 (*previous_motion_vertical)) * num_subpixels);
      
      *previous_motion_horizontal = motion_horizontal;
      *previous_motion_vertical = motion_vertical;
      
      if (QccENTExponentialGolombEncode(output_buffer,
                                        &u,
                                        1,
                                        1))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Error calling QccENTExponentialGolombEncode()");
          return(1);
        }
      if (QccENTExponentialGolombEncode(output_buffer,
                                        &v,
                                        1,
                                        1))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsEncodeVector): Error calling QccENTExponentialGolombEncode()");
          return(1);
        }
    }

  return(0);
}


static int QccVIDMotionVectorsDecodeVector(double *motion_horizontal,
                                           double *motion_vertical,
                                           double *previous_motion_horizontal,
                                           double *previous_motion_vertical,
                                           const QccVIDMotionVectorsTable
                                           *table,
                                           int subpixel_accuracy,
                                           QccBitBuffer *input_buffer)
{
  int u, v;
  int subpixel_u, subpixel_v;
  int num_subpixels;

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      num_subpixels = 1;
      break;
    case QCCVID_ME_HALFPIXEL:
      num_subpixels = 2;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      num_subpixels = 4;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      num_subpixels = 8;
      break;
    default:
      QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Unrecognized subpixel accuracy");
      return(1);
    }

  if (table != NULL)
    {
      if (QccVIDMotionVectorsDecodeMVD(input_buffer,
                                       &u,
                                       (int)(*previous_motion_horizontal),
                                       table))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Error calling QccVIDMotionVectorsDecodeMVD()");
          return(1);
        }
      if (QccVIDMotionVectorsDecodeMVD(input_buffer,
                                       &v,
                                       (int)(*previous_motion_vertical),
                                       table))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Error calling QccVIDMotionVectorsDecodeMVD()");
          return(1);
        }
      
      if (num_subpixels > 1)
        {
          if (QccVIDMotionVectorsDecodeMVD(input_buffer,
                                           &subpixel_u,
                                           0,
                                           table))
            {
              QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Error calling QccVIDMotionVectorsDecodeMVD()");
              return(1);
            }
          if (QccVIDMotionVectorsDecodeMVD(input_buffer,
                                           &subpixel_v,
                                           0,
                                           table))
            {
              QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Error calling QccVIDMotionVectorsDecodeMVD()");
              return(1);
            }
        }
      else
        subpixel_u = subpixel_v = 0;
      
      *motion_horizontal = u + (double)subpixel_u / num_subpixels;
      *motion_vertical = v + (double)subpixel_v / num_subpixels;
    }
  else
    {
      if (QccENTExponentialGolombDecode(input_buffer,
                                        &u,
                                        1,
                                        1))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Error calling QccENTExponentialGolombDecode()");
          return(1);
        }
      if (QccENTExponentialGolombDecode(input_buffer,
                                        &v,
                                        1,
                                        1))
        {
          QccErrorAddMessage("(QccVIDMotionVectorsDecodeVector): Error calling QccENTExponentialGolombDecode()");
          return(1);
        }
      
      *motion_horizontal =
        ((double)u / num_subpixels) + (*previous_motion_horizontal);
      *motion_vertical =
        ((double)v / num_subpixels) + (*previous_motion_vertical);
    }

  *previous_motion_horizontal = *motion_horizontal;
  *previous_motion_vertical = *motion_vertical;
  
  return(0);
}


int QccVIDMotionVectorsEncode(const QccIMGImageComponent
                              *motion_vector_horizontal,
                              const QccIMGImageComponent
                              *motion_vector_vertical,
                              const QccVIDMotionVectorsTable *table,
                              int subpixel_accuracy,
                              QccBitBuffer *output_buffer)
{
  double previous_motion_horizontal;
  double previous_motion_vertical;
  int row, col;
  
  for (row = 0; row < motion_vector_horizontal->num_rows; row++)
    {
      previous_motion_horizontal = 0;
      previous_motion_vertical = 0;
      
      for (col = 0; col < motion_vector_horizontal->num_cols; col++)
        if (QccVIDMotionVectorsEncodeVector(motion_vector_horizontal->image
                                            [row][col],
                                            motion_vector_vertical->image
                                            [row][col],
                                            &previous_motion_horizontal,
                                            &previous_motion_vertical,
                                            table,
                                            subpixel_accuracy,
                                            output_buffer))
          {
            QccErrorAddMessage("(QccVIDMotionVectorsEncode): Error calling QccVIDMotionVectorsEncodeVector()");
            return(1);
          }
    }
  
  return(0);
}


int QccVIDMotionVectorsDecode(QccIMGImageComponent
                              *motion_vector_horizontal,
                              QccIMGImageComponent
                              *motion_vector_vertical,
                              const QccVIDMotionVectorsTable *table,
                              int subpixel_accuracy,
                              QccBitBuffer *input_buffer)
{
  double previous_motion_horizontal;
  double previous_motion_vertical;
  int row, col;
  
  for (row = 0; row < motion_vector_horizontal->num_rows; row++)
    {
      previous_motion_horizontal = 0;
      previous_motion_vertical = 0;
      
      for (col = 0; col < motion_vector_horizontal->num_cols; col++)
        if (QccVIDMotionVectorsDecodeVector(&motion_vector_horizontal->image
                                            [row][col],
                                            &motion_vector_vertical->image
                                            [row][col],
                                            &previous_motion_horizontal,
                                            &previous_motion_vertical,
                                            table,
                                            subpixel_accuracy,
                                            input_buffer))
          {
            QccErrorAddMessage("(QccVIDMotionVectorsDecode): Error calling QccVIDMotionVectorsDecodeVector()");
            return(1);
          }
    }
  
  return(0);
}


int QccVIDMotionVectorsReadFile(QccIMGImageComponent
                                *motion_vectors_horizontal,
                                QccIMGImageComponent
                                *motion_vectors_vertical,
                                const QccString filename,
                                int frame_num)
{
  QccString current_filename;
  FILE *current_file;
  int row, col;

  if (motion_vectors_horizontal == NULL)
    return(0);
  if (motion_vectors_vertical == NULL)
    return(0);
  if (filename == NULL)
    return(0);
  if (QccStringNull(filename))
    return(0);

  if (motion_vectors_horizontal->image == NULL)
    return(0);
  if (motion_vectors_vertical->image == NULL)
    return(0);

  if (frame_num < 0)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsReadFile): Invalid frame number (%d)",
                         frame_num);
      return(1);
    }

  if ((motion_vectors_horizontal->num_rows !=
       motion_vectors_vertical->num_rows) ||
      (motion_vectors_horizontal->num_cols !=
       motion_vectors_vertical->num_cols))
    {
      QccErrorAddMessage("(QccVIDMotionVectorsReadFile): Motion-vector fields must have same size");
      return(1);
    }

  QccStringSprintf(current_filename, filename, frame_num);

  if ((current_file = QccFileOpen(current_filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsReadFile): Error calling QccFileOpen()");
      return(1);
    }

  for (row = 0; row < motion_vectors_horizontal->num_rows; row++)
    for (col = 0; col < motion_vectors_horizontal->num_cols; col++)
      {
        fscanf(current_file, "%lf",
               &motion_vectors_vertical->image[row][col]);
        if (ferror(current_file) || feof(current_file))
          {
            QccErrorAddMessage("(QccVIDMotionVectorsReadFile): Error reading file %s",
                               current_filename);
            return(1);
          }
        fscanf(current_file, "%lf",
               &motion_vectors_horizontal->image[row][col]);
        if (ferror(current_file) || feof(current_file))
          {
            QccErrorAddMessage("(QccVIDMotionVectorsReadFile): Error reading file %s",
                               current_filename);
            return(1);
          }
      }

  QccFileClose(current_file);

  return(0);
}


int QccVIDMotionVectorsWriteFile(const QccIMGImageComponent
                                 *motion_vectors_horizontal,
                                 const QccIMGImageComponent
                                 *motion_vectors_vertical,
                                 const QccString filename,
                                 int frame_num)
{
  QccString current_filename;
  FILE *current_file;
  int row, col;

  if (motion_vectors_horizontal == NULL)
    return(0);
  if (motion_vectors_vertical == NULL)
    return(0);
  if (filename == NULL)
    return(0);
  if (QccStringNull(filename))
    return(0);

  if (frame_num < 0)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsWriteFile): Invalid frame number (%d)",
                         frame_num);
      return(1);
    }

  if ((motion_vectors_horizontal->num_rows !=
       motion_vectors_vertical->num_rows) ||
      (motion_vectors_horizontal->num_cols !=
       motion_vectors_vertical->num_cols))
    {
      QccErrorAddMessage("(QccVIDMotionVectorsWriteFile): Motion-vector fields must have same size");
      return(1);
    }

  QccStringSprintf(current_filename, filename, frame_num);

  if ((current_file = QccFileOpen(current_filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccVIDMotionVectorsWriteFile): Error calling QccFileOpen()");
      return(1);
    }

  for (row = 0; row < motion_vectors_horizontal->num_rows; row++)
    for (col = 0; col < motion_vectors_horizontal->num_cols; col++)
      fprintf(current_file, "% 11.4f % 11.4f\n",
              motion_vectors_vertical->image[row][col],
              motion_vectors_horizontal->image[row][col]);

  QccFileClose(current_file);

  return(0);
}


