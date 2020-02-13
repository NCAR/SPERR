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


/*
 * ----------------------------------------------------------------------------
 * 
 * Public License for the SPECK Algorithm
 * Version 1.1, March 8, 2004
 * 
 * ----------------------------------------------------------------------------
 * 
 * The Set-Partitioning Embedded Block (SPECK) algorithm is protected by US
 * Patent #6,671,413 (issued December 30, 2003) and by patents pending.
 * An implementation of the SPECK algorithm is included herein with the
 * gracious permission of Dr. William A. Pearlman, exclusive holder of patent
 * rights. Dr. Pearlman has granted the following license governing the terms
 * and conditions for use, copying, distribution, and modification of the
 * SPECK algorithm implementation contained herein (hereafter referred to as
 * "the SPECK source code").
 * 
 * 0. Use of the SPECK source code, including any executable-program or
 *    linkable-library form resulting from its compilation, is restricted to
 *    solely academic or non-commercial research activities.
 * 
 * 1. Any other use, including, but not limited to, use in the development
 *    of a commercial product, use in a commercial application, or commercial
 *    distribution, is prohibited by this license. Such acts require a separate
 *    license directly from Dr. Pearlman.
 * 
 * 2. For academic and non-commercial purposes, this license does not restrict
 *    use; copying, distribution, and modification are permitted under the
 *    terms of the GNU General Public License as published by the Free Software
 *    Foundation, with the further restriction that the terms of the present
 *    license shall also apply to all subsequent copies, distributions,
 *    or modifications of the SPECK source code.
 * 
 * NO WARRANTY
 * 
 * 3. Dr. Pearlman dislaims all warranties, expressed or implied, including
 *    without limitation any warranty whatsoever as to the fitness for a
 *    particular use or the merchantability of the SPECK source code.
 * 
 * 4. In no event shall Dr. Pearlman be liable for any loss of profits, loss
 *    of business, loss of use or loss of data, nor for indirect, special,
 *    incidental or consequential damages of any kind related to use of the
 *    SPECK source code.
 * 
 * 
 * END OF TERMS AND CONDITIONS
 * 
 * 
 * Persons desiring to license the SPECK algorithm for commercial purposes or
 * for uses otherwise prohibited by this license may wish to contact
 * Dr. Pearlman regarding the possibility of negotiating such licenses:
 * 
 *   Dr. William A. Pearlman
 *   Dept. of Electrical, Computer, and Systems Engineering
 *   Rensselaer Polytechnic Institute
 *   Troy, NY 12180-3590
 *   U.S.A.
 *   email: pearlman@ecse.rpi.edu
 *   tel.: (518) 276-6082
 *   fax: (518) 276-6261
 *  
 * ----------------------------------------------------------------------------
 */


#include "libQccPack.h"


#define QCCSPECK_BOUNDARY_VALUE 0

#define QCCSPECK_INSIGNIFICANT 0
#define QCCSPECK_SIGNIFICANT 1
#define QCCSPECK_NEWLY_SIGNIFICANT 2
#define QCCSPECK_EMPTYSET 3

#define QCCSPECK_POSITIVE 0
#define QCCSPECK_NEGATIVE 1

#define QCCSPECK_SET_S 0
#define QCCSPECK_SET_I 1

#define QCCSPECK_MAXBITPLANES 128

#define QCCSPECK_CONTEXT_NOCODE -1
#define QCCSPECK_CONTEXT_SIGNIFICANCE 0
#define QCCSPECK_CONTEXT_SIGNIFICANCE_S0 1
#define QCCSPECK_CONTEXT_SIGNIFICANCE_S1 4
#define QCCSPECK_CONTEXT_SIGNIFICANCE_S2 7
#define QCCSPECK_CONTEXT_SIGNIFICANCE_S3 10
#define QCCSPECK_CONTEXT_SIGN 11
#define QCCSPECK_CONTEXT_REFINEMENT 12
#define QCCSPECK_CONTEXT_I 13
#define QCCSPECK_NUM_CONTEXTS 14
static const int QccSPECKNumSymbols[] =
  { 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2 };


#define QccSPECKTransparent(mask, row, col) \
(((mask) == NULL) ? 0 : QccAlphaTransparent((mask)->matrix[(row)][(col)]))


typedef struct
{
  char type;
  int level;
  int origin_row;
  int origin_col;
  int num_rows;
  int num_cols;
  char significance;
} QccSPECKSet;


static int QccSPECKSetIsPixel(QccSPECKSet *set)
{
  if ((set->num_rows == 1) && (set->num_cols == 1))
    return(1);
  
  return(0);
}


#if 0
static void QccSPECKSetPrint(const QccSPECKSet *set)
{
  printf("<%c%d,%d,%d,%d,%d,%c>\n",
         ((set->type == QCCSPECK_SET_S) ? 'S' : 'I'),
         set->level,
         set->origin_row,
         set->origin_col,
         set->num_rows,
         set->num_cols,
         ((set->significance == QCCSPECK_INSIGNIFICANT) ? 'i' :
          ((set->significance == QCCSPECK_SIGNIFICANT) ? 's' : 'S')));
}


static void QccSPECKLISPrint(const QccList *LIS)
{
  QccListNode *current_list_node;
  QccList *current_list;
  QccListNode *current_set_node;
  QccSPECKSet *current_set;
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
          current_set = (QccSPECKSet *)(current_set_node->value);
          QccSPECKSetPrint(current_set);
          current_set_node = current_set_node->next;
        }

      current_list_node = current_list_node->next;
    }

  printf("===============================================\n");

  fflush(stdout);
}


static void QccSPECKLSPPrint(const QccList *LSP)
{
  QccListNode *current_set_node;
  QccSPECKSet *current_set;

  printf("LSP:\n");
  
  current_set_node = LSP->start;
  while (current_set_node != NULL)
    {
      current_set = (QccSPECKSet *)(current_set_node->value);
      QccSPECKSetPrint(current_set);
      current_set_node = current_set_node->next;
    }

  printf("===============================================\n");

  fflush(stdout);
}
#endif


static int QccSPECKSetTransparent(const QccSPECKSet *set,
                                  const QccWAVSubbandPyramid *mask)
{
  int row, col;

  for (row = 0; row < set->num_rows; row++)
    for (col = 0; col < set->num_cols; col++)
      if (!QccSPECKTransparent(mask,
                               row + set->origin_row,
                               col + set->origin_col))
        return(0);

  return(1);
}


