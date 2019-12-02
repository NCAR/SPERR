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


#define QCCSPECK3D_BOUNDARY_VALUE 0

#define QCCSPECK3D_INSIGNIFICANT 0
#define QCCSPECK3D_SIGNIFICANT 1
#define QCCSPECK3D_NEWLY_SIGNIFICANT 2
#define QCCSPECK3D_EMPTYSET 3

#define QCCSPECK3D_POSITIVE 0
#define QCCSPECK3D_NEGATIVE 1

#define QCCSPECK3D_SIGNIFICANCEMASK 0x03
#define QCCSPECK3D_SIGNMASK 0x04

#define QCCSPECK3D_MAXBITPLANES 128

#define QCCSPECK3D_CONTEXT_NOCODE -1
#define QCCSPECK3D_CONTEXT_SIGNIFICANCE 0
#define QCCSPECK3D_CONTEXT_SIGNIFICANCE_S0 1
#define QCCSPECK3D_CONTEXT_SIGNIFICANCE_S1 2
#define QCCSPECK3D_CONTEXT_SIGNIFICANCE_S2 3
#define QCCSPECK3D_CONTEXT_SIGN 4
#define QCCSPECK3D_CONTEXT_REFINEMENT 5
#define QCCSPECK3D_NUM_CONTEXTS 6
static const int QccSPECK3DNumSymbols[] =
  { 2, 2, 2, 2, 2, 2 };


typedef struct
{
  char temporal_splits;
  char horizontal_splits;
  char vertical_splits;
  int origin_frame;
  int origin_row;
  int origin_col;
  int num_frames;
  int num_rows;
  int num_cols;
  char significance;
} QccSPECK3DSet;


static void QccSPECK3DPutSignificance(unsigned char *state,
                                      int significance)
{
  *state &= (0xff ^ QCCSPECK3D_SIGNIFICANCEMASK); 
  *state |= significance;
}


static int QccSPECK3DGetSignificance(unsigned char state)
{
  return(state & QCCSPECK3D_SIGNIFICANCEMASK);
}


static void QccSPECK3DPutSign(unsigned char *state, int sign_value)
{
  if (sign_value)
    *state |= QCCSPECK3D_SIGNMASK;
  else
    *state &= (0xff ^ QCCSPECK3D_SIGNMASK); 
}


static int QccSPECK3DGetSign(unsigned char state)
{
  return((state & QCCSPECK3D_SIGNMASK) == QCCSPECK3D_SIGNMASK);
}


static int QccSPECK3DTransparent(const QccWAVSubbandPyramid3D *mask,
                                 int frame, int row, int col)
{
  if (mask == NULL)
    return(0);
  return(QccAlphaTransparent(mask->volume[frame][row][col]));
}


static int QccSPECK3DSetTransparent(const QccSPECK3DSet *set,
                                    const QccWAVSubbandPyramid3D *mask)
{
  int frame, row, col;

  for (frame = 0; frame < set->num_frames; frame++)
    for (row = 0; row < set->num_rows; row++)
      for (col = 0; col < set->num_cols; col++)
        if (!QccSPECK3DTransparent(mask,
                                   frame + set->origin_frame,
                                   row + set->origin_row,
                                   col + set->origin_col))
          return(0);
  
  return(1);
}


static int QccSPECK3DSetIsPixel(QccSPECK3DSet *set)
{
  if ((set->num_frames == 1) && (set->num_rows == 1) && (set->num_cols == 1))
    return(1);
  
  return(0);
}


#if 0
static void QccSPECK3DSetPrint(const QccSPECK3DSet *set)
{
  printf("<%c,%d,%d,%d,%d,%d,%d,%d,%d,%c>\n",
         ((set->type == QCCSPECK3D_set) ? 'S' : 'I'),
         set->temporal_splits,
         set->horizontal_splits,
         set->vertical_splits,
         set->origin_frame,
         set->origin_row,
         set->origin_col,
         set->num_frames,
         set->num_rows,
         set->num_cols,
         ((set->significance == QCCSPECK3D_INSIGNIFICANT) ? 'i' :
          ((set->significance == QCCSPECK3D_SIGNIFICANT) ? 's' : 'S')));
}


static void QccSPECK3DLISPrint(const QccList *LIS)
{
  QccListNode *current_list_node;
  QccList *current_list;
  QccListNode *current_set_node;
  QccSPECK3DSet *current_set;
  int temporal_splits, horizontal_splits, vertical_splits = 0;
  printf("LIS:\n");
  current_list_node = LIS->start;
  while (current_list_node != NULL)
    {
      printf("Temporal Splits %d:\n", temporal_splits++);
      printf("Horizontal Splits %d:\n", horizontal_splits++);
      printf("Vertical Splits %d:\n", vertical_splits++);
      current_list = (QccList *)(current_list_node->value);
      current_set_node = current_list->start;
      while (current_set_node != NULL)
        {
          current_set = (QccSPECK3DSet *)(current_set_node->value);
          QccSPECK3DSetPrint(current_set);
          current_set_node = current_set_node->next;
        }
      current_list_node = current_list_node->next;
    }
  printf("===============================================\n");
  fflush(stdout);
}


static void QccSPECK3DLSPPrint(const QccList *LSP)
{
  QccListNode *current_set_node;
  QccSPECK3DSet *current_set;
  printf("LSP:\n");
  
  current_set_node = LSP->start;
  while (current_set_node != NULL)
    {
      current_set = (QccSPECK3DSet *)(current_set_node->value);
      QccSPECK3DSetPrint(current_set);
      current_set_node = current_set_node->next;
    }
  printf("===============================================\n");
  fflush(stdout);
}
#endif


