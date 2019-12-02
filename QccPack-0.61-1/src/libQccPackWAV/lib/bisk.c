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


#define QCCBISK_BOUNDARY_VALUE 0

#define QCCBISK_INSIGNIFICANT 0
#define QCCBISK_SIGNIFICANT 1
#define QCCBISK_NEWLY_SIGNIFICANT 2
#define QCCBISK_EMPTYSET 3

#define QCCBISK_POSITIVE 0
#define QCCBISK_NEGATIVE 1

#define QCCBISK_MAXBITPLANES 128

#define QCCBISK_CONTEXT_NOCODE -1
#define QCCBISK_CONTEXT_SIGNIFICANCE 0
#define QCCBISK_CONTEXT_SIGNIFICANCE_SUBSET1 1
#define QCCBISK_CONTEXT_SIGNIFICANCE_SUBSET2 2
#define QCCBISK_CONTEXT_SIGN 3
#define QCCBISK_CONTEXT_REFINEMENT 4
#define QCCBISK_NUM_CONTEXTS 5
static const int QccWAVbiskNumSymbols[] =
  { 2, 2, 2, 2, 2 };


typedef struct
{
  int level;
  int origin_row;
  int origin_col;
  int num_rows;
  int num_cols;
  char significance;
} QccWAVbiskSet;


typedef struct
{
  QccWAVSubbandPyramid original_coefficients;
  QccWAVSubbandPyramid reconstructed_coefficients;
  QccWAVSubbandPyramid squared_errors;
  double distortion;
  int num_pixels;
  int start_position;
  FILE *file;
} QccWAVbiskDistortionTrace;


static int QccWAVbiskTransparent(const QccWAVSubbandPyramid *mask,
                                 int row, int col)
{
  if (mask == NULL)
    return(0);
  return(QccAlphaTransparent(mask->matrix[row][col]));
}


static void QccWAVbiskDistortionTraceInitialize(QccWAVbiskDistortionTrace
                                                *distortion_trace,
                                                QccWAVSubbandPyramid
                                                *original_coefficients,
                                                QccWAVSubbandPyramid
                                                *mask,
                                                QccBitBuffer *output_buffer,
                                                FILE *outfile)
{
  int row, col;
  
  QccWAVSubbandPyramidInitialize(&distortion_trace->original_coefficients);
  QccWAVSubbandPyramidInitialize(&distortion_trace->reconstructed_coefficients);
  QccWAVSubbandPyramidInitialize(&distortion_trace->squared_errors);
  
  QccWAVSubbandPyramidCopy(&distortion_trace->original_coefficients,
                           original_coefficients);
  QccWAVSubbandPyramidCopy(&distortion_trace->reconstructed_coefficients,
                           original_coefficients);
  QccWAVSubbandPyramidCopy(&distortion_trace->squared_errors,
                           original_coefficients);
  
  QccMatrixZero(distortion_trace->reconstructed_coefficients.matrix,
                distortion_trace->reconstructed_coefficients.num_rows,
                distortion_trace->reconstructed_coefficients.num_cols);
  
  
  distortion_trace->distortion = 0;
  distortion_trace->num_pixels = 0;
  distortion_trace->start_position = output_buffer->bit_cnt;

  if (mask == NULL)
    {
      for (row = 0; row < distortion_trace->squared_errors.num_rows; row++)
        for (col = 0; col < distortion_trace->squared_errors.num_cols; col++)
          {
            distortion_trace->squared_errors.matrix[row][col] *=
              distortion_trace->squared_errors.matrix[row][col];
            distortion_trace->distortion +=
              distortion_trace->squared_errors.matrix[row][col];
            distortion_trace->num_pixels++;
          }
    }
  else
    for (row = 0; row < distortion_trace->squared_errors.num_rows; row++)
      for (col = 0; col < distortion_trace->squared_errors.num_cols; col++)
        if (!QccAlphaTransparent(mask->matrix[row][col]))
          {
            distortion_trace->squared_errors.matrix[row][col] *=
              distortion_trace->squared_errors.matrix[row][col];
            distortion_trace->distortion +=
              distortion_trace->squared_errors.matrix[row][col];
            distortion_trace->num_pixels++;
          }
  
  distortion_trace->file = outfile;
  
  fprintf(outfile,
          "      Rate (bpp)\t      MSE\n");
  fprintf(outfile,
          "--------------------------------------------\n");
}


static void QccWAVbiskDistortionTraceUpdate(QccWAVbiskDistortionTrace
                                            *distortion_trace,
                                            int row,
                                            int col,
                                            QccBitBuffer *buffer)
{
  double diff;
  
  QccMatrix original_coefficients =
    distortion_trace->original_coefficients.matrix;
  QccMatrix reconstructed_coefficients =
    distortion_trace->reconstructed_coefficients.matrix;
  QccMatrix squared_errors =
    distortion_trace->squared_errors.matrix;
  
  distortion_trace->distortion -= squared_errors[row][col];
  diff = original_coefficients[row][col] - 
    reconstructed_coefficients[row][col];
  squared_errors[row][col] = diff * diff;
  distortion_trace->distortion += squared_errors[row][col];
  
  fprintf(distortion_trace->file,
          "%16.6f\t%16.6f\n",
          (double)(buffer->bit_cnt - distortion_trace->start_position) /
          distortion_trace->num_pixels,
          distortion_trace->distortion / distortion_trace->num_pixels);
}


static int QccWAVbiskSetIsPixel(QccWAVbiskSet *set)
{
  if ((set->num_rows == 1) && (set->num_cols == 1))
    return(1);
  
  return(0);
}


#if 0
static void QccWAVbiskSetPrint(const QccWAVbiskSet *set)
{
  printf("<%d,%d,%d,%d,%d,%c>\n",
         set->level,
         set->origin_row,
         set->origin_col,
         set->num_rows,
         set->num_cols,
         ((set->significance == QCCBISK_INSIGNIFICANT) ? 'i' :
          ((set->significance == QCCBISK_SIGNIFICANT) ? 's' : 'S')));
}