static int QccSPECKSetSize(QccSPECKSet *set,
                           const QccWAVSubbandPyramid *coefficients,
                           int subband) /* specifies which chunk (0, 1, 2, 3), though 
                                           its mechanism seems quite complex. */
{
  if (QccWAVSubbandPyramidSubbandOffsets(coefficients,      /* input */
                                         subband,           /* input */
                                         &set->origin_row,  /* output */
                                         &set->origin_col)) /* output */
    {
      QccErrorAddMessage("(QccSPECKSetSize): Error calling QccWAVSubbandPyramidSubbandOffsets()");
      return(1);
    }
  if (QccWAVSubbandPyramidSubbandSize(coefficients,         /* input */  
                                      subband,              /* input */
                                      &set->num_rows,       /* output */
                                      &set->num_cols))      /* output */
    {
      QccErrorAddMessage("(QccSPECKSetSize): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }
  
  return(0);
}


static int QccSPECKInsertSet(QccList *LIS,
                             QccSPECKSet *set,
                             QccListNode **list_node,   /* return the newly created node */
                             QccListNode **set_node)    /* return the list that's holding 
                                                           the newly created node */
{
  QccListNode *new_set_node;
  QccListNode *current_list_node;
  QccList *current_list;
  int level;

  if ((new_set_node = QccListCreateNode(sizeof(QccSPECKSet), set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKInsertSet): Error calling QccListCreateNode()");
      return(1);
    }

  current_list_node = LIS->start;

  /* Sets are inserted to according lists of LIS based on level.
     current_list_node != NULL is always true in a normal situation */
  for (level = set->level; (current_list_node != NULL) && level; level--)
    current_list_node = current_list_node->next;

  current_list = (QccList *)(current_list_node->value);
  if (QccListAppendNode(current_list, new_set_node))
    {
      QccErrorAddMessage("(QccSPECKInsertSet): Error calling QccListAppendNode()");
      return(1);
    }

  if (set_node != NULL)
    *set_node = new_set_node;
  if (list_node != NULL)
    *list_node = current_list_node;

  return(0);
}


static int QccSPECKInitialization(QccList *LIS,
                                  QccList *LSP,
                                  QccSPECKSet *I,
                                  const QccWAVSubbandPyramid *coefficients,
                                  const QccWAVSubbandPyramid *mask)
{
  QccSPECKSet   set_S;  /* A QccSPECKSet keeps indices of elements in this set, 
                           but not the element values.                       */
  QccList       new_list;
  QccListNode*  new_list_node;
  int return_value;

  int num_rows = coefficients->num_rows * 2;
  int num_cols = coefficients->num_cols * 2;

  /* Get ready LIS: Each node is an individual empty list */
  while ((num_rows > 1) || (num_cols > 1))
    {
      QccListInitialize(&new_list);
      if ((new_list_node = QccListCreateNode(sizeof(QccList),     /* size of the value */
                                             &new_list)) == NULL) /* node value to be copied */
        {
          QccErrorAddMessage("(QccSPECKInitialization): Error calling QccListCreateNode()");
          return(1);
        }
      if (QccListAppendNode(LIS, new_list_node))
        {
          QccErrorAddMessage("(QccSPECKInitialization): Error calling QccListAppendNode()");
          return(1);
        }

      num_rows = num_rows - (num_rows >> 1);
      num_cols = num_cols - (num_cols >> 1);
    }
    /* Sam note: this while loop is to pre-allocate spaces for sets with various sizes. 
                 That's why each new list is empty, and future sets will be inserted
                 into corresponding lists.                                           */

  set_S.type         = QCCSPECK_SET_S;
  set_S.level        = coefficients->num_levels; /* Sam's level == qcc's level - 1 */
  set_S.significance = QCCSPECK_INSIGNIFICANT;
  /* Determine other fields of set_S that regard to sizes and offsets */
  return_value = QccSPECKSetSize(&set_S,
                                 coefficients,
                                 0);        /* 0 means the top most band */
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccSPECKInitialization): Error calling QccSPECKSetSize()");
      return(1);
    }

  /* Is set_S transparent according to mask? */
  if (!QccSPECKSetTransparent(&set_S, mask))
  {
    /* Insert set_S to the appropriate slot of LIS */
    if (QccSPECKInsertSet(LIS,
                          &set_S,
                          NULL,
                          NULL))
      {
        QccErrorAddMessage("(QccSPECKInitialization): Error calling QccSPECKInsertSet()");
        return(1);
      }
  }

  QccListInitialize(LSP);   /* initialize as an empty list */

  I->type = QCCSPECK_SET_I;
  I->level = coefficients->num_levels; /* Sam's level == qcc's level - 1 */
  I->origin_row = set_S.num_rows;
  I->origin_col = set_S.num_cols;
  I->num_rows = coefficients->num_rows;
  I->num_cols = coefficients->num_cols;
  I->significance = QCCSPECK_INSIGNIFICANT;

  return(0);
}


static void QccSPECKFreeLIS(QccList *LIS)
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


static int QccSPECKSignificanceMap(const QccWAVSubbandPyramid *coefficients,
                                   const QccWAVSubbandPyramid *mask,
                                   char **significance_map,
                                   double threshold)
{
  int row, col;

  if (significance_map == NULL)
    return(0);

  for (row = 0; row < coefficients->num_rows; row++)
  {
    for (col = 0; col < coefficients->num_cols; col++)
    {
      if (!QccSPECKTransparent(mask, row, col))
      {
        if (coefficients->matrix[row][col] >= threshold)
          significance_map[row][col] = QCCSPECK_SIGNIFICANT;
        else
          significance_map[row][col] = QCCSPECK_INSIGNIFICANT;
      }
      else
        significance_map[row][col] = QCCSPECK_INSIGNIFICANT;
    }
  }

  return(0);
}


static int QccSPECKSetSignificance(QccSPECKSet *set,
                                   char **significance_map)
{
  int row, col;
  
  set->significance = QCCSPECK_INSIGNIFICANT;
  
  if (set->type == QCCSPECK_SET_S)
  {  
      for (row = 0; row < set->num_rows; row++)
        for (col = 0; col < set->num_cols; col++)
          if (significance_map[set->origin_row + row][set->origin_col + col] == QCCSPECK_SIGNIFICANT)
          {
              set->significance = QCCSPECK_SIGNIFICANT;
              return(0);
          }
  }
  else  /* type I */
  {
      for (row = 0; row < set->num_rows; row++)
        for (col = 0; col < set->num_cols; col++)
          if (((row >= set->origin_row) || (col >= set->origin_col)) && (significance_map[row][col] ==
               QCCSPECK_SIGNIFICANT))
          {
              set->significance = QCCSPECK_SIGNIFICANT;
              return(0);
          }
  }
  
  return(0);
}


static int QccSPECKInputOutputSetSignificance(QccSPECKSet *current_set,
                                              char **significance_map,
                                              QccENTArithmeticModel *model,
                                              QccBitBuffer *buffer)
{
  int symbol;
  int return_value;

  if (buffer->type == QCCBITBUFFER_OUTPUT)
  {
      if (model->current_context != QCCSPECK_CONTEXT_NOCODE)
      {
          /* Determine if the current set is significant based on significance_map.
             The result is stored in current_set. */
          if (QccSPECKSetSignificance(current_set, significance_map))
          {
              QccErrorAddMessage("(QccSPECKInputOutputSetSignificance): Error calling QccSPECKSetSignificance()");
              return(1);
          }
          
          symbol = (current_set->significance == QCCSPECK_SIGNIFICANT);
printf("set significance = %d\n", symbol );
          
          return_value = QccENTArithmeticEncode(&symbol, 1, model, buffer);

          if (return_value == 2) /* reach target_num_bits */
            return(2);
          else if (return_value) /* error */
            {
                QccErrorAddMessage("(QccSPECKInputOutputSetSignificance): Error calling QccENTArithmeticEncode()");
                return(1);
            }
      }
      else
        current_set->significance = QCCSPECK_SIGNIFICANT;
  }
  else if (model->current_context != QCCSPECK_CONTEXT_NOCODE)
  {
        if (QccENTArithmeticDecode(buffer, model, &symbol, 1))
          return(2);
        
        current_set->significance =
          (symbol) ? QCCSPECK_SIGNIFICANT : QCCSPECK_INSIGNIFICANT;
  }
  else
      current_set->significance = QCCSPECK_SIGNIFICANT;

  return(0);
}


static int QccSPECKInputOutputSign(char *sign,
                                   double *coefficient,
                                   double threshold,
                                   QccENTArithmeticModel *model,
                                   QccBitBuffer *buffer)
{
  int return_value;
  int symbol;

  model->current_context = QCCSPECK_CONTEXT_SIGN;

  if (buffer->type == QCCBITBUFFER_OUTPUT)
  {
      symbol = (*sign == QCCSPECK_POSITIVE);
printf("pixel sign = %d\n", symbol );

      *coefficient -= threshold;
      
      return_value = QccENTArithmeticEncode(&symbol, 1, model, buffer);

      if (return_value == 2)
        return(2);
      else
        if (return_value)
          {
            QccErrorAddMessage("(QccSPECKInputOutputSetSignificance): Error calling QccENTArithmeticEncode()");
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
        (symbol) ? QCCSPECK_POSITIVE : QCCSPECK_NEGATIVE;

      *coefficient  = 1.5 * threshold;
  }

  return(0);
}


static int QccSPECKPartitionSet(const QccSPECKSet *set,
                                const QccWAVSubbandPyramid *mask,
                                QccList *subsets)
{
  QccSPECKSet new_set;
  QccListNode *new_set_node;
  int num_rows;
  int num_cols;

  num_rows = set->num_rows - (set->num_rows >> 1);
  num_cols = set->num_cols - (set->num_cols >> 1);

  /* Lower right */
  new_set.type = QCCSPECK_SET_S;
  new_set.level = set->level + 1;
  new_set.origin_row = set->origin_row + num_rows;
  new_set.origin_col = set->origin_col + num_cols;
  new_set.num_rows = set->num_rows - num_rows;
  new_set.num_cols = set->num_cols - num_cols;
  new_set.significance = QCCSPECK_INSIGNIFICANT;
  if (QccSPECKSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECKSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  /* Lower left */
  new_set.type = QCCSPECK_SET_S;
  new_set.level = set->level + 1;
  new_set.origin_row = set->origin_row + num_rows;
  new_set.origin_col = set->origin_col;
  new_set.num_rows = set->num_rows - num_rows;
  new_set.num_cols = num_cols;
  new_set.significance = QCCSPECK_INSIGNIFICANT;
  if (QccSPECKSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECKSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  /* Upper right */
  new_set.type = QCCSPECK_SET_S;
  new_set.level = set->level + 1;
  new_set.origin_row = set->origin_row;
  new_set.origin_col = set->origin_col + num_cols;
  new_set.num_rows = num_rows;
  new_set.num_cols = set->num_cols - num_cols;
  new_set.significance = QCCSPECK_INSIGNIFICANT;
  if (QccSPECKSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECKSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }
  
  /* Upper left */
  new_set.type = QCCSPECK_SET_S;
  new_set.level = set->level + 1;
  new_set.origin_row = set->origin_row;
  new_set.origin_col = set->origin_col;
  new_set.num_rows = num_rows;
  new_set.num_cols = num_cols;
  new_set.significance = QCCSPECK_INSIGNIFICANT;
  if (QccSPECKSetTransparent(&new_set, mask))
    new_set.significance = QCCSPECK_EMPTYSET;
  if ((new_set_node =
       QccListCreateNode(sizeof(QccSPECKSet), &new_set)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListCreateNode()");
      return(1);
    }
  if (QccListAppendNode(subsets, new_set_node))
    {
      QccErrorAddMessage("(QccSPECKPartitionSet): Error calling QccListAppendNode()");
      return(1);
    }

  return(0);
}


static int QccSPECKProcessS(QccListNode *current_set_node,
                            QccListNode *current_list_node,
                            QccList *LSP,
                            QccList *garbage,
                            QccWAVSubbandPyramid *coefficients,
                            const QccWAVSubbandPyramid *mask,
                            char **significance_map,
                            char **sign_array,
                            double threshold,
                            QccENTArithmeticModel *model,
                            QccBitBuffer *buffer);

/* http://qccpack.sourceforge.net/Documentation/QccSPECKEncode.3.html
   "That is to say, as originally described by Pearlman() et al., recursion in SPECK 
   is limited to within CodeS(), whereas, in the QccPack implementation of SPECK, 
   ProcessS() and CodeS() recursively call one another. 
   This latter recursion strategy is a bit simpler to code, 
   while performance to the original algorithm is identical." */

/* Sam note: CodeS() doesn't need to output anything in this implementation. 
   It does, however, change the model context of the arithmetic encoder.  */

static int QccSPECKCodeS(QccListNode *current_set_node,
                         QccListNode *current_list_node,
                         QccList *LSP,
                         QccList *garbage,
                         QccWAVSubbandPyramid *coefficients,
                         const QccWAVSubbandPyramid *mask,
                         char **significance_map,
                         char **sign_array,
                         double threshold,
                         QccENTArithmeticModel *model,
                         QccBitBuffer *buffer)
{
  int           return_value;
  QccSPECKSet*  current_set = (QccSPECKSet *)(current_set_node->value);
  QccList*      next_list = (QccList *)(current_list_node->next->value);
  QccList       subsets;
  QccListNode*  current_subset_node;
  QccSPECKSet*  current_subset;
  int           subset_cnt = 0;
  int           prior_significance = QCCSPECK_INSIGNIFICANT;
  int           significance_cnt = 0;
  int           context_offset;

  QccListInitialize(&subsets);

  if (QccSPECKPartitionSet(current_set,
                           mask,
                           &subsets))
    {
      QccErrorAddMessage("(QccSPECKCodeS): Error calling QccSPECKPartitionSet()");
      return(1);
    }

  current_subset_node = subsets.start;

  while (current_subset_node != NULL)
  {
      if (QccListRemoveNode(&subsets, current_subset_node))
        {
          QccErrorAddMessage("(QccSPECKCodeS): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(next_list, current_subset_node))
        {
          QccErrorAddMessage("(QccSPECKCodeS): Error calling QccListAppendNode()");
          return(1);
        }

      if (prior_significance == QCCSPECK_INSIGNIFICANT)
          context_offset = 0;
      else if (prior_significance == QCCSPECK_EMPTYSET)
          context_offset = 1;
      else
          context_offset = 2;
        
      switch (subset_cnt)
        {
        case 0:
          model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S3;
          break;
        case 1:
          model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S2 + context_offset;
          break;
        case 2:
          model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S1 + context_offset;
          break;
        case 3:
          if (!significance_cnt)
            model->current_context = QCCSPECK_CONTEXT_NOCODE;
          else
            model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S0 + context_offset;
          break;
        }
        
      return_value = QccSPECKProcessS(current_subset_node,
                                      current_list_node->next,
                                      LSP,
                                      garbage,
                                      coefficients,
                                      mask,
                                      significance_map,
                                      sign_array,
                                      threshold,
                                      model,
                                      buffer);
      if (return_value == 1)
      {
          QccErrorAddMessage("(QccSPECKCodeS): Error calling QccSPECKProcessS()");
          return(1);
      }
      else if (return_value == 2)
      {
          QccListFree(&subsets);
          return(2);
      }

      current_subset = (QccSPECKSet *)(current_subset_node->value);
      prior_significance = current_subset->significance;
      /* seems empty set occurs when this set is marked by ''mask'' */
      if ((prior_significance != QCCSPECK_INSIGNIFICANT) &&
          (prior_significance != QCCSPECK_EMPTYSET))
        significance_cnt++;
      subset_cnt++;

      current_subset_node = subsets.start;
  }

  return(0);
}


static int QccSPECKProcessS(QccListNode *current_set_node,
                            QccListNode *current_list_node,
                            QccList *LSP,
                            QccList *garbage,
                            QccWAVSubbandPyramid *coefficients,
                            const QccWAVSubbandPyramid *mask,
                            char **significance_map,
                            char **sign_array,
                            double threshold,
                            QccENTArithmeticModel *model,
                            QccBitBuffer *buffer)
{
  int           return_value;
  QccSPECKSet*  current_set  = (QccSPECKSet *)(current_set_node->value);
  QccList*      current_list = (QccList *)(current_list_node->value);

  /* if the current set is empty, then put it in the garbage list and return! */
  /* seems empty set occurs when this set is marked by ''mask'' */
  if (current_set->significance == QCCSPECK_EMPTYSET)
    {
      if (QccListRemoveNode(current_list, current_set_node))
        {
          QccErrorAddMessage("(QccSPECKProcessS): Error calling QccListRemoveNode()");
          return(1);
        }
      if (QccListAppendNode(garbage, current_set_node))
        {
          QccErrorAddMessage("(QccSPECKProcessS): Error calling QccListAppendNode()");
          return(1);
        }

      return(0);
    }

  /* Write to buffer the significance value of current_set.
     Corresponds to line 1) of ProcessS() in Figure 2.   */
  return_value = QccSPECKInputOutputSetSignificance(current_set,
                                                    significance_map,
                                                    model,
                                                    buffer);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccSPECKProcessS): Error calling QccSPECKInputOutputSetSignificance()");
      return(1);
    }
  else if (return_value == 2) /* reach target_num_bits */
      return(2);

  /* Line 2) of ProcessS() in Figure 2. */
  if (current_set->significance != QCCSPECK_INSIGNIFICANT)
  {
      if (QccListRemoveNode(current_list, current_set_node))
        {
          QccErrorAddMessage("(QccSPECKProcessS): Error calling QccListRemoveNode()");
          return(1);
        }
      /* temporarily put current_set_node in garbage. It will be rescued either in line 
         873 or inside of CodeS at line 886. */
      if (QccListAppendNode(garbage, current_set_node))
        {
          QccErrorAddMessage("(QccSPECKProcessS): Error calling QccListAppendNode()");
          return(1);
        }

      if (QccSPECKSetIsPixel(current_set)) /* Current set is one pixel. */
      {
          current_set->significance = QCCSPECK_NEWLY_SIGNIFICANT;
          return_value = QccSPECKInputOutputSign(&sign_array              /* sign is output */
                                                 [current_set->origin_row]
                                                 [current_set->origin_col],
       /* coefficient is reduced by threshold */ &coefficients->matrix
                                                 [current_set->origin_row]
                                                 [current_set->origin_col],
                                                 threshold,
                                                 model,
                                                 buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECKProcessS): Error calling QccSPECKInputOutputSign()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);

          if (QccListRemoveNode(garbage, current_set_node))
            {
              QccErrorAddMessage("(QccSPECKProcessS): Error calling QccListRemoveNode()");
              return(1);
            }
          if (QccListAppendNode(LSP, current_set_node))
            {
              QccErrorAddMessage("(QccSPECKProcessS): Error calling QccListAppendNode()");
              return(1);
            }
      }
      else
      {
          return_value = QccSPECKCodeS(current_set_node,
                                       current_list_node,
                                       LSP,
                                       garbage,
                                       coefficients,
                                       mask,
                                       significance_map,
                                       sign_array,
                                       threshold,
                                       model,
                                       buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECKProcessS): Error calling QccSPECKCodeS()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
      }
  }

  return(0);
}


static int QccSPECKProcessI(QccSPECKSet *I,
                            QccList *LIS,
                            QccList *LSP,
                            QccList *garbage,
                            QccWAVSubbandPyramid *coefficients,
                            const QccWAVSubbandPyramid *mask,
                            char **significance_map,
                            char **sign_array,
                            double threshold,
                            QccENTArithmeticModel *model,
                            QccBitBuffer *buffer);


static int QccSPECKCodeI(QccSPECKSet *I,
                         QccList *LIS,
                         QccList *LSP,
                         QccList *garbage,
                         QccWAVSubbandPyramid *coefficients,
                         const QccWAVSubbandPyramid *mask,
                         char **significance_map,
                         char **sign_array,
                         double threshold,
                         QccENTArithmeticModel *model,
                         QccBitBuffer *buffer)
{
  int           return_value;
  QccWAVSubbandPyramid subband_pyramid;
  int           subband;
  QccSPECKSet   set_S;
  QccSPECKSet*  current_set;
  QccListNode*  current_list_node;
  QccListNode*  current_set_node;
  int           prior_significance = QCCSPECK_INSIGNIFICANT;
  int           significance_cnt = 0;
  int           mask_subband;

  QccWAVSubbandPyramidInitialize(&subband_pyramid);

  subband_pyramid.num_rows = I->num_rows;
  subband_pyramid.num_cols = I->num_cols;
  subband_pyramid.num_levels = I->level;

  /* There's no explicit partition on Set I; 
     just create and process each Set S on the fly. */
  for (subband = 3; subband >= 1; subband--)
  {
      /* Construct a Set S */
      set_S.type         = QCCSPECK_SET_S;
      set_S.level        = I->level;
      set_S.significance = QCCSPECK_INSIGNIFICANT;
      mask_subband = subband - 1 +
        QccWAVSubbandPyramidNumLevelsToNumSubbands(coefficients->num_levels -
                                                   I->level);
      /* Get the origin and dimensions of the current Set S */
      return_value = QccSPECKSetSize(&set_S,
                                     coefficients,
                                     mask_subband);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECKCodeI): Error calling QccSPECKSetSize()");
          return(1);
        }

      if (!QccSPECKSetTransparent(&set_S, mask))
      {
          if (QccSPECKInsertSet(LIS,
                                &set_S,
                                &current_list_node, /* output */
                                &current_set_node)) /* output */
            {
              QccErrorAddMessage("(QccSPECKCodeI): Error calling QccSPECKInsertSet()");
              return(1);
            }
          
          switch (subband)
          {
            case 3:
              model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S3;
              break;
            case 2:
              model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S2 +
                                       (prior_significance != QCCSPECK_INSIGNIFICANT);
              break;
            case 1:
              model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE_S1 +
                                       (prior_significance != QCCSPECK_INSIGNIFICANT);
              break;
          }
          
          return_value =
            QccSPECKProcessS(current_set_node,
                             current_list_node,
                             LSP,
                             garbage,
                             coefficients,
                             mask,
                             significance_map,
                             sign_array,
                             threshold,
                             model,
                             buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECKCodeI): Error calling QccSPECKProcessS()");
              return(1);
            }
          else
            if (return_value == 2)
              return(2);
          
          current_set = (QccSPECKSet *)(current_set_node->value);
          prior_significance = current_set->significance;
          if (prior_significance != QCCSPECK_INSIGNIFICANT)
            significance_cnt++;
      }
  }   /* Finish processing 3 Type S sets */

  I->level--;
  subband_pyramid.num_levels = I->level;
  if (QccWAVSubbandPyramidSubbandSize(&subband_pyramid,
                                      0,
                                      &I->origin_row,
                                      &I->origin_col))
    {
      QccErrorAddMessage("(QccSPECKCodeI): Error calling QccWAVSubbandPyramidSubbandSize()");
      return(1);
    }

  return_value = QccSPECKProcessI(I,
                                  LIS,
                                  LSP,
                                  garbage,
                                  coefficients,
                                  mask,
                                  significance_map,
                                  sign_array,
                                  threshold,
                                  model,
                                  buffer);
  if (return_value == 1)
    {
      QccErrorAddMessage("(QccSPECKCodeI): Error calling QccSPECKProcessI()");
      return(1);
    }
  else
    if (return_value == 2)
      return(2);

  return(0);
}


static int QccSPECKProcessI(QccSPECKSet *I,
                            QccList *LIS,
                            QccList *LSP,
                            QccList *garbage,
                            QccWAVSubbandPyramid *coefficients,
                            const QccWAVSubbandPyramid *mask,
                            char **significance_map,
                            char **sign_array,
                            double threshold,
                            QccENTArithmeticModel *model,
                            QccBitBuffer *buffer)
{
  int return_value;

  if (!I->level)
    return(0);

  model->current_context = QCCSPECK_CONTEXT_I;

  return_value = QccSPECKInputOutputSetSignificance(I,
                                                    significance_map,
                                                    model,
                                                    buffer);
  if (return_value == 1)
  {
      QccErrorAddMessage("(QccSPECKProcessI): Error calling QccSPECKInputOutputSetSignificance()");
      return(1);
  }
  else if (return_value == 2)
      return(2);

  if (I->significance == QCCSPECK_SIGNIFICANT)
  {
      return_value = QccSPECKCodeI(I,
                                   LIS,
                                   LSP,
                                   garbage,
                                   coefficients,
                                   mask,
                                   significance_map,
                                   sign_array,
                                   threshold,
                                   model,
                                   buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECKProcessI): Error calling QccSPECKCodeI()");
          return(1);
        }
      else if (return_value == 2)
          return(2);
  }

  return(0);
}