static int QccSPECK3DSetShrink(QccSPECK3DSet *set,
                               const QccWAVSubbandPyramid3D *mask)
{
  int row, col, frame;
  int min_frame = MAXINT;
  int min_row = MAXINT;
  int min_col = MAXINT;
  int max_frame = -MAXINT;
  int max_row = -MAXINT;
  int max_col = -MAXINT;
  int totally_transparent = 1;
  
  if (mask == NULL)
    return(0);
  
  for (frame = 0; frame < set->num_frames; frame++)
    for (row = 0; row < set->num_rows; row++)
      for (col = 0; col < set->num_cols; col++)
        if (!QccSPECK3DTransparent(mask,
                                   frame + set->origin_frame,
                                   row + set->origin_row,
                                   col + set->origin_col))
          {
            totally_transparent = 0;
            max_frame = QccMathMax(max_frame, frame + set->origin_frame);
            max_row = QccMathMax(max_row, row + set->origin_row);
            max_col = QccMathMax(max_col, col + set->origin_col);
            min_frame = QccMathMin(min_frame, frame + set->origin_frame);
            min_row = QccMathMin(min_row, row + set->origin_row);
            min_col = QccMathMin(min_col, col + set->origin_col);
          }
  
  if (totally_transparent)
    return(2);
  
  set->origin_frame = min_frame;
  set->origin_row = min_row;
  set->origin_col = min_col;
  set->num_frames = max_frame - min_frame + 1;
  set->num_rows = max_row - min_row + 1;
  set->num_cols = max_col - min_col + 1;
  
  return(0);
}


static int QccSPECK3DSetSize(QccSPECK3DSet *set,
                             const QccWAVSubbandPyramid3D *coefficients,
                             const QccWAVSubbandPyramid3D *mask,
                             int subband)
{
  int return_value;
  
  if (QccWAVSubbandPyramid3DSubbandOffsets(coefficients,
                                           subband,
                                           &set->origin_frame,
                                           &set->origin_row,
                                           &set->origin_col))
    {
      QccErrorAddMessage("(QccSPECK3DSetSize): Error calling QccWAVSubbandPyramid3DSubbandOffsets()");
      return(1);
    }
  if (QccWAVSubbandPyramid3DSubbandSize(coefficients,
                                        subband,
                                        &set->num_frames,
                                        &set->num_rows,
                                        &set->num_cols))
    {
      QccErrorAddMessage("(QccSPECK3DSetSize): Error calling QccWAVSubbandPyramid3DSubbandSize()");
      return(1);
    }
  
  return_value = QccSPECK3DSetShrink(set, mask);
  if (return_value == 2)
    return(2);
  else
    if (return_value == 1)
      {
        QccErrorAddMessage("(QccSPECK3DSetSize): Error calling QccSPECK3DSetShrink()");
        return(1);
      }
  
  return(0);
}


static int QccSPECK3DLengthenLIS(QccList *LIS)
{
  QccList new_list;
  QccListNode *new_list_node;
  
  QccListInitialize(&new_list);
  
  if ((new_list_node = QccListCreateNode(sizeof(QccList),
                                         &new_list)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DLengthenLIS): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(LIS, new_list_node))
    {
      QccErrorAddMessage("(QccSPECK3DLengthenLIS): Error calling QccListAppendNode()");
      return(1);
    }
  
  return(0);
}