static void QccWAVbiskLISPrint(const QccList *LIS)
{
  QccListNode *current_list_node;
  QccList *current_list;
  QccListNode *current_set_node;
  QccWAVbiskSet *current_set;
  int level = 0;

  printf("LIS:\n");
  current_list_node = LIS->start;
  while (current_list_node != NULL)
    {
      printf("Level %d:\n", level++);

      current_list = (QccList *)(current_list_node->value);
      current_set_node = current_list->start;
      while (current_set_node != NULL)
        {
          current_set = (QccWAVbiskSet *)(current_set_node->value);
          QccWAVbiskSetPrint(current_set);
          current_set_node = current_set_node->next;
        }

      current_list_node = current_list_node->next;
    }

  printf("===============================================\n");

  fflush(stdout);
}


static void QccWAVbiskLSPPrint(const QccList *LSP)
{
  QccListNode *current_set_node;
  QccWAVbiskSet *current_set;

  printf("LSP:\n");
  
  current_set_node = LSP->start;
  while (current_set_node != NULL)
    {
      current_set = (QccWAVbiskSet *)(current_set_node->value);
      QccWAVbiskSetPrint(current_set);
      current_set_node = current_set_node->next;
    }

  printf("===============================================\n");

  fflush(stdout);
}
#endif


static int QccWAVbiskSetShrink(QccWAVbiskSet *set,
                               const QccWAVSubbandPyramid *mask)
{
  int row;
  int col;
  int min_row = MAXINT;
  int min_col = MAXINT;
  int max_row = -MAXINT;
  int max_col = -MAXINT;
  int totally_transparent = 1;

  if (mask == NULL)
    {
      if ((!set->num_rows) || (!set->num_cols))
        return(2);
      else
        return(0);
    }

  for (row = 0; row < set->num_rows; row++)
    for (col = 0; col < set->num_cols; col++)
      if (!QccWAVbiskTransparent(mask,
                                 row + set->origin_row,
                                 col + set->origin_col))
        {
          totally_transparent = 0;
          max_row = QccMathMax(max_row, row + set->origin_row);
          max_col = QccMathMax(max_col, col + set->origin_col);
          
          min_row = QccMathMin(min_row, row + set->origin_row);
          min_col = QccMathMin(min_col, col + set->origin_col);
        }
  
  if (totally_transparent)
    return(2);

  set->origin_row = min_row;
  set->origin_col = min_col;
  set->num_rows = max_row - min_row + 1;
  set->num_cols = max_col - min_col + 1;

  return(0);
}