static int QccSPECKSortingPass(QccWAVSubbandPyramid *coefficients,
                               const QccWAVSubbandPyramid *mask,
                               char **significance_map,
                               char **sign_array,
                               double threshold,
                               QccList *LIS,
                               QccList *LSP,
                               QccSPECKSet *I,
                               QccENTArithmeticModel *model,
                               QccBitBuffer *buffer)
{
  int           return_value;
  QccListNode*  current_list_node;
  QccList*      current_list;
  QccListNode*  current_set_node;
  QccListNode*  next_set_node;
  QccList       garbage;

  QccListInitialize(&garbage);

  /* Compare every coefficient against the threshold. Note coefficients are ALL positive 
     at this point. Keep the comparison results in significance_map. */
  if (QccSPECKSignificanceMap(coefficients,
                              mask,
                              significance_map,
                              threshold))
    {
      QccErrorAddMessage("(QccSPECKSortingPass): Error calling QccSPECKSignificanceMap()");
      goto Error;
    }

  /* starting from the end of LIS, we process smaller sets first. */
  current_list_node = LIS->end;
  while (current_list_node != NULL)
  {
      current_list = (QccList *)(current_list_node->value);
      current_set_node = current_list->start;
      while (current_set_node != NULL)
      {
          next_set_node = current_set_node->next;

          model->current_context = QCCSPECK_CONTEXT_SIGNIFICANCE;

          return_value = QccSPECKProcessS(current_set_node,
                                          current_list_node,
                                          LSP,
                                          &garbage,
                                          coefficients,
                                          mask,
                                          significance_map,
                                          sign_array,
                                          threshold,
                                          model,
                                          buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECKSortingPass): Error calling QccSPECKProcessS()");
              return(1);
            }
          else
            if (return_value == 2)
              goto Finished;

          current_set_node = next_set_node;
      }

      current_list_node = current_list_node->previous;
  }

  return_value = QccSPECKProcessI(I,
                                  LIS,
                                  LSP,
                                  &garbage,
                                  coefficients,
                                  mask,
                                  significance_map,
                                  sign_array,
                                  threshold,
                                  model,
                                  buffer);
  if (return_value == 1)
  {
      QccErrorAddMessage("(QccSPECKSortingPass): Error calling QccSPECKProcessI()");
      goto Error;
  }
  else if (return_value == 2)
      goto Finished;

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