static int QccSPECK3DInsertSet(QccList *LIS,
                               QccListNode *set_node,
                               QccListNode **list_node)
{
  QccSPECK3DSet *set = (QccSPECK3DSet *)(set_node->value);
  QccListNode *current_list_node;
  QccList *current_list;
  int splits;
  
  current_list_node = LIS->start;
  
  for (splits =
         set->temporal_splits + set->horizontal_splits + set->vertical_splits;
       splits >= 0;
       splits--)
    {
      if (current_list_node->next == NULL)
        if (QccSPECK3DLengthenLIS(LIS))
          {
            QccErrorAddMessage("(QccSPECK3DInsertSet): Error calling QccSPECK3DLengthenLIS()");
            return(1);
          }
      current_list_node = current_list_node->next;
    }
  
  current_list = (QccList *)(current_list_node->value);
  if (QccListAppendNode(current_list, set_node))
    {
      QccErrorAddMessage("(QccSPECK3DInsertSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  if (list_node != NULL)
    *list_node = current_list_node;
  
  return(0);
}


static int QccSPECK3DInitialization(QccList *LIS,
                                    QccList *LSP,
                                    const QccWAVSubbandPyramid3D
                                    *coefficients,
                                    const QccWAVSubbandPyramid3D *mask)
{
  QccSPECK3DSet set;
  QccListNode *set_node;
  int subband;
  int num_subbands;
  int return_value;
  int horizontal_splits = 0;
  int temporal_splits = 0;
  
  if (coefficients->transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(coefficients->spatial_num_levels);
  else
    num_subbands =
      QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(coefficients->temporal_num_levels,
                                                         coefficients->spatial_num_levels);
  
  if (QccSPECK3DLengthenLIS(LIS))
    {
      QccErrorAddMessage("(QccSPECK3DInitialization): Error calling QccSPECK3DLengthenLIS()");
      return(1);
    }
  
  for (subband = 0; subband < num_subbands; subband++)
    {
      if (coefficients->transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
        {
          if (QccWAVSubbandPyramid3DCalcLevelFromSubbandPacket(subband,
                                                               coefficients->temporal_num_levels,
                                                               coefficients->spatial_num_levels,
                                                               &temporal_splits,
                                                               &horizontal_splits))
            {
              QccErrorAddMessage("(QccSPECK3DInitialization): Error calling QccWAVSubbandPyramid3DCalcLevelFromSubbandPacket()");
              return(1);
            }
          set.temporal_splits = temporal_splits;
          set.horizontal_splits = horizontal_splits;
          set.vertical_splits = set.horizontal_splits;
        }
      else
        {
          set.horizontal_splits =
            QccWAVSubbandPyramid3DCalcLevelFromSubbandDyadic(subband,
                                                             coefficients->spatial_num_levels);
          set.temporal_splits = set.horizontal_splits;
          set.vertical_splits = set.horizontal_splits;
        }
      
      set.significance = QCCSPECK3D_INSIGNIFICANT;
      return_value = QccSPECK3DSetSize(&set,
                                       coefficients,
                                       mask,
                                       subband);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECK3DInitialization): Error calling QccSPECK3DSetSize()");
          return(1);
        }
      
      if (return_value != 2)
        {
          if ((set_node =
               QccListCreateNode(sizeof(QccSPECK3DSet), &set)) == NULL)
            {
              QccErrorAddMessage("(QccSPECK3DInitialization): Error calling QccListCreateNode()");
              return(1);
            }
          if (QccSPECK3DInsertSet(LIS,
                                  set_node,
                                  NULL))
            {
              QccErrorAddMessage("(QccSPECK3DInitialization): Error calling QccSPECK3DInsertSet()");
              return(1);
            }
        }
    }
  
  QccListInitialize(LSP);
  
  return(0);
}


static void QccSPECK3DFreeLIS(QccList *LIS)
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


static int QccSPECK3DSignificanceMap(const QccWAVSubbandPyramid3D
                                     *coefficients,
                                     const QccWAVSubbandPyramid3D *mask,
                                     unsigned char ***state_array,
                                     double threshold)
{
  int frame, row, col;
  
  if (state_array == NULL)
    return(0);
  
  for (frame = 0; frame < coefficients->num_frames; frame++)
    for (row = 0; row < coefficients->num_rows; row++)
      for (col = 0; col < coefficients->num_cols; col++)
        if (!QccSPECK3DTransparent(mask, frame, row, col))
          if (coefficients->volume[frame][row][col] >= threshold)
            QccSPECK3DPutSignificance(&state_array[frame][row][col],
                                      QCCSPECK3D_SIGNIFICANT);
          else
            QccSPECK3DPutSignificance(&state_array[frame][row][col],
                                      QCCSPECK3D_INSIGNIFICANT);
        else
          QccSPECK3DPutSignificance(&state_array[frame][row][col],
                                    QCCSPECK3D_INSIGNIFICANT);
  
  return(0);
}


static int QccSPECK3DSetSignificance(QccSPECK3DSet *set,
                                     unsigned char ***state_array)
{
  int frame, row, col;
  
  set->significance = QCCSPECK3D_INSIGNIFICANT;
  
  for (frame=0; frame < set->num_frames; frame++)
    for (row = 0; row < set->num_rows; row++)
      for (col = 0; col < set->num_cols; col++)
        {
          if (QccSPECK3DGetSignificance(state_array
                                        [set->origin_frame + frame]
                                        [set->origin_row + row]
                                        [set->origin_col + col]) ==
              QCCSPECK3D_SIGNIFICANT)
            {
              set->significance = QCCSPECK3D_SIGNIFICANT;
              return(0);
            }
        }
  
  return(0);
}


static int QccSPECK3DInputOutputSetSignificance(QccSPECK3DSet *current_set,
                                                unsigned char ***state_array,
                                                QccENTArithmeticModel *model,
                                                QccBitBuffer *buffer)
{
  int symbol;
  int return_value;
  
  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      if (model->current_context != QCCSPECK3D_CONTEXT_NOCODE)
        {
          if (QccSPECK3DSetSignificance(current_set,
                                        state_array))
            {
              QccErrorAddMessage("(QccSPECK3DInputOutputSetSignificance): Error calling QccSPECK3DSetSignificance()");
              return(1);
            }
          symbol = (current_set->significance == QCCSPECK3D_SIGNIFICANT);
          return_value =
            QccENTArithmeticEncode(&symbol, 1,
                                   model, buffer);
          if (return_value == 2)
            return(2);
          else
            if (return_value)
              {
                QccErrorAddMessage("(QccSPECK3DInputOutputSetSignificance): Error calling QccENTArithmeticEncode()");
                return(1);
              }
        }
      else
        current_set->significance = QCCSPECK3D_SIGNIFICANT;
    }
  else
    if (model->current_context != QCCSPECK3D_CONTEXT_NOCODE)
      {
        if (QccENTArithmeticDecode(buffer,
                                   model,
                                   &symbol, 1))
          return(2);
        current_set->significance =
          (symbol) ? QCCSPECK3D_SIGNIFICANT : QCCSPECK3D_INSIGNIFICANT;
      }
    else
      current_set->significance = QCCSPECK3D_SIGNIFICANT;
  
  return(0);
}


static int QccSPECK3DInputOutputSign(unsigned char *state,
                                     double *coefficient,
                                     double threshold,
                                     QccENTArithmeticModel *model,
                                     QccBitBuffer *buffer)
{
  int return_value;
  int symbol;
  
  model->current_context = QCCSPECK3D_CONTEXT_SIGN;
  if (buffer->type == QCCBITBUFFER_OUTPUT)
    {
      symbol = (QccSPECK3DGetSign(*state) == QCCSPECK3D_POSITIVE);
      *coefficient -= threshold;
      
      return_value =
        QccENTArithmeticEncode(&symbol, 1,
                               model, buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value)
          {
            QccErrorAddMessage("(QccSPECK3DInputOutputSign): Error calling QccENTArithmeticEncode()");
            return(1);
          }
    }
  else
    {
      if (QccENTArithmeticDecode(buffer,
                                 model,
                                 &symbol, 1))
        return(2);
      
      QccSPECK3DPutSign(state,
                        (symbol) ? QCCSPECK3D_POSITIVE : QCCSPECK3D_NEGATIVE);
      
      *coefficient = 1.5 * threshold;
    }
  
  return(0);
}


static int QccSPECK3DPartitionSet(const QccSPECK3DSet *set,
                                  const QccWAVSubbandPyramid3D *mask,
                                  QccList *subsets)
{
  QccSPECK3DSet new_set;
  QccListNode *new_set_node;
  int split_frame;
  int split_row;
  int split_col;
  int num_frames;
  int num_rows;
  int num_cols;
  
  split_frame = set->num_frames >> 1;
  split_row = set->num_rows >> 1;
  split_col = set->num_cols >> 1;
  
  num_frames = set->num_frames - split_frame;
  num_rows = set->num_rows - split_row;
  num_cols = set->num_cols - split_col;

  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame + num_frames;
  new_set.origin_row = set->origin_row + num_rows;
  new_set.origin_col = set->origin_col + num_cols;
  new_set.num_frames = set->num_frames - num_frames;
  new_set.num_rows = set->num_rows - num_rows;
  new_set.num_cols = set->num_cols - num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame;
  new_set.origin_row = set->origin_row + num_rows;
  new_set.origin_col = set->origin_col + num_cols;
  new_set.num_frames = num_frames;
  new_set.num_rows = set->num_rows - num_rows;
  new_set.num_cols = set->num_cols - num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame + num_frames;
  new_set.origin_row = set->origin_row;
  new_set.origin_col = set->origin_col + num_cols;
  new_set.num_frames = set->num_frames - num_frames;
  new_set.num_rows = num_rows;
  new_set.num_cols = set->num_cols - num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame + num_frames;
  new_set.origin_row = set->origin_row + num_rows;
  new_set.origin_col = set->origin_col;
  new_set.num_frames = set->num_frames - num_frames;
  new_set.num_rows = set->num_rows - num_rows;
  new_set.num_cols = num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame;
  new_set.origin_row = set->origin_row;
  new_set.origin_col = set->origin_col + num_cols;
  new_set.num_frames = num_frames;
  new_set.num_rows = num_rows;
  new_set.num_cols = set->num_cols - num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame + num_frames;
  new_set.origin_row = set->origin_row;
  new_set.origin_col = set->origin_col;
  new_set.num_frames = set->num_frames - num_frames;
  new_set.num_rows = num_rows;
  new_set.num_cols = num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame;
  new_set.origin_row = set->origin_row + num_rows;
  new_set.origin_col = set->origin_col;
  new_set.num_frames = num_frames;
  new_set.num_rows = set->num_rows - num_rows;
  new_set.num_cols = num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  new_set.vertical_splits = set->vertical_splits + 1;
  new_set.horizontal_splits = set->horizontal_splits + 1;
  new_set.temporal_splits = set->temporal_splits + 1;
  new_set.origin_frame = set->origin_frame;
  new_set.origin_row = set->origin_row;
  new_set.origin_col = set->origin_col;
  new_set.num_frames = num_frames;
  new_set.num_rows = num_rows;
  new_set.num_cols = num_cols;
  new_set.significance = QCCSPECK3D_INSIGNIFICANT;
  if (QccSPECK3DSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK3D_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECK3DSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECK3DPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  return(0);
}


static int QccSPECK3DProcessSet(QccListNode *current_set_node,
                                QccListNode *current_list_node,
                                QccList *LIS,
                                QccList *LSP,
                                QccList *garbage,
                                QccWAVSubbandPyramid3D *coefficients,
                                const QccWAVSubbandPyramid3D *mask,
                                unsigned char ***state_array,
                                double threshold,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer);


static int QccSPECK3DCodeSet(QccListNode *current_set_node,
                             QccListNode *current_list_node,
                             QccList *LIS,
                             QccList *LSP,
                             QccList *garbage,
                             QccWAVSubbandPyramid3D *coefficients,
                             const QccWAVSubbandPyramid3D *mask,
                             unsigned char ***state_array,
                             double threshold,
                             QccENTArithmeticModel *model,
                             QccBitBuffer *buffer)
{
  int return_value;
  QccSPECK3DSet *current_set = (QccSPECK3DSet *)(current_set_node->value);
  QccSPECK3DSet *current_subset = NULL;
  QccList subsets;
  QccListNode *current_subset_node;
  int subset_cnt = 0;
  int prior_significance = QCCSPECK3D_INSIGNIFICANT;
  int significance_cnt = 0;
  
  QccListInitialize(&subsets);

  if (QccSPECK3DPartitionSet(current_set,
                             mask,
                             &subsets))
    {
      QccErrorAddMessage("(QccSPECK3DCodeSet): Error calling QccSPECK3DPartitionSet()");
      goto Error;
    }
  
  current_subset_node = subsets.start;
  while (current_subset_node != NULL)
    {
      if (QccListRemoveNode(&subsets, current_subset_node))
        {
          QccErrorAddMessage("(QccSPECK3DCodeSet): Error calling QccListRemoveNode()");
          goto Error;
        }
      if (QccListAppendNode(garbage, current_subset_node))
        {
          QccErrorAddMessage("(QccSPECK3DCodeSet): Error calling QccListAppendNode()");
          goto Error;
        }
      
      current_subset = (QccSPECK3DSet *)(current_subset_node->value);
  
      if (current_subset->significance != QCCSPECK3D_EMPTYSET)
        {
          if (QccListRemoveNode(garbage, current_subset_node))
            {
              QccErrorAddMessage("(QccSPECK3DCodeSet): Error calling QccListRemoveNode()");
              goto Error;
            }
          if (QccSPECK3DInsertSet(LIS,
                                  current_subset_node,
                                  &current_list_node))
            {
              QccErrorAddMessage("(QccSPECK3DCodeSet): Error calling QccSPECK3DInsertSet()");
              goto Error;
            }
          
          if (prior_significance == QCCSPECK3D_INSIGNIFICANT)
            model->current_context =
              QCCSPECK3D_CONTEXT_SIGNIFICANCE_S0;
          else
            if (prior_significance == QCCSPECK3D_EMPTYSET)
              model->current_context =
                QCCSPECK3D_CONTEXT_SIGNIFICANCE_S1;
            else
              model->current_context =
                QCCSPECK3D_CONTEXT_SIGNIFICANCE_S2;
      
          if ((subset_cnt == 7) && !significance_cnt)
            model->current_context = QCCSPECK3D_CONTEXT_NOCODE;
          
          return_value =
            QccSPECK3DProcessSet(current_subset_node,
                                 current_list_node,
                                 LIS,
                                 LSP,
                                 garbage,
                                 coefficients,
                                 mask,
                                 state_array,
                                 threshold,
                                 model,
                                 buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECK3DCodeSet): Error calling QccSPECK3DProcessSet()");
              goto Error;
            }
          else
            if (return_value == 2)
              goto Finished;
        }

      prior_significance = current_subset->significance;
      if ((prior_significance != QCCSPECK3D_INSIGNIFICANT) &&
          (prior_significance != QCCSPECK3D_EMPTYSET))
        significance_cnt++;
      subset_cnt++;

      current_subset_node = subsets.start;
    }
      
  return_value = 0;
  goto Return;
 Finished:
  return_value = 2;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccListFree(&subsets);
  return(return_value);
}


static int QccSPECK3DProcessSet(QccListNode *current_set_node,
                                QccListNode *current_list_node,
                                QccList *LIS,
                                QccList *LSP,
                                QccList *garbage,
                                QccWAVSubbandPyramid3D *coefficients,
                                const QccWAVSubbandPyramid3D *mask,
                                unsigned char ***state_array,
                                double threshold,
                                QccENTArithmeticModel *model,
                                QccBitBuffer *buffer)
{
  int return_value;
  QccSPECK3DSet *current_set = (QccSPECK3DSet *)(current_set_node->value);
  QccList *current_list = (QccList *)(current_list_node->value);
  
  if (current_set->significance == QCCSPECK3D_EMPTYSET)
    {
      if (QccListRemoveNode(current_list, current_set_node))
        {
          QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(garbage, current_set_node))
        {
          QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccListAppendNode()");
          return(1);
        }
      
      return(0);
    }
  
  return_value = QccSPECK3DInputOutputSetSignificance(current_set,
                                                      state_array,
                                                      model,
                                                      buffer);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccSPECK3DInputOutputSetSignificance()");
      return(1);
    }
  else
    if (return_value == 2)
      return(2);
  
  if (current_set->significance != QCCSPECK3D_INSIGNIFICANT)
    {
      if (QccListRemoveNode(current_list, current_set_node))
        {
          QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(garbage, current_set_node))
        {
          QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccListAppendNode()");
          return(1);
        }
      
      if (QccSPECK3DSetIsPixel(current_set))
        {
          current_set->significance = QCCSPECK3D_NEWLY_SIGNIFICANT;
          return_value =
            QccSPECK3DInputOutputSign(&state_array
                                      [current_set->origin_frame]
                                      [current_set->origin_row]
                                      [current_set->origin_col],
                                      &coefficients->volume
                                      [current_set->origin_frame]
                                      [current_set->origin_row]
                                      [current_set->origin_col],
                                      threshold,
                                      model,
                                      buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccSPECK3DInputOutputSign()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
          
          if (QccListRemoveNode(garbage, current_set_node))
            {
              QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccListRemoveNode()");
              return(1);
            }
          if (QccListAppendNode(LSP, current_set_node))
            {
              QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccListAppendNode()");
              return(1);
            }
        }
      else
        {
          return_value =
            QccSPECK3DCodeSet(current_set_node,
                              current_list_node,
                              LIS,
                              LSP,
                              garbage,
                              coefficients,
                              mask,
                              state_array,
                              threshold,
                              model,
                              buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECK3DProcessSet): Error calling QccSPECK3DProcessSet()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
        }
    }
  
  return(0);
}


static int QccSPECK3DSortingPass(QccWAVSubbandPyramid3D *coefficients,
                                 const QccWAVSubbandPyramid3D *mask,
                                 unsigned char ***state_array,
                                 double threshold,
                                 QccList *LIS,
                                 QccList *LSP,
                                 QccENTArithmeticModel *model,
                                 QccBitBuffer *buffer)
{
  int return_value;
  QccListNode *current_list_node;
  QccList *current_list;
  QccListNode *current_set_node;
  QccListNode *next_set_node;
  QccList garbage;
  
  QccListInitialize(&garbage);
  
  if (QccSPECK3DSignificanceMap(coefficients,
                                mask,
                                state_array,
                                threshold))
    {
      QccErrorAddMessage("(QccSPECK3DSortingPass): Error calling QccSPECK3DSignificanceMap()");
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
          
          if (model != NULL)
            model->current_context = QCCSPECK3D_CONTEXT_SIGNIFICANCE;
          
          return_value = QccSPECK3DProcessSet(current_set_node,
                                              current_list_node,
                                              LIS,
                                              LSP,
                                              &garbage,
                                              coefficients,
                                              mask,
                                              state_array,
                                              threshold,
                                              model,
                                              buffer);
          
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECK3DSortingPass): Error calling QccSPECK3DProcessSet()");
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


static int QccSPECK3DRefinementInputOutput(double *coefficient,
                                           double threshold,
                                           QccENTArithmeticModel *model,
                                           QccBitBuffer *buffer)
{
  int return_value;
  int symbol;
  
  model->current_context = QCCSPECK3D_CONTEXT_REFINEMENT;
  
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
                               model,
                               buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value)
          {
            QccErrorAddMessage("(QccSPECK3DRefinementInputOutput): Error calling QccENTArithmeticEncode()");
            return(1);
          }
    }
  else
    {
      if (QccENTArithmeticDecode(buffer,
                                 model,
                                 &symbol, 1))
        return(2);
      
      *coefficient +=
        (symbol == 1) ? threshold / 2 : -threshold / 2;
    }
  
  return(0);
}