static int QccWAVbiskSetSize(QccWAVbiskSet *set,
                             const QccWAVSubbandPyramid *coefficients,
                             const QccWAVSubbandPyramid *mask,
                             int subband)
{
  int return_value;

  if (QccWAVSubbandPyramidSubbandOffsets(coefficients,
                                         subband,
                                         &set->origin_row,
                                         &set->origin_col))
    {
      QccErrorAddMessage("(QccWAVbiskSetSize): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      return(1);
    }
  if (QccWAVSubbandPyramidSubbandSize(coefficients,
                                      subband,
                                      &set->num_rows,
                                      &set->num_cols))
    {
      QccErrorAddMessage("(QccWAVbiskSetSize): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }
  
  return_value = QccWAVbiskSetShrink(set, mask);
  
  if (return_value == 2)
    return(2);
  else
    if (return_value == 1)
      {
        QccErrorAddMessage("(QccWAVbiskSetSize): Error calling QccWAVShapeAdaptiveMaskBoundingBox()");
        return(1);
      }

  return(0);
}


static int QccWAVbiskLengthenLIS(QccList *LIS)
{
  QccList new_list;
  QccListNode *new_list_node;

  QccListInitialize(&new_list);
  if ((new_list_node = QccListCreateNode(sizeof(QccList),
                                         &new_list)) == NULL)
    {
      QccErrorAddMessage("(QccWAVbiskLengthenLIS): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(LIS, new_list_node))
    {
      QccErrorAddMessage("(QccWAVbiskLengthenLIS): Error calling QccListAppendNode()");
      return(1);
    }

  return(0);
}


static int QccWAVbiskInsertSet(QccList *LIS,
                               QccWAVbiskSet *set,
                               QccListNode **list_node,
                               QccListNode **set_node)
{
  QccListNode *new_set_node;
  QccListNode *current_list_node;
  QccList *current_list;
  int level;

  if ((new_set_node = QccListCreateNode(sizeof(QccWAVbiskSet), set)) == NULL)
    {
      QccErrorAddMessage("(QccWAVbiskInsertSet): Error calling QccListCreateNode()");
      return(1);
    }

  current_list_node = LIS->start;

  for (level = set->level; (current_list_node != NULL) && level; level--)
    {
      if (current_list_node->next == NULL)
        if (QccWAVbiskLengthenLIS(LIS))
          {
            QccErrorAddMessage("(QccWAVbiskInsertSet): Error calling QccWAVbiskLengthenLIS()");
            return(1);
          }

      current_list_node = current_list_node->next;
    }

  current_list = (QccList *)(current_list_node->value);
  if (QccListAppendNode(current_list, new_set_node))
    {
      QccErrorAddMessage("(QccWAVbiskInsertSet): Error calling QccListAppendNode()");
      return(1);
    }

  if (set_node != NULL)
    *set_node = new_set_node;
  if (list_node != NULL)
    *list_node = current_list_node;

  return(0);
}


static int QccWAVbiskInitialization(QccList *LIS,
                                    QccList *LSP,
                                    const QccWAVSubbandPyramid *coefficients,
                                    const QccWAVSubbandPyramid *mask)
{
  QccWAVbiskSet set;
  int subband;
  int num_subbands;
  int return_value;

  num_subbands =
    QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels);

  if (QccWAVbiskLengthenLIS(LIS))
    {
      QccErrorAddMessage("(QccWAVbiskInitialization): Error calling QccWAVbiskLengthenLIS()");
      return(1);
    }

  for (subband = 0; subband < num_subbands; subband++)
    {
      set.level =
        QccWAVSubbandPyramidCalcLevelFromSubband(subband,
                                                 coefficients->num_levels) * 2;
      set.significance = QCCBISK_INSIGNIFICANT;
      return_value = QccWAVbiskSetSize(&set,
                                       coefficients,
                                       mask,
                                       subband);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVbiskInitialization): Error calling QccWAVbiskSetSize()");
          return(1);
        }
      
      if (return_value != 2)
        if (QccWAVbiskInsertSet(LIS,
                                &set,
                                NULL,
                                NULL))
          {
            QccErrorAddMessage("(QccWAVbiskInitialization): Error calling QccWAVbiskInsertSet()");
            return(1);
          }
    }

  QccListInitialize(LSP);

  return(0);
}


static void QccWAVbiskFreeLIS(QccList *LIS)
{
  QccListNode *current_list_node;
  QccList *current_list;

  current_list_node = LIS->start;
  while (current_list_node != NULL)
    {
      current_list = (QccList *)(current_list_node->value);
      QccListFree(current_list);
      current_list_node = current_list_node->next;
    }

  QccListFree(LIS);
}


static int QccWAVbiskSignificanceMap(const QccWAVSubbandPyramid *coefficients,
                                     const QccWAVSubbandPyramid *mask,
                                     char **significance_map,
                                     double threshold)
{
  int row, col;

  if (significance_map == NULL)
    return(0);

  for (row = 0; row < coefficients->num_rows; row++)
    for (col = 0; col < coefficients->num_cols; col++)
      if (!QccWAVbiskTransparent(mask, row, col))
        if (coefficients->matrix[row][col] >= threshold)
          significance_map[row][col] = QCCBISK_SIGNIFICANT;
        else
          significance_map[row][col] = QCCBISK_INSIGNIFICANT;
      else
        significance_map[row][col] = QCCBISK_INSIGNIFICANT;

  return(0);
}


static int QccWAVbiskSetSignificance(QccWAVbiskSet *set,
                                     char **significance_map)
{
  int row, col;
  
  set->significance = QCCBISK_INSIGNIFICANT;
  
  for (row = 0; row < set->num_rows; row++)
    for (col = 0; col < set->num_cols; col++)
      if (significance_map[set->origin_row + row]
          [set->origin_col + col] ==
          QCCBISK_SIGNIFICANT)
        {
          set->significance = QCCBISK_SIGNIFICANT;
          return(0);
        }
  
  return(0);
}


static int QccWAVbiskInputOutputSetSignificance(QccWAVbiskSet *current_set,
                                                char **significance_map,
                                                QccENTArithmeticModel *model,
                                                QccBitBuffer *buffer)
{
  int symbol;
  int return_value;

  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      if (model->current_context != QCCBISK_CONTEXT_NOCODE)
        {
          if (QccWAVbiskSetSignificance(current_set,
                                        significance_map))
            {
              QccErrorAddMessage("(QccWAVbiskInputOutputSetSignificance): Error calling QccWAVbiskSetSignificance()");
              return(1);
            }
          
          symbol = (current_set->significance == QCCBISK_SIGNIFICANT);
          
          return_value =
            QccENTArithmeticEncode(&symbol, 1,
                                   model, buffer);
          if (return_value == 2)
            return(2);
          else
            if (return_value)
              {
                QccErrorAddMessage("(QccWAVbiskInputOutputSetSignificance): Error calling QccENTArithmeticEncode()");
                return(1);
              }
        }
      else
        current_set->significance = QCCBISK_SIGNIFICANT;
    }
  else
    if (model->current_context != QCCBISK_CONTEXT_NOCODE)
      {
        if (QccENTArithmeticDecode(buffer,
                                   model,
                                   &symbol, 1))
          return(2);
        
        current_set->significance =
          (symbol) ? QCCBISK_SIGNIFICANT : QCCBISK_INSIGNIFICANT;
      }
    else
      current_set->significance = QCCBISK_SIGNIFICANT;
  
  return(0);
}


static int QccWAVbiskInputOutputSign(char *sign,
                                     double *coefficient,
                                     int row,
                                     int col,
                                     double threshold,
                                     QccENTArithmeticModel *model,
                                     QccBitBuffer *buffer,
                                     QccWAVbiskDistortionTrace
                                     *distortion_trace)
{
  int return_value;
  int symbol;
  
  model->current_context = QCCBISK_CONTEXT_SIGN;
  
  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      symbol = (*sign == QCCBISK_POSITIVE);
      *coefficient -= threshold;
      
      return_value =
        QccENTArithmeticEncode(&symbol, 1,
                               model, buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value)
          {
            QccErrorAddMessage("(QccWAVbiskInputOutputSign): Error calling QccENTArithmeticEncode()");
            return(1);
          }
    }
  else
    {
      if (QccENTArithmeticDecode(buffer,
                                 model,
                                 &symbol, 1))
        return(2);
      
      *sign =
        (symbol) ? QCCBISK_POSITIVE : QCCBISK_NEGATIVE;
      
      *coefficient  = 1.5 * threshold;
    }
  
  if (distortion_trace != NULL)
    {
      distortion_trace->reconstructed_coefficients.matrix[row][col] =
        1.5 * threshold;
      QccWAVbiskDistortionTraceUpdate(distortion_trace,
                                      row,
                                      col,
                                      buffer);
    }
  
  return(0);
}