static int QccSPECKRefinementInputOutput(double *coefficient,
                                         double threshold,
                                         QccENTArithmeticModel *model,
                                         QccBitBuffer *buffer)
{
  int return_value;
  int symbol;

  model->current_context = QCCSPECK_CONTEXT_REFINEMENT;

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
            QccErrorAddMessage("(QccSPECKRefinementInputOutput): Error calling QccENTArithmeticEncode()");
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


static int QccSPECKRefinementPass(QccWAVSubbandPyramid *coefficients,
                                  QccList *LSP,
                                  double threshold,
                                  QccENTArithmeticModel *model,
                                  QccBitBuffer *buffer)
{
  int return_value;
  QccListNode *current_set_node;
  QccSPECKSet *current_set;

  current_set_node = LSP->start;
  while (current_set_node != NULL)
  {
      current_set = (QccSPECKSet *)(current_set_node->value);
      
      if (current_set->significance == QCCSPECK_NEWLY_SIGNIFICANT)
        current_set->significance = QCCSPECK_SIGNIFICANT;
      else
      {
          return_value =
            QccSPECKRefinementInputOutput(&coefficients->matrix
                                          [current_set->origin_row]
                                          [current_set->origin_col],
                                          threshold,
                                          model,
                                          buffer);
          if (return_value == 1)
            {
              QccErrorAddMessage("(QccSPECKRefinementPass): Error calling QccSPECKRefinementInputOutput()");
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


static int QccSPECKEncodeDWT(QccWAVSubbandPyramid *image_subband_pyramid,
                             char **sign_array,
                             const QccIMGImageComponent *image,
                             int num_levels,
                             double *image_mean,
                             int *max_coefficient_bits,
                             QccWAVSubbandPyramid *mask_subband_pyramid,
                             const QccIMGImageComponent *mask,
                             const QccWAVWavelet *wavelet)
{
  double coefficient_magnitude;
  double max_coefficient = -MAXFLOAT;
  int row, col;
  
  if (mask == NULL)
    {
      if (QccMatrixCopy(image_subband_pyramid->matrix, image->image,
                        image->num_rows, image->num_cols))
        {
          QccErrorAddMessage("(QccSPECKEncodeDWT): Error calling QccMatrixCopy()");
          return(1);
        }
      
      if (QccWAVSubbandPyramidSubtractMean(image_subband_pyramid,
                                           image_mean,
                                           NULL))
        {
          QccErrorAddMessage("(QccSPECKEncodeDWT): Error calling QccWAVSubbandPyramidSubtractMean()");
          return(1);
        }
    }
  else
    { /*
      if (QccMatrixCopy(mask_subband_pyramid->matrix, mask->image,
                        mask->num_rows, mask->num_cols))
        {
          QccErrorAddMessage("(QccSPECKEncodeDWT): Error calling QccMatrixCopy()");
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
        */
    }
  
  if (mask != NULL)
    { /*
      if (QccWAVSubbandPyramidShapeAdaptiveDWT(image_subband_pyramid,
                                               mask_subband_pyramid,
                                               num_levels,
                                               wavelet))
        {
          QccErrorAddMessage("(QccSPECKEncodeDWT): Error calling QccWAVSubbandPyramidShapeAdaptiveDWT()");
          return(1);
        } */
    }
  else
    if (QccWAVSubbandPyramidDWT(image_subband_pyramid,
                                num_levels,
                                wavelet))
      {
        QccErrorAddMessage("(QccSPECKEncodeDWT): Error calling QccWAVSubbandPyramidDWT()");
        return(1);
      }
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      {
        coefficient_magnitude = fabs(image_subband_pyramid->matrix[row][col]);
        if (image_subband_pyramid->matrix[row][col] != coefficient_magnitude)
          {
            image_subband_pyramid->matrix[row][col] = coefficient_magnitude;
            sign_array[row][col] = QCCSPECK_NEGATIVE;
          }
        else
          sign_array[row][col] = QCCSPECK_POSITIVE;
        if (coefficient_magnitude > max_coefficient)
          max_coefficient = coefficient_magnitude;
      }
  
  *max_coefficient_bits = (int)floor(QccMathLog2(max_coefficient));
  
  return(0);
}


int QccSPECKEncodeHeader(QccBitBuffer *output_buffer, 
                         int num_levels, 
                         int num_rows, int num_cols,
                         double image_mean,
                         int max_coefficient_bits)
{
  int return_value;
  
  if (QccBitBufferPutChar(output_buffer, (unsigned char)num_levels))
    {
      QccErrorAddMessage("(QccSPECKEncodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_rows))
    {
      QccErrorAddMessage("(QccSPECKEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, num_cols))
    {
      QccErrorAddMessage("(QccSPECKEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  if (QccBitBufferPutDouble(output_buffer, image_mean))
    {
      QccErrorAddMessage("(QccSPECKEncodeHeader): Error calling QccBitBufferPutDouble()");
      goto Error;
    }
  
  if (QccBitBufferPutInt(output_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccSPECKEncodeHeader): Error calling QccBitBufferPutInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}



/* Sam: Overall Encoding Procedure */
int QccSPECKEncode(const QccIMGImageComponent *image,
                   const QccIMGImageComponent *mask,
                   int num_levels,
                   int target_bit_cnt,
                   const QccWAVWavelet *wavelet,
                   QccBitBuffer *output_buffer)
{
  int return_value;
  QccENTArithmeticModel *model = NULL;
  QccWAVSubbandPyramid image_subband_pyramid;
  QccWAVSubbandPyramid mask_subband_pyramid;
  char **sign_array = NULL;
  char **significance_map = NULL;
  double image_mean;
  int max_coefficient_bits;
  double threshold;
  int row, col;
  QccList LIS;
  QccList LSP;
  QccSPECKSet I;
  int bitplane = 0;

  if (image == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);
  QccListInitialize(&LIS);
  QccListInitialize(&LSP);

  image_subband_pyramid.num_levels = 0;
  image_subband_pyramid.num_rows = image->num_rows;
  image_subband_pyramid.num_cols = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccSPECKEncode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
  {
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccSPECKEncode): Mask and image must be same size");
          goto Error;
        }

      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccSPECKEncode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }
  }  
  
  /* allocate space memory space for sign_array */
  if ((sign_array = (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKEncode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccSPECKEncode): Error allocating memory");
        goto Error;
      }
  
  /* allocate memory space for significance_map */
  if ((significance_map =
       (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKEncode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((significance_map[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccSPECKEncode): Error allocating memory");
        goto Error;
      }
  
  /* every location of the significance_map is initialized to be INSIGNIFICANT */
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      significance_map[row][col] = QCCSPECK_INSIGNIFICANT;

  /* Perform DWT, and collect 1) image_mean, 2) max_coefficient_bits, and 3) sign_array.
     Note that the returned wavelet coefficients are all positive.                    */
  if (QccSPECKEncodeDWT(&image_subband_pyramid,
                        sign_array,
                        image,
                        num_levels,
                        &image_mean,
                        &max_coefficient_bits,
                        ((mask != NULL) ? &mask_subband_pyramid : NULL),
                        mask,
                        wavelet))
    {
      QccErrorAddMessage("(QccSPECKEncode): Error calling QccSPECKEncodeDWT()");
      goto Error;
    }

  if (QccSPECKEncodeHeader(output_buffer,
                           num_levels,
                           image->num_rows, image->num_cols, 
                           image_mean,
                           max_coefficient_bits))
    {
      QccErrorAddMessage("(QccSPECKEncode): Error calling QccSPECKEncodeHeader()");
      goto Error;
    }
  
  if ((model = QccENTArithmeticEncodeStart(QccSPECKNumSymbols,    /* fourteen 2s */
                                           QCCSPECK_NUM_CONTEXTS, /* 14 */
                                           NULL,
                                           target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKEncode): Error calling QccENTArithmeticEncodeStart()");
      goto Error;
    }

  threshold = pow((double)2, (double)max_coefficient_bits);
  
  /* Pay attention to how LIS is represented */
  if (QccSPECKInitialization(&LIS,
                             &LSP,
                             &I,
                             &image_subband_pyramid,
                             ((mask != NULL) ?
                              &mask_subband_pyramid : NULL)))
    {
      QccErrorAddMessage("(QccSPECKEncode): Error calling QccSPECKInitialization()");
      goto Error;
    }

  /* bitplane starts from 0, while QCCSPECK_MAXBITPLANES is 128 */
  while (bitplane < QCCSPECK_MAXBITPLANES)
    {
      return_value = QccSPECKSortingPass(&image_subband_pyramid,
                                         ((mask != NULL) ?
                                          &mask_subband_pyramid : NULL),
                                         significance_map,
                                         sign_array,
                                         threshold,
                                         &LIS,
                                         &LSP,
                                         &I,
                                         model,
                                         output_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECKEncode): Error calling QccSPECKSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      return_value = QccSPECKRefinementPass(&image_subband_pyramid,
                                            &LSP,
                                            threshold,
                                            model,
                                            output_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECKEncode): Error calling QccSPECKRefinementPass()");
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
  QccWAVSubbandPyramidFree(&image_subband_pyramid);
  QccWAVSubbandPyramidFree(&mask_subband_pyramid);
  if (sign_array != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  if (significance_map != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (significance_map[row] != NULL)
          QccFree(significance_map[row]);
      QccFree(significance_map);
    }
  QccENTArithmeticFreeModel(model);
  QccSPECKFreeLIS(&LIS);
  QccListFree(&LSP);
  return(return_value);
}
/* Sam: Finish Overall Encoding Procedure */



static int QccSPECKDecodeInverseDWT(QccWAVSubbandPyramid
                                    *image_subband_pyramid,
                                    QccWAVSubbandPyramid
                                    *mask_subband_pyramid,
                                    char **sign_array,
                                    QccIMGImageComponent *image,
                                    double image_mean,
                                    const QccWAVWavelet *wavelet)
{
  int row, col;
  
  for (row = 0; row < image_subband_pyramid->num_rows; row++)
    for (col = 0; col < image_subband_pyramid->num_cols; col++)
      if (sign_array[row][col] == QCCSPECK_NEGATIVE)
        image_subband_pyramid->matrix[row][col] *= -1;
  
  if (mask_subband_pyramid != NULL)
    {
      if (QccWAVSubbandPyramidInverseShapeAdaptiveDWT(image_subband_pyramid,
                                                      mask_subband_pyramid,
                                                      wavelet))
        {
          QccErrorAddMessage("(QccSPECKDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseShapeAdaptiveDWT()");
          return(1);
        }
    }
  else
    if (QccWAVSubbandPyramidInverseDWT(image_subband_pyramid,
                                       wavelet))
      {
        QccErrorAddMessage("(QccSPECKDecodeInverseDWT): Error calling QccWAVSubbandPyramidInverseDWT()");
        return(1);
      }
  
  if (QccWAVSubbandPyramidAddMean(image_subband_pyramid,
                                  image_mean))
    {
      QccErrorAddMessage("(QccSPECKDecodeInverseDWT): Error calling QccWAVSubbandPyramidAddMean()");
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
      QccErrorAddMessage("(QccSPECKDecodeInverseDWT): Error calling QccMatrixCopy()");
      return(1);
    }
  
  if (QccIMGImageComponentSetMaxMin(image))
    {
      QccErrorAddMessage("(QccSPECKDecodeInverseDWT): Error calling QccIMGImageComponentSetMaxMin()");
      return(1);
    }

  return(0);
}


int QccSPECKDecodeHeader(QccBitBuffer *input_buffer, 
                         int *num_levels, 
                         int *num_rows, int *num_cols,
                         double *image_mean,
                         int *max_coefficient_bits)
{
  int return_value;
  unsigned char ch;

  if (QccBitBufferGetChar(input_buffer, &ch))
    {
      QccErrorAddMessage("(QccSPECKDecodeHeader): Error calling QccBitBufferPuChar()");
      goto Error;
    }
  *num_levels = (int)ch;
  
  if (QccBitBufferGetInt(input_buffer, num_rows))
    {
      QccErrorAddMessage("(QccSPECKDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, num_cols))
    {
      QccErrorAddMessage("(QccSPECKDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  if (QccBitBufferGetDouble(input_buffer, image_mean))
    {
      QccErrorAddMessage("(QccSPECKDecodeHeader): Error calling QccBitBufferGetDouble()");
      goto Error;
    }
  
  if (QccBitBufferGetInt(input_buffer, max_coefficient_bits))
    {
      QccErrorAddMessage("(QccSPECKDecodeHeader): Error calling QccBitBufferGetInt()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


/* Sam: overall decode routine */
int QccSPECKDecode(QccBitBuffer *input_buffer,
                   QccIMGImageComponent *image,
                   const QccIMGImageComponent *mask,
                   int num_levels,
                   const QccWAVWavelet *wavelet,
                   double image_mean,
                   int max_coefficient_bits,
                   int target_bit_cnt)
{
  int                    return_value;
  QccENTArithmeticModel* model = NULL;
  QccWAVSubbandPyramid   image_subband_pyramid;
  QccWAVSubbandPyramid   mask_subband_pyramid;
  char**                 sign_array = NULL;
  double                 threshold;
  int row, col;
  QccWAVWavelet          lazy_wavelet_transform;
  QccList     LIS;
  QccList     LSP;
  QccSPECKSet I;

  if (image == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);
  if (wavelet == NULL)
    return(0);
  
  QccWAVSubbandPyramidInitialize(&image_subband_pyramid);
  QccWAVSubbandPyramidInitialize(&mask_subband_pyramid);
  QccWAVWaveletInitialize(&lazy_wavelet_transform);
  QccListInitialize(&LIS);
  QccListInitialize(&LSP);
  
  image_subband_pyramid.num_levels = num_levels;
  image_subband_pyramid.num_rows   = image->num_rows;
  image_subband_pyramid.num_cols   = image->num_cols;
  if (QccWAVSubbandPyramidAlloc(&image_subband_pyramid))
    {
      QccErrorAddMessage("(QccSPECKDecode): Error calling QccWAVSubbandPyramidAlloc()");
      goto Error;
    }
  
  if (mask != NULL)
  { /*
      if ((mask->num_rows != image->num_rows) ||
          (mask->num_cols != image->num_cols))
        {
          QccErrorAddMessage("(QccSPECKDecode): Mask and image must be same size");
          goto Error;
        }

      if (QccWAVWaveletCreate(&lazy_wavelet_transform, "LWT.lft", "symmetric"))
        {
          QccErrorAddMessage("(QccSPECKDecode): Error calling QccWAVWaveletCreate()");
          goto Error;
        }

      mask_subband_pyramid.num_levels = 0;
      mask_subband_pyramid.num_rows = mask->num_rows;
      mask_subband_pyramid.num_cols = mask->num_cols;
      if (QccWAVSubbandPyramidAlloc(&mask_subband_pyramid))
        {
          QccErrorAddMessage("(QccSPECKDecode): Error calling QccWAVSubbandPyramidAlloc()");
          goto Error;
        }

      if (QccMatrixCopy(mask_subband_pyramid.matrix,
                        mask->image,
                        mask->num_rows,
                        mask->num_cols))
        {
          QccErrorAddMessage("(QccSPECKDecode): Error calling QccMatrixCopy()");
          goto Error;
        }

      if (QccWAVSubbandPyramidDWT(((mask != NULL) ?
                                   &mask_subband_pyramid : NULL),
                                  num_levels,
                                  &lazy_wavelet_transform))
        {
          QccErrorAddMessage("(QccSPECKDecode): Error calling QccWAVSubbandPyramidDWT()");
          goto Error;
        } */
  }
  
  /* Allocate memory for sign_array. */
  if ((sign_array = (char **)malloc(sizeof(char *) * image->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKDecode): Error allocating memory");
      goto Error;
    }
  for (row = 0; row < image->num_rows; row++)
    if ((sign_array[row] =
         (char *)malloc(sizeof(char) * image->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccSPECKDecode): Error allocating memory");
        goto Error;
      }
  
  /* Initialize coefficients to be zero, and signs to be positive. */
  for (row = 0; row < image->num_rows; row++)
    for (col = 0; col < image->num_cols; col++)
      {
        image_subband_pyramid.matrix[row][col] = 0.0;
        sign_array[row][col] = QCCSPECK_POSITIVE;
      }
  
  if ((model =
       QccENTArithmeticDecodeStart(input_buffer,
                                   QccSPECKNumSymbols,
                                   QCCSPECK_NUM_CONTEXTS,
                                   NULL,
                                   target_bit_cnt)) == NULL)
    {
      QccErrorAddMessage("(QccSPECKDecode): Error calling QccENTArithmeticDecodeStart()");
      goto Error;
    }

  threshold = pow((double)2, (double)max_coefficient_bits);
  
  if (QccSPECKInitialization(&LIS,
                             &LSP,
                             &I,
                             &image_subband_pyramid,
                             ((mask != NULL) ?
                              &mask_subband_pyramid : NULL)))
    {
      QccErrorAddMessage("(QccSPECKDecode): Error calling QccSPECKInitialization()");
      goto Error;
    }

  while (1)
  {
      return_value = QccSPECKSortingPass(&image_subband_pyramid,
                                         ((mask != NULL) ?
                                          &mask_subband_pyramid : NULL),
                                         NULL,
                                         sign_array,
                                         threshold,
                                         &LIS,
                                         &LSP,
                                         &I,
                                         model,
                                         input_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECKDecode): Error calling QccSPECKSortingPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      return_value = QccSPECKRefinementPass(&image_subband_pyramid,
                                            &LSP,
                                            threshold,
                                            model,
                                            input_buffer);
      if (return_value == 1)
        {
          QccErrorAddMessage("(QccSPECKDecode): Error calling QccSPECKRefinementPass()");
          goto Error;
        }
      else
        if (return_value == 2)
          goto Finished;

      threshold /= 2.0;
  }

 Finished:
  if (QccSPECKDecodeInverseDWT(&image_subband_pyramid,
                               ((mask != NULL) ?
                                &mask_subband_pyramid : NULL),
                               sign_array,
                               image,
                               image_mean,
                               wavelet))
    {
      QccErrorAddMessage("(QccSPECKDecode): Error calling QccSPECKDecodeInverseDWT()");
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
  if (sign_array != NULL)
    {
      for (row = 0; row < image->num_rows; row++)
        if (sign_array[row] != NULL)
          QccFree(sign_array[row]);
      QccFree(sign_array);
    }
  QccENTArithmeticFreeModel(model);
  QccSPECKFreeLIS(&LIS);
  QccListFree(&LSP);
  return(return_value);
}
/* Sam: finish overall decode routine */