static int QccSPECK3DRefinementPass(QccWAVSubbandPyramid3D *coefficients,
                                    QccList *LSP,
                                    double threshold,
                                    QccENTArithmeticModel *model,
                                    QccBitBuffer *buffer)
{
  int return_value;
  QccListNode *current_set_node;
  QccSPECK3DSet *current_set;
  
  current_set_node = LSP->start;
  while (current_set_node != NULL)
    {
      current_set = (QccSPECK3DSet *)(current_set_node->value);
      
      if (current_set->significance == QCCSPECK3D_NEWLY_SIGNIFICANT)
        current_set->significance = QCCSPECK3D_SIGNIFICANT;
      else
        {
          return_value =
            QccSPECK3DRefinementInputOutput(&coefficients->volume
                                            [current_set->origin_frame]
                                            [current_set->origin_row]
                                            [current_set->origin_col],
                                            threshold,
                                            model,
                                            buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECK3DRefinementPass): Error calling QccSPECK3DRefinementInputOutput()");
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


int QccSPECK3DEncodeHeader(QccBitBuffer *output_buffer,
                           int transform_type,
                           int temporal_num_levels,
                           int spatial_num_levels,
                           int num_frames,
                           int num_rows,
                           int num_cols,
                           double image_mean,
                           int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutBit(output_buffer, transform_type))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPutBit()");
      goto Error;
    }
  
  if (transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferPutChar(output_buffer, (unsigned char)temporal_num_levels))
        {
          QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPuChar()");
          goto Error;
        }
    }
  
  if (QccBitBufferPutChar(output_buffer, (unsigned char)spatial_num_levels))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_frames))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, image_mean))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccSPECK3DEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