static int QccWAVbiskPartitionSet(const QccWAVbiskSet *set,
                                  const QccWAVSubbandPyramid *mask,
                                  QccListNode **subset_node1,
                                  QccListNode **subset_node2)
{
  QccWAVbiskSet subset1;
  QccWAVbiskSet subset2;
  int split_row;
  int split_col;
  int num_rows;
  int num_cols;
  int return_value;

  split_row = set->num_rows / 2;
  split_col = set->num_cols / 2;
  
  if (set->level & 1)
    {
      num_cols = set->num_cols - split_col;
      
      /* Right */
      subset1.origin_row = set->origin_row;
      subset1.origin_col = set->origin_col + num_cols;
      subset1.num_rows = set->num_rows;
      subset1.num_cols = set->num_cols - num_cols;
      
      /* Left */
      subset2.origin_row = set->origin_row;
      subset2.origin_col = set->origin_col;
      subset2.num_rows = set->num_rows;
      subset2.num_cols = num_cols;
    }
  else
    {
      num_rows = set->num_rows - split_row;
      
      /* Bottom */
      subset1.origin_row = set->origin_row + num_rows;
      subset1.origin_col = set->origin_col;
      subset1.num_rows = set->num_rows - num_rows;
      subset1.num_cols = set->num_cols;
      
      /* Top */
      subset2.origin_row = set->origin_row;
      subset2.origin_col = set->origin_col;
      subset2.num_rows = num_rows;
      subset2.num_cols = set->num_cols;
    }

  subset1.level = set->level + 1;
  subset1.significance = QCCBISK_INSIGNIFICANT;

  subset2.level = set->level + 1;
  subset2.significance = QCCBISK_INSIGNIFICANT;

  return_value = QccWAVbiskSetShrink(&subset1, mask);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccWAVbiskPartitionSet): Error calling QccWAVbiskSetShrink()");
      return(1);
    }
  if (return_value == 2)
    subset1.significance = QCCBISK_EMPTYSET;

  return_value = QccWAVbiskSetShrink(&subset2, mask);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccWAVbiskPartitionSet): Error calling QccWAVbiskSetShrink()");
      return(1);
    }
  if (return_value == 2)
    subset2.significance = QCCBISK_EMPTYSET;

  if ((*subset_node1 =
       QccListCreateNode(sizeof(QccWAVbiskSet), &subset1)) == NULL)
    {
      QccErrorAddMessage("(QccWAVbiskPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if ((*subset_node2 =
       QccListCreateNode(sizeof(QccWAVbiskSet), &subset2)) == NULL)
    {
      QccErrorAddMessage("(QccWAVbiskPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }

  return(0);
}


static int QccWAVbiskProcessSet(QccListNode *current_set_node,
                                QccListNode *current_list_node,
                                QccList *LIS,
                                QccList *LSP,
                                QccList *garbage,
                                QccWAVSubbandPyramid *coefficients,
                                const QccWAVSubbandPyramid *mask,
                                char **significance_map,
                                char **sign_array,
                                double threshold,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer,
                                QccWAVbiskDistortionTrace
                                *distortion_trace);


static int QccWAVbiskCodeSet(QccListNode *current_set_node,
                             QccListNode *current_list_node,
                             QccList *LIS,
                             QccList *LSP,
                             QccList *garbage,
                             QccWAVSubbandPyramid *coefficients,
                             const QccWAVSubbandPyramid *mask,
                             char **significance_map,
                             char **sign_array,
                             double threshold,
                             QccENTArithmeticModel *model,
                             QccBitBuffer *buffer,
                             QccWAVbiskDistortionTrace
                             *distortion_trace)
{
  int return_value;
  QccWAVbiskSet *current_set = (QccWAVbiskSet *)(current_set_node->value);
  QccList *next_list;
  QccWAVbiskSet *subset1;
  QccListNode *subset_node1;
  QccListNode *subset_node2;
  
  if (current_list_node->next == NULL)
    if (QccWAVbiskLengthenLIS(LIS))
      {
        QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccWAVbiskLengthenLIS()");
        return(1);
      }
  next_list = (QccList *)(current_list_node->next->value);
  
  if (QccWAVbiskPartitionSet(current_set,
                             mask,
                             &subset_node1,
                             &subset_node2))
    {
      QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccWAVbiskPartitionSet()");
      return(1);
    }
  
  if (QccListAppendNode(garbage, subset_node1))
    {
      QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccListAppendNode()");
      return(1);
    }
  if (QccListAppendNode(garbage, subset_node2))
    {
      QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  subset1 = (QccWAVbiskSet *)(subset_node1->value);
  
  if (subset1->significance != QCCBISK_EMPTYSET)
    {
      if (QccListRemoveNode(garbage, subset_node1))
        {
          QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(next_list, subset_node1))
        {
          QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccListAppendNode()");
          return(1);
        }
      
      model->current_context = 
        QCCBISK_CONTEXT_SIGNIFICANCE_SUBSET1;
      
      return_value =
        QccWAVbiskProcessSet(subset_node1,
                             current_list_node->next,
                             LIS,
                             LSP,
                             garbage,
                             coefficients,
                             mask,
                             significance_map,
                             sign_array,
                             threshold,
                             model,
                             buffer,
                             distortion_trace);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccWAVbiskProcessSet()");
          return(1);
        }
      else
        if (return_value == 2)
          return(2);
    }
  
  if (QccListRemoveNode(garbage, subset_node2))
    {
      QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccListRemoveNode()");
      return(1);
    }
  if (QccListAppendNode(next_list, subset_node2))
    {
      QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  if ((subset1->significance == QCCBISK_INSIGNIFICANT) ||
      (subset1->significance == QCCBISK_EMPTYSET))
    model->current_context = QCCBISK_CONTEXT_NOCODE;
  else
    model->current_context = 
      QCCBISK_CONTEXT_SIGNIFICANCE_SUBSET2;
  
  return_value =
    QccWAVbiskProcessSet(subset_node2,
                         current_list_node->next,
                         LIS,
                         LSP,
                         garbage,
                         coefficients,
                         mask,
                         significance_map,
                         sign_array,
                         threshold,
                         model,
                         buffer,
                         distortion_trace);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccWAVbiskCodeSet): Error calling QccWAVbiskProcessSet()");
      return(1);
    }
  else
    if (return_value == 2)
      return(2);
  
  return(0);
}