static int QccSPECK3DEncodeDWT(QccWAVSubbandPyramid3D *image_subband_pyramid,
                               unsigned char ***state_array,
                               const QccIMGImageCube *image_cube,
                               int transform_type,
                               int temporal_num_levels,
                               int spatial_num_levels,
                               double *image_mean,
                               int *max_coefficient_bits,
                               QccWAVSubbandPyramid3D *mask_subband_pyramid,
                               const QccIMGImageCube *mask,
                               const QccWAVWavelet *wavelet)
{
  double coefficient_magnitude;
  double max_coefficient = -MAXFLOAT;
  int frame, row, col;
  
  if (mask == NULL)
    {
      if (QccVolumeCopy(image_subband_pyramid->volume,
                        image_cube->volume,
                        image_cube->num_frames,
                        image_cube->num_rows,
                        image_cube->num_cols))
        {
          QccErrorAddMessage("(QccSPECK3DEncodeDWT): Error calling QccVolumeCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramid3DSubtractMean(image_subband_pyramid,
                                             image_mean,
                                             NULL))
        {
          QccErrorAddMessage("(QccSPECK3DEncodeDWT): Error calling QccWAVSubbandPyramid3DSubtractMean()");
          return(1);
        }
    }
  else
    {
      if (QccVolumeCopy(mask_subband_pyramid->volume,
                        mask->volume,
                        mask->num_frames,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccSPECK3DEncodeDWT): Error calling QccVolumeCopy()");
          return(1);
        }
      
      *image_mean = QccIMGImageCubeShapeAdaptiveMean(image_cube, mask);
      
      for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
        for (row = 0; row < image_subband_pyramid->num_rows; row++)
          for (col = 0; col < image_subband_pyramid->num_cols; col++)
            if (QccAlphaTransparent(mask_subband_pyramid->volume
                                    [frame][row][col]))
              image_subband_pyramid->volume[frame][row][col] = 0;
            else
              image_subband_pyramid->volume[frame][row][col] =
                image_cube->volume[frame][row][col] - *image_mean;
    }
  
  if (mask != NULL)
    {
      if (QccWAVSubbandPyramid3DShapeAdaptiveDWT(image_subband_pyramid,
                                                 mask_subband_pyramid,
                                                 transform_type,
                                                 temporal_num_levels,
                                                 spatial_num_levels,
                                                 wavelet))
        {
          QccErrorAddMessage("(QccSPECK3DEncodeDWT): Error calling QccWAVSubbandPyramid3DShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramid3DDWT(image_subband_pyramid,
                                  transform_type,
                                  temporal_num_levels,
                                  spatial_num_levels,
                                  wavelet))
      {
        QccErrorAddMessage("(QccSPECK3DEncodeDWT): Error calling QccWAVSubbandPyramid3DDWT()");
        return(1);
      }
  
  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        {
          coefficient_magnitude =
            fabs(image_subband_pyramid->volume[frame][row][col]);
          if (image_subband_pyramid->volume[frame][row][col] !=
              coefficient_magnitude)
            {
              image_subband_pyramid->volume[frame][row][col] =
                coefficient_magnitude;
              QccSPECK3DPutSign(&state_array[frame][row][col],
                                QCCSPECK3D_NEGATIVE);
            }
          else
            QccSPECK3DPutSign(&state_array[frame][row][col],
                              QCCSPECK3D_POSITIVE);
          if (coefficient_magnitude > max_coefficient)
            max_coefficient = coefficient_magnitude;
        }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return(0);
}


static int QccSPECK3DDecodeInverseDWT(QccWAVSubbandPyramid3D
                                      *image_subband_pyramid,
                                      QccWAVSubbandPyramid3D
                                      *mask_subband_pyramid,
                                      unsigned char ***state_array,
                                      QccIMGImageCube *image_cube,
                                      double image_mean,
                                      const QccWAVWavelet *wavelet)
{
  int frame, row, col;
  
  for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
    for (row = 0; row < image_subband_pyramid->num_rows; row++)
      for (col = 0; col < image_subband_pyramid->num_cols; col++)
        if (QccSPECK3DGetSign(state_array[frame][row][col]) ==
            QCCSPECK3D_NEGATIVE)
          image_subband_pyramid->volume[frame][row][col] *= -1;
  
  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                        mask_subband_pyramid,
                                                        wavelet))
        {
          QccErrorAddMessage("(QccSPECK3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramid3DInverseDWT(image_subband_pyramid,
                                         wavelet))
      {
        QccErrorAddMessage("(QccSPECK3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DInverseDWT()");
        return(1);
      }
  
  if (QccWAVSubbandPyramid3DAddMean(image_subband_pyramid,
                                    image_mean))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeInverseDWT): Error calling QccWAVSubbandPyramid3DAddMean()");
      return(1);
    }
  
  if (mask_subband_pyramid != NULL)
    for (frame = 0; frame < image_subband_pyramid->num_frames; frame++)
      for (row = 0; row < image_subband_pyramid->num_rows; row++)
        for (col = 0; col < image_subband_pyramid->num_cols; col++)
          if (QccAlphaTransparent(mask_subband_pyramid->volume
                                  [frame][row][col]))
            image_subband_pyramid->volume[frame][row][col] = 0;
  
  if (QccVolumeCopy(image_cube->volume,
                    image_subband_pyramid->volume,
                    image_cube->num_frames,
                    image_cube->num_rows,
                    image_cube->num_cols))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeInverseDWT): Error calling QccVolumeCopy()");
      return(1);
    }
  
  if (QccIMGImageCubeSetMaxMin(image_cube))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeInverseDWT): Error calling QccIMGImageCubeSetMaxMin()");
      return(1);
    }
  
  return(0);
}


int QccSPECK3DEncode(QccIMGImageCube *image_cube,
                     QccIMGImageCube *mask,
                     int transform_type,
                     int temporal_num_levels,
                     int spatial_num_levels,
                     const QccWAVWavelet *wavelet,
                     QccBitBuffer *output_buffer,
                     int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3D image_subband_pyramid;
  QccWAVSubbandPyramid3D mask_subband_pyramid;
  unsigned char ***state_array = NULL;
  double image_mean=0;
  int max_coefficient_bits;
  double threshold;
  int frame;
  int row, col;
  QccList LIS;
  QccList LSP;
  int bitplane = 0;
  
  if (image_cube == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccListInitialize(&LIS);
  QccListInitialize(&LSP);
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramid3DInitialize(&mask_subband_pyramid);
  
  image_subband_pyramid.temporal_num_levels = 0;
  image_subband_pyramid.spatial_num_levels = 0;
  image_subband_pyramid.num_frames = image_cube->num_frames;
  image_subband_pyramid.num_rows = image_cube->num_rows;
  image_subband_pyramid.num_cols = image_cube->num_cols;
  image_subband_pyramid.transform_type = transform_type;
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_frames != image_cube->num_frames) ||
          (mask->num_rows != image_cube->num_rows) ||
          (mask->num_cols != image_cube->num_cols))
        {
          QccErrorAddMessage("(QccSPECK3DEncode): Mask and image must be same size");
          goto Error;
        }
      
      mask_subband_pyramid.temporal_num_levels = 0;
      mask_subband_pyramid.spatial_num_levels = 0;
      mask_subband_pyramid.num_frames = mask->num_frames;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramid3DAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }
    }
  
  if ((state_array =
       (unsigned char ***)malloc(sizeof(unsigned char **) *
                                 (image_cube->num_frames))) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DEncode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < image_cube->num_frames; frame++)
    {
      if ((state_array[frame] =
           (unsigned char **)malloc(sizeof(unsigned char *) *
                                    (image_cube->num_rows))) == NULL)
        {
          QccErrorAddMessage("(QccSPECK3DEncode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < image_cube->num_rows; row++)
        if ((state_array[frame][row] =
             (unsigned char *)malloc(sizeof(unsigned char) *
                                     (image_cube->num_cols))) == NULL)
          {
            QccErrorAddMessage("(QccSPECK3DEncode): Error allocating memory");
            goto Error;
          }
    }
  
  for (frame = 0; frame < (image_cube->num_frames); frame++)
    for (row = 0; row < (image_cube->num_rows); row++)
      for (col = 0; col < (image_cube->num_cols); col++)
        state_array[frame][row][col] = 0;
  
  if (QccSPECK3DEncodeDWT(&image_subband_pyramid,
                          state_array,
                          image_cube,
                          transform_type,
                          temporal_num_levels,
                          spatial_num_levels,
                          &image_mean,
                          &max_coefficient_bits,
                          ((mask != NULL) ? &mask_subband_pyramid : NULL),
                          mask,
                          wavelet))
    {
      QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccSPECK3DEncodeDWT()");
      goto Error;
    }
  
  if (QccSPECK3DEncodeHeader(output_buffer,
                             transform_type,
                             temporal_num_levels,
                             spatial_num_levels,
                             image_cube->num_frames,
                             image_cube->num_rows,
                             image_cube->num_cols,
                             image_mean,
                             max_coefficient_bits))
    {
      QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccSPECK3DEncodeHeader()");
      goto Error;
    }
  
  
  if ((model = QccENTArithmeticEncodeStart(QccSPECK3DNumSymbols,
                                           QCCSPECK3D_NUM_CONTEXTS,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  if (QccSPECK3DInitialization(&LIS,
                               &LSP,
                               &image_subband_pyramid,
                               ((mask != NULL) ?
                                &mask_subband_pyramid : NULL)))
    {
      QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccSPECK3DInitialization()");
      goto Error;
    }
  
  while (bitplane < QCCSPECK3D_MAXBITPLANES)
    {
      return_value = QccSPECK3DSortingPass(&image_subband_pyramid,
                                           ((mask != NULL) ?
                                            &mask_subband_pyramid : NULL),
                                           state_array,
                                           threshold,
                                           &LIS,
                                           &LSP,
                                           model,
                                           output_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccSPECK3DSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;
      
      return_value = QccSPECK3DRefinementPass(&image_subband_pyramid,
                                              &LSP,
                                              threshold,
                                              model,
                                              output_buffer);
      
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECK3DEncode): Error calling QccWAVbiskRefinementPass()");
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
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccWAVSubbandPyramid3DFree(&mask_subband_pyramid);
  if (state_array != NULL)
    {
      for (frame = 0; frame < image_cube->num_frames; frame++)
        if (state_array[frame] != NULL)
          {
            for (row = 0; row < image_cube->num_rows; row++)
              if (state_array[frame][row] != NULL)
                free(state_array[frame][row]);
            free(state_array[frame]);
          }
      free(state_array);
    }
  QccENTArithmeticFreeModel(model);
  QccSPECK3DFreeLIS(&LIS);
  QccListFree(&LSP);
  return(return_value);
}


int QccSPECK3DDecodeHeader(QccBitBuffer *input_buffer,
                           int *transform_type,
                           int *temporal_num_levels,
                           int *spatial_num_levels,
                           int *num_frames,
                           int *num_rows,
                           int *num_cols,
                           double *image_mean,
                           int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;
  
  if (QccBitBufferGetBit(input_buffer, transform_type))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetBit()");
      goto Error;
    }
  
  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_PACKET)
    {
      if (QccBitBufferGetChar(input_buffer, &ch))
        {
          QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetChar()");
          goto Error;
        }
      *temporal_num_levels = (int)ch;
    }
  
  if (QccBitBufferGetChar(input_buffer, &ch))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetChar()");
      goto Error;
    }
  *spatial_num_levels = (int)ch;
  
  if (*transform_type == QCCWAVSUBBANDPYRAMID3D_DYADIC)
    *temporal_num_levels = *spatial_num_levels;

  if (QccBitBufferGetInt(input_buffer, num_frames))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_rows))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_cols))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer, image_mean))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccSPECK3DDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccSPECK3DDecode(QccBitBuffer *input_buffer,
                     QccIMGImageCube *image_cube,
                     QccIMGImageCube *mask,
                     int transform_type,
                     int temporal_num_levels,
                     int spatial_num_levels,
                     const QccWAVWavelet *wavelet,
                     double image_mean,
                     int max_coefficient_bits,
                     int target_bit_cnt)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid3D image_subband_pyramid;
  QccWAVSubbandPyramid3D mask_subband_pyramid;
  unsigned char ***state_array = NULL;
  double threshold;
  int frame;
  int row, col;
  QccWAVWavelet lazy_wavelet_transform;
  QccList LIS;
  QccList LSP;
  
  if (image_cube == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramid3DInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramid3DInitialize(&mask_subband_pyramid);
  QccListInitialize(&LIS);
  QccListInitialize(&LSP);
  QccWAVWaveletInitialize(&lazy_wavelet_transform);
  
  image_subband_pyramid.transform_type = transform_type;
  image_subband_pyramid.temporal_num_levels = temporal_num_levels;
  image_subband_pyramid.spatial_num_levels = spatial_num_levels;
  image_subband_pyramid.num_frames = image_cube->num_frames;
  image_subband_pyramid.num_rows = image_cube->num_rows;
  image_subband_pyramid.num_cols = image_cube->num_cols;
  if (QccWAVSubbandPyramid3DAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
    {
      if ((mask->num_frames != image_cube->num_frames) ||
          (mask->num_rows != image_cube->num_rows) ||
          (mask->num_cols != image_cube->num_cols))
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Mask and image cube must be same size");
          goto Error;
        }
      
      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }
      
      mask_subband_pyramid.temporal_num_levels = 0;
      mask_subband_pyramid.spatial_num_levels = 0;
      mask_subband_pyramid.num_frames = mask->num_frames;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramid3DAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccWAVSubbandPyramid3DAlloc()");
          goto Error;
        }
      
      if (QccVolumeCopy(mask_subband_pyramid.volume,
                        mask->volume,
                        mask->num_frames,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccVolumeCopy()");
          goto Error;
        }
      
      if (QccWAVSubbandPyramid3DDWT(&mask_subband_pyramid,
                                    transform_type,
                                    temporal_num_levels,
                                    spatial_num_levels,
                                    &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccWAVSubbandPyramid3DDWT()");
          goto Error;
        }
    }
  
  if ((state_array =
       (unsigned char ***)malloc(sizeof(unsigned char **) *
                                 (image_cube->num_frames))) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DDecode): Error allocating memory");
      goto Error;
    }
  for (frame = 0; frame < (image_cube->num_frames); frame++)
    {
      if ((state_array[frame] =
           (unsigned char **)malloc(sizeof(unsigned char *) *
                                    (image_cube->num_rows))) == NULL)
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error allocating memory");
          goto Error;
        }
      for (row = 0; row < (image_cube->num_rows); row++)
        if ((state_array[frame][row] =
             (unsigned char *)malloc(sizeof(unsigned char) *
                                     (image_cube->num_cols))) == NULL)
          {
            QccErrorAddMessage("(QccSPECK3DDecode): Error allocating memory");
            goto Error;
          }
    }
  for (frame = 0; frame < image_subband_pyramid.num_frames; frame++)
    for (row = 0; row < image_subband_pyramid.num_rows; row++)
      for (col = 0; col < image_subband_pyramid.num_cols; col++)
        {
          image_subband_pyramid.volume[frame][row][col] = 0.0;
          state_array[frame][row][col] = 0;
        }
  
  if ((model = QccENTArithmeticDecodeStart(input_buffer,
                                           QccSPECK3DNumSymbols,
                                           QCCSPECK3D_NUM_CONTEXTS,
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }
  
  threshold = pow((double)2, (double)max_coefficient_bits);
  
  if (QccSPECK3DInitialization(&LIS,
                               &LSP,
                               &image_subband_pyramid,
                               ((mask != NULL) ?
                                &mask_subband_pyramid : NULL)))
    {
      QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccSPECK3DInitialization()");
      goto Error;
    }
  
  while (1)
    {
      return_value = QccSPECK3DSortingPass(&image_subband_pyramid,
                                           ((mask != NULL) ?
                                            &mask_subband_pyramid : NULL),
                                           state_array,
                                           threshold,
                                           &LIS,
                                           &LSP,
                                           model,
                                           input_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccSPECK3DSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;
      
      return_value = QccSPECK3DRefinementPass(&image_subband_pyramid,
                                              &LSP,
                                              threshold,
                                              model,
                                              input_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccSPECK3DRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;
      
      threshold /= 2.0;
    }
  
 Finished:
  if (QccSPECK3DDecodeInverseDWT(&image_subband_pyramid,
                                 ((mask != NULL) ?
                                  &mask_subband_pyramid : NULL),
                                 state_array,
                                 image_cube,
                                 image_mean,
                                 wavelet))
    {
      QccErrorAddMessage("(QccSPECK3DDecode): Error calling QccWAVDecodeInverseDWT()");
      goto Error;
    }
  
  return_value = 0;
  QccErrorClearMessages();
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVSubbandPyramid3DFree(&image_subband_pyramid);
  QccWAVSubbandPyramid3DFree(&mask_subband_pyramid);
  if (state_array != NULL)
    {
      for (frame = 0; frame < (image_cube->num_frames); frame++)
        if (state_array[frame] != NULL)
          {
            for (row = 0; row < (image_cube->num_rows); row++)
              if (state_array[frame][row] != NULL)
                free(state_array[frame][row]);
            free(state_array[frame]);
          }
      free(state_array);
    }
  QccENTArithmeticFreeModel(model);
  QccSPECK3DFreeLIS(&LIS);
  QccListFree(&LSP);
  QccWAVWaveletFree(&lazy_wavelet_transform);
  return(return_value);
}