static int QccWAVbiskProcessSet(QccListNode *current_set_node,
                                QccListNode *current_list_node,
                                QccList *LIS,
                                QccList *LSP,
                                QccList *garbage,
                                QccWAVSubbandPyramid *coefficients,
                                const QccWAVSubbandPyramid *mask,
                                char **significance_map,
                                char **sign_array,
                                double threshold,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer,
                                QccWAVbiskDistortionTrace
                                *distortion_trace)
{
  int return_value;
  QccWAVbiskSet *current_set = (QccWAVbiskSet *)(current_set_node->value);
  QccList *current_list = (QccList *)(current_list_node->value);
  
  if (current_set->significance == QCCBISK_EMPTYSET)
    {
      if (QccListRemoveNode(current_list, current_set_node))
        {
          QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(garbage, current_set_node))
        {
          QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccListAppendNode()");
          return(1);
        }
      
      return(0);
    }
  
  return_value = QccWAVbiskInputOutputSetSignificance(current_set,
                                                      significance_map,
                                                      model,
                                                      buffer);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccWAVbiskInputOutputSetSignificance()");
      return(1);
    }
  else
    if (return_value == 2)
      return(2);
  
  if (current_set->significance != QCCBISK_INSIGNIFICANT)
    {
      if (QccListRemoveNode(current_list, current_set_node))
        {
          QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(garbage, current_set_node))
        {
          QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccListAppendNode()");
          return(1);
        }
      
      if (QccWAVbiskSetIsPixel(current_set))
        {
          current_set->significance = QCCBISK_NEWLY_SIGNIFICANT;
          return_value =
            QccWAVbiskInputOutputSign(&sign_array
                                      [current_set->origin_row]
                                      [current_set->origin_col],
                                      &coefficients->matrix
                                      [current_set->origin_row]
                                      [current_set->origin_col],
                                      current_set->origin_row,
                                      current_set->origin_col,
                                      threshold,
                                      model,
                                      buffer,
                                      distortion_trace);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccWAVbiskInputOutputSign()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
          
          if (QccListRemoveNode(garbage, current_set_node))
            {
              QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccListRemoveNode()");
              return(1);
            }
          if (QccListAppendNode(LSP, current_set_node))
            {
              QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccListAppendNode()");
              return(1);
            }
        }
      else
        {
          return_value =
            QccWAVbiskCodeSet(current_set_node,
                              current_list_node,
                              LIS,
                              LSP,
                              garbage,
                              coefficients,
                              mask,
                              significance_map,
                              sign_array,
                              threshold,
                              model,
                              buffer,
                              distortion_trace);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVbiskProcessSet): Error calling QccWAVbiskCodeSet()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
        }
    }
  
  return(0);
}


static int QccWAVbiskSortingPass(QccWAVSubbandPyramid *coefficients,
                                 const QccWAVSubbandPyramid *mask,
                                 char **significance_map,
                                 char **sign_array,
                                 double threshold,
                                 QccList *LIS,
                                 QccList *LSP,
                                 QccENTArithmeticModel *model,
                                 QccBitBuffer *buffer,
                                 QccWAVbiskDistortionTrace *distortion_trace)
{
  int return_value;
  QccListNode *current_list_node;
  QccList *current_list;
  QccListNode *current_set_node;
  QccListNode *next_set_node;
  QccList garbage;

  QccListInitialize(&garbage);

  if (QccWAVbiskSignificanceMap(coefficients,
                                mask,
                                significance_map,
                                threshold))
    {
      QccErrorAddMessage("(QccWAVbiskSortingPass): Error calling QccWAVbiskSignificanceMap()");
      goto Error;
    }

  current_list_node = LIS->end;
  while (current_list_node != NULL)
    {
      current_list = (QccList *)(current_list_node->value);
      current_set_node = current_list->start;
      while (current_set_node != NULL)
        {
          next_set_node = current_set_node->next;

          model->current_context = QCCBISK_CONTEXT_SIGNIFICANCE;

          return_value = QccWAVbiskProcessSet(current_set_node,
                                              current_list_node,
                                              LIS,
                                              LSP,
                                              &garbage,
                                              coefficients,
                                              mask,
                                              significance_map,
                                              sign_array,
                                              threshold,
                                              model,
                                              buffer,
                                              distortion_trace);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVbiskSortingPass): Error calling QccWAVbiskProcessSet()");
              goto Error;
            }
          else
            if (return_value == 2)
              goto Finished;

          current_set_node = next_set_node;
        }

      current_list_node = current_list_node->previous;
    }

  return_value = 0;
  goto Return;
 Finished:
  return_value = 2;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccListFree(&garbage);
  return(return_value);
}


static void QccWAVbiskRefineCoefficient(double *coefficient,
                                        int bit,
                                        double threshold)
{
  *coefficient +=
    (bit) ? (threshold / 2) : (-threshold / 2);
}


static int QccWAVbiskRefinementInputOutput(double *coefficient,
                                           int row,
                                           int col,
                                           double threshold,
                                           QccENTArithmeticModel *model,
                                           QccBitBuffer *buffer,
                                           QccWAVbiskDistortionTrace
                                           *distortion_trace)
{
  int return_value;
  int symbol;
  
  model->current_context = QCCBISK_CONTEXT_REFINEMENT;
  
  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      if (*coefficient >= threshold)
        {
          symbol = 1;
          *coefficient -= threshold;
        }
      else
        symbol = 0;
      
      return_value =
        QccENTArithmeticEncode(&symbol, 1,
                               model, buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value)
          {
            QccErrorAddMessage("(QccWAVbiskRefinementInputOutput): Error calling QccENTArithmeticEncode()");
            return(1);
          }
    }
  else
    {
      if (QccENTArithmeticDecode(buffer,
                                 model,
                                 &symbol, 1))
        return(2);
      
      QccWAVbiskRefineCoefficient(coefficient,
                                  symbol,
                                  threshold);
    }
  
  if (distortion_trace != NULL)
    {
      double *coefficient_value =
        &(distortion_trace->reconstructed_coefficients.matrix[row][col]);
      QccWAVbiskRefineCoefficient(coefficient_value,
                                  symbol,
                                  threshold);
      QccWAVbiskDistortionTraceUpdate(distortion_trace,
                                      row,
                                      col,
                                      buffer);
    }
  
  return(0);
}


static int QccWAVbiskRefinementPass(QccWAVSubbandPyramid *coefficients,
                                    QccList *LSP,
                                    double threshold,
                                    QccENTArithmeticModel *model,
                                    QccBitBuffer *buffer,
                                    QccWAVbiskDistortionTrace
                                    *distortion_trace)
{
  int return_value;
  QccListNode *current_set_node;
  QccWAVbiskSet *current_set;

  current_set_node = LSP->start;
  while (current_set_node != NULL)
    {
      current_set = (QccWAVbiskSet *)(current_set_node->value);
      
      if (current_set->significance == QCCBISK_NEWLY_SIGNIFICANT)
        current_set->significance = QCCBISK_SIGNIFICANT;
      else
        {
          return_value =
            QccWAVbiskRefinementInputOutput(&coefficients->matrix
                                            [current_set->origin_row]
                                            [current_set->origin_col],
                                            current_set->origin_row,
                                            current_set->origin_col,
                                            threshold,
                                            model,
                                            buffer,
                                            distortion_trace);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccWAVbiskRefinementPass): Error calling QccWAVbiskRefinementInputOutput()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
        }

      current_set_node = current_set_node->next;
    }

  return(0);
}


static int QccWAVbiskEncodeDWT(QccWAVSubbandPyramid *image_subband_pyramid,
                               const QccIMGImageComponent *image,
                               int num_levels,
                               double *image_mean,
                               QccWAVSubbandPyramid *mask_subband_pyramid,
                               const QccIMGImageComponent *mask,
                               const QccWAVWavelet *wavelet)
{
  int row, col;
  
  if (mask == NULL)
    {
      if (QccMatrixCopy(image_subband_pyramid->matrix, image->image,
                        image->num_rows, image->num_cols))
        {
          QccErrorAddMessage("(QccWAVbiskEncodeDWT): Error calling QccMatrixCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubtractMean(image_subband_pyramid,
                                           image_mean,
                                           NULL))
        {
          QccErrorAddMessage("(QccWAVbiskEncodeDWT): Error calling QccWAVSubbandPyramidSubtractMean()");
          return(1);
        }
    }
  else
    {
      if (QccMatrixCopy(mask_subband_pyramid->matrix, mask->image,
                        mask->num_rows, mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVbiskEncodeDWT): Error calling QccMatrixCopy()");
          return(1);
        }

      *image_mean = QccIMGImageComponentShapeAdaptiveMean(image, mask);

      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (QccAlphaTransparent(mask_subband_pyramid->matrix[row][col]))
            image_subband_pyramid->matrix[row][col] = 0;
          else
            image_subband_pyramid->matrix[row][col] =
              image->image[row][col] - *image_mean;
    }
  
  if (mask != NULL)
    {
      if (QccWAVSubbandPyramidShapeAdaptiveDWT(image_subband_pyramid,
                                               mask_subband_pyramid,
                                               num_levels,
                                               wavelet))
        {
          QccErrorAddMessage("(QccWAVbiskEncodeDWT): Error calling QccWAVSubbandPyramidShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramidDWT(image_subband_pyramid,
                                num_levels,
                                wavelet))
      {
        QccErrorAddMessage("(QccWAVbiskEncodeDWT): Error calling QccWAVSubbandPyramidDWT()");
        return(1);
      }
  
  return(0);
}


int QccWAVbiskEncodeHeader(QccBitBuffer *output_buffer, 
                           int num_levels, 
                           int num_rows, int num_cols,
                           double image_mean,
                           int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutChar(output_buffer, (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccWAVbiskEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVbiskEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVbiskEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVbiskEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVbiskEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccWAVbiskEncodeExtractSigns(QccWAVSubbandPyramid
                                        *image_subband_pyramid,
                                        QccWAVSubbandPyramid
                                        *mask_subband_pyramid,
                                        char **sign_array,
                                        int *max_coefficient_bits)
{
  double coefficient_magnitude;
  double max_coefficient = -MAXDOUBLE;
  int row, col;

  if (mask_subband_pyramid == NULL)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        {
          coefficient_magnitude =
            fabs(image_subband_pyramid->matrix[row][col]);
          if (image_subband_pyramid->matrix[row][col] != coefficient_magnitude)
            {
              image_subband_pyramid->matrix[row][col] = coefficient_magnitude;
              sign_array[row][col] = QCCBISK_NEGATIVE;
            }
          else
            sign_array[row][col] = QCCBISK_POSITIVE;
          if (coefficient_magnitude > max_coefficient)
            max_coefficient = coefficient_magnitude;
        }
  else
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (!QccAlphaTransparent(mask_subband_pyramid->matrix[row][col]))
          {
            coefficient_magnitude =
              fabs(image_subband_pyramid->matrix[row][col]);
            if (image_subband_pyramid->matrix[row][col] !=
                coefficient_magnitude)
              {
                image_subband_pyramid->matrix[row][col] =
                  coefficient_magnitude;
                sign_array[row][col] = QCCBISK_NEGATIVE;
              }
            else
              sign_array[row][col] = QCCBISK_POSITIVE;
            if (coefficient_magnitude > max_coefficient)
              max_coefficient = coefficient_magnitude;
          }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return(0);
}


static int QccWAVbiskDecodeApplySigns(QccWAVSubbandPyramid
                                      *image_subband_pyramid,
                                      QccWAVSubbandPyramid
                                      *mask_subband_pyramid,
                                      char **sign_array)
{
  int row, col;
  
  if (mask_subband_pyramid == NULL)
    {
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (sign_array[row][col])
            image_subband_pyramid->matrix[row][col] *= -1;
    }
  else
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if ((!QccAlphaTransparent(mask_subband_pyramid->matrix[row][col])) &&
            sign_array[row][col])
          image_subband_pyramid->matrix[row][col] *= -1;

  return(0);
}


int QccWAVbiskEncode2(QccWAVSubbandPyramid *image_subband_pyramid,
                      QccWAVSubbandPyramid *mask_subband_pyramid,
                      double image_mean,
                      int target_bit_cnt,
                      QccBitBuffer *output_buffer,
                      FILE *rate_distortion_file)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  char **sign_array = NULL;
  char **significance_map = NULL;
  int max_coefficient_bits;
  double threshold;
  int row, col;
  QccList LIS;
  QccList LSP;
  int bitplane = 0;
  QccWAVbiskDistortionTrace distortion_trace;

  if (image_subband_pyramid == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  
  QccListInitialize(&LIS);
  QccListInitialize(&LSP);

  if ((sign_array =
       (char **)malloc(sizeof(char *) * image_subband_pyramid->num_rows)) ==
      NULL)
    {
      QccErrorAddMessage("(QccWAVbiskEncode2): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image_subband_pyramid->num_cols)) ==
        NULL)
      {
        QccErrorAddMessage("(QccWAVbiskEncode2): Error allocating memory");
        goto Error;
      }
  if ((significance_map =
       (char **)malloc(sizeof(char *) * image_subband_pyramid->num_rows)) ==
      NULL)
    {
      QccErrorAddMessage("(QccWAVbiskEncode2): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    if ((significance_map[row] =
         (char *)malloc(sizeof(char) * image_subband_pyramid->num_cols)) ==
        NULL)
      {
        QccErrorAddMessage("(QccWAVbiskEncode2): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      significance_map[row][col] = QCCBISK_INSIGNIFICANT;
  
  if (QccWAVbiskEncodeExtractSigns(image_subband_pyramid,
                                   mask_subband_pyramid,
                                   sign_array,
                                   &max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVbiskEncode2): Error calling QccWAVbiskEncodeExtractSigns()");
      goto Error;
    }
  
  if (rate_distortion_file != NULL)
    QccWAVbiskDistortionTraceInitialize(&distortion_trace,
                                        image_subband_pyramid,
                                        mask_subband_pyramid,
                                        output_buffer,
                                        rate_distortion_file);

  if (QccWAVbiskEncodeHeader(output_buffer,
                             image_subband_pyramid->num_levels,
                             image_subband_pyramid->num_rows,
                             image_subband_pyramid->num_cols, 
                             image_mean,
                             max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVbiskEncode2): Error calling QccWAVbiskEncodeHeader()");
      goto Error;
    }
  
  if ((model = QccENTArithmeticEncodeStart(QccWAVbiskNumSymbols,
                                           QCCBISK_NUM_CONTEXTS,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVbiskEncode2): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  if (QccWAVbiskInitialization(&LIS,
                               &LSP,
                               image_subband_pyramid,
                               mask_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVbiskEncode2): Error calling QccWAVbiskInitialization()");
      goto Error;
    }
  
  while (bitplane < QCCBISK_MAXBITPLANES)
    {
      return_value = QccWAVbiskSortingPass(image_subband_pyramid,
                                           mask_subband_pyramid,
                                           significance_map,
                                           sign_array,
                                           threshold,
                                           &LIS,
                                           &LSP,
                                           model,
                                           output_buffer,
                                           ((rate_distortion_file != NULL) ?
                                            &distortion_trace : NULL));
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVbiskEncode2): Error calling QccWAVbiskSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      return_value = QccWAVbiskRefinementPass(image_subband_pyramid,
                                              &LSP,
                                              threshold,
                                              model,
                                              output_buffer,
                                              ((rate_distortion_file != NULL) ?
                                               &distortion_trace : NULL));
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVbiskEncode2): Error calling QccWAVbiskRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      threshold /= 2.0;
      bitplane++;
    }

 Finished:
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (sign_array != NULL)
    {
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  if (significance_map != NULL)
    {
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        if (significance_map[row] != NULL)
          QccFree(significance_map[row]);
      QccFree(significance_map);
    }
  QccENTArithmeticFreeModel(model);
  QccWAVbiskFreeLIS(&LIS);
  QccListFree(&LSP);
  return(return_value);
}


int QccWAVbiskEncode(const QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     QccBitBuffer *output_buffer,
                     FILE *rate_distortion_file)
{
  int return_value;
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  double image_mean;

  if (image == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);

  image_subband_pyramid.num_levels = 0;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVbiskEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVbiskEncode): Mask and image must be same size");
          goto Error;
        }

      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVbiskEncode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }
    }
  
  if (QccWAVbiskEncodeDWT(&image_subband_pyramid,
                          image,
                          num_levels,
                          &image_mean,
                          &mask_subband_pyramid,
                          mask,
                          wavelet))
    {
      QccErrorAddMessage("(QccWAVbiskEncode): Error calling QccWAVbiskEncodeDWT()");
      goto Error;
    }

  if (QccWAVbiskEncode2(&image_subband_pyramid,
                        (mask != NULL) ?
                        &mask_subband_pyramid : NULL,
                        image_mean,
                        target_bit_cnt,
                        output_buffer,
                        rate_distortion_file))
    {
      QccErrorAddMessage("(QccWAVbiskEncode): Error calling QccWAVbiskEncode2()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  QccWAVSubbandPyramidFree(&mask_subband_pyramid);
  return(return_value);
}


static int QccWAVbiskDecodeInverseDWT(QccWAVSubbandPyramid
                                      *image_subband_pyramid,
                                      QccWAVSubbandPyramid
                                      *mask_subband_pyramid,
                                      QccIMGImageComponent *image,
                                      double image_mean,
                                      const QccWAVWavelet *wavelet)
{
  int row, col;

  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramidInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                      mask_subband_pyramid,
                                                      wavelet))
        {
          QccErrorAddMessage("(QccWAVbiskDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramidInverseDWT(image_subband_pyramid,
                                       wavelet))
      {
        QccErrorAddMessage("(QccWAVbiskDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseDWT()");
        return(1);
      }
  
  if (QccWAVSubbandPyramidAddMean(image_subband_pyramid,
                                  image_mean))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeInverseDWT): Error calling QccWAVSubbandPyramidAddMean()");
      return(1);
    }
  
  if (mask_subband_pyramid != NULL)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (QccAlphaTransparent(mask_subband_pyramid->matrix[row][col]))
          image_subband_pyramid->matrix[row][col] = 0;

  if (QccMatrixCopy(image->image, image_subband_pyramid->matrix,
                    image->num_rows, image->num_cols))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeInverseDWT): Error calling QccMatrixCopy()");
      return(1);
    }
  
  return(0);
}


int QccWAVbiskDecodeHeader(QccBitBuffer *input_buffer, 
                           int *num_levels, 
                           int *num_rows, int *num_cols,
                           double *image_mean,
                           int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;

  if (QccBitBufferGetChar(input_buffer, &ch))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  *num_levels = (int)ch;
  
  if (QccBitBufferGetInt(input_buffer, num_rows))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_cols))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer, image_mean))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccWAVbiskDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVbiskDecode2(QccBitBuffer *input_buffer,
                      QccWAVSubbandPyramid *image_subband_pyramid,
                      QccWAVSubbandPyramid *mask_subband_pyramid,
                      int max_coefficient_bits,
                      int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  char **sign_array = NULL;
  double threshold;
  int row, col;
  QccList LIS;
  QccList LSP;

  if (image_subband_pyramid == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  
  QccListInitialize(&LIS);
  QccListInitialize(&LSP);
  
  if ((sign_array =
       (char **)malloc(sizeof(char *) * image_subband_pyramid->num_rows)) ==
      NULL)
    {
      QccErrorAddMessage("(QccWAVbiskDecode2): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image_subband_pyramid->num_cols)) ==
        NULL)
      {
        QccErrorAddMessage("(QccWAVbiskDecode2): Error allocating memory");
        goto Error;
      }
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      {
        image_subband_pyramid->matrix[row][col] = 0.0;
        sign_array[row][col] = QCCBISK_POSITIVE;
      }
  
  if ((model =
       QccENTArithmeticDecodeStart(input_buffer,
                                   QccWAVbiskNumSymbols,
                                   QCCBISK_NUM_CONTEXTS,
                                   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccWAVbiskDecode2): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }

  threshold = pow((double)2, (double)max_coefficient_bits);
  
  if (QccWAVbiskInitialization(&LIS,
                               &LSP,
                               image_subband_pyramid,
                               mask_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVbiskDecode2): Error calling QccWAVbiskInitialization()");
      goto Error;
    }

  while (1)
    {
      return_value = QccWAVbiskSortingPass(image_subband_pyramid,
                                           mask_subband_pyramid,
                                           NULL,
                                           sign_array,
                                           threshold,
                                           &LIS,
                                           &LSP,
                                           model,
                                           input_buffer,
                                           NULL);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVbiskDecode2): Error calling QccWAVbiskSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      return_value = QccWAVbiskRefinementPass(image_subband_pyramid,
                                              &LSP,
                                              threshold,
                                              model,
                                              input_buffer,
                                              NULL);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccWAVbiskDecode2): Error calling QccWAVbiskRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      threshold /= 2.0;
    }

 Finished:
  if (QccWAVbiskDecodeApplySigns(image_subband_pyramid,
                                 mask_subband_pyramid,
                                 sign_array))
    {
      QccErrorAddMessage("(QccWAVbiskDecode2): Error calling QccWAVbiskApplySigns()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (sign_array != NULL)
    {
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  QccENTArithmeticFreeModel(model);
  QccWAVbiskFreeLIS(&LIS);
  QccListFree(&LSP);
  return(return_value);
}


int QccWAVbiskDecode(QccBitBuffer *input_buffer,
                     QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     double image_mean,
                     int max_coefficient_bits,
                     int target_bit_cnt)
{
  int return_value;
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  QccWAVWavelet lazy_wavelet_transform;

  if (image == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);
  QccWAVWaveletInitialize(&lazy_wavelet_transform);
  
  image_subband_pyramid.num_levels = num_levels;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccWAVbiskDecode): Mask and image must be same size");
          goto Error;
        }

      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }

      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }

      if (QccMatrixCopy(mask_subband_pyramid.matrix,
                        mask->image,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccMatrixCopy()");
          goto Error;
        }

      if (QccWAVSubbandPyramidDWT(((mask != NULL) ?
                                   &mask_subband_pyramid : NULL),
                                  num_levels,
                                  &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccWAVSubbandPyramidDWT()");
          goto Error;
        }
    }
  

  if (QccWAVbiskDecode2(input_buffer,
                        &image_subband_pyramid,
                        (mask != NULL) ?
                        &mask_subband_pyramid : NULL,
                        max_coefficient_bits,
                        target_bit_cnt))
    {
      QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccWAVbiskDecode2()");
      goto Error;
    }
  
  if (QccWAVbiskDecodeInverseDWT(&image_subband_pyramid,
                                 ((mask != NULL) ?
                                  &mask_subband_pyramid : NULL),
                                 image,
                                 image_mean,
                                 wavelet))
    {
      QccErrorAddMessage("(QccWAVbiskDecode): Error calling QccWAVbiskDecodeInverseDWT()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  QccWAVSubbandPyramidFree(&mask_subband_pyramid);
  QccWAVWaveletFree(&lazy_wavelet_transform);
  return(return_value);
}
