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


static int QccENTHuffmanDesignCompareProbabilities(const void *value1,
                                                   const void *value2)
{
  QccENTHuffmanDesignTreeNode *node1, *node2;
  
  node1 = (QccENTHuffmanDesignTreeNode *)value1;
  node2 = (QccENTHuffmanDesignTreeNode *)value2;
  
  if (node1->probability < node2->probability)
    return(-1);
  if (node1->probability > node2->probability)
    return(1);
  
  return(0);
}


static int QccENTHuffmanDesignCompareCodewordLengths(const void *value1,
                                                     const void *value2)
{
  QccENTHuffmanDesignTreeNode *node1, *node2;
  
  node1 = (QccENTHuffmanDesignTreeNode *)value1;
  node2 = (QccENTHuffmanDesignTreeNode *)value2;
  
  if (node1->codeword_length < node2->codeword_length)
    return(-1);
  if (node1->codeword_length > node2->codeword_length)
    return(1);
  
  return(0);
}


/*
static void QccENTHuffmanDesignPrintNode(QccENTHuffmanDesignTreeNode
                                         *tree_node)
{
  if (tree_node == 0)
    return;

  printf("    Symbol: %d",
         tree_node->symbol);
  printf("    Probability: %f",
         tree_node->probability);
  printf("    Codeword Length: %d\n",
         tree_node->codeword_length);
}
*/


static int QccENTHuffmanDesignMergeNodes(QccList *tree, QccList *leaf_list)
{
  int return_value;
  QccListNode *new_node = NULL;
  QccENTHuffmanDesignTreeNode tree_node;
  
  tree_node.left = tree->start;
  if (QccListRemoveNode(tree, tree_node.left))
    {
      QccErrorAddMessage("(QccENTHuffmanDesignMergeNodes): Error calling QccListRemoveNode()");
      goto Error;
    }
  if (((QccENTHuffmanDesignTreeNode *)(tree_node.left->value))->left == NULL)
    if (QccListAppendNode(leaf_list, tree_node.left))
      {
        QccErrorAddMessage("(QccENTHuffmanDesignMergeNodes): Error calling QccListAppendNode()");
        goto Error;
      }

  tree_node.right = tree->start;
  if (QccListRemoveNode(tree, tree_node.right))
    {
      QccErrorAddMessage("(QccENTHuffmanDesignMergeNodes): Error calling QccListRemoveNode()");
      goto Error;
    }
  if (((QccENTHuffmanDesignTreeNode *)(tree_node.right->value))->left == NULL)
    if (QccListAppendNode(leaf_list, tree_node.right))
      {
        QccErrorAddMessage("(QccENTHuffmanDesignMergeNodes): Error calling QccListAppendNode()");
        goto Error;
      }
  
  tree_node.symbol = -1;
  tree_node.probability =
    ((QccENTHuffmanDesignTreeNode *)(tree_node.left->value))->probability +
    ((QccENTHuffmanDesignTreeNode *)(tree_node.right->value))->probability;
  
  if ((new_node =
       QccListCreateNode(sizeof(QccENTHuffmanDesignTreeNode),
                         (void *)(&tree_node))) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanDesignMergeNodes): Error calling QccListCreateNode()");
      goto Error;
    }
  
  if (QccListSortedInsertNode(tree, new_node,
                              QCCLIST_SORTASCENDING,
                              QccENTHuffmanDesignCompareProbabilities))
    {
      QccErrorAddMessage("(QccENTHuffmanDesignMergeNodes): Error calling QccListSortedInsertNode()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
  QccListFreeNode(new_node);
 Return:
  return(return_value);
}


int QccENTHuffmanDesignAssignLengths(QccListNode *current,
                                     int codeword_length)
{
  QccENTHuffmanDesignTreeNode *tree_node;
  QccListNode *left, *right;
  
  if (current == NULL)
    return(0);
  
  if (codeword_length > QCCENTHUFFMAN_MAXCODEWORDLEN)
    {
      QccErrorAddMessage("QccENTHuffmanDesignAssignLengths): Codeword length has exceeded maximum allowed length of %d bits",
                         QCCENTHUFFMAN_MAXCODEWORDLEN);
      return(1);
    }
  
  tree_node = (QccENTHuffmanDesignTreeNode *)current->value;
  
  left = tree_node->left;
  right = tree_node->right;
  tree_node->codeword_length = codeword_length;
  
  codeword_length++;
  
  if (QccENTHuffmanDesignAssignLengths(left, codeword_length))
    return(1);
  if (QccENTHuffmanDesignAssignLengths(right, codeword_length))
    return(1);
  
  return(0);
}


void QccENTHuffmanDesignFreeTree(QccListNode *current)
{
  QccENTHuffmanDesignTreeNode *tree_node;

  if (current == NULL)
    return;

  tree_node = (QccENTHuffmanDesignTreeNode *)current->value;

  QccENTHuffmanDesignFreeTree(tree_node->left);
  QccENTHuffmanDesignFreeTree(tree_node->right);
}


int QccENTHuffmanDesign(const int *symbols,
                        const double *probabilities,
                        int num_symbols,
                        QccENTHuffmanTable *huffman_table)
{
  int return_value;
  int symbol;
  QccList huffman_tree;
  QccList leaf_list;
  QccListNode *new_node = NULL;
  QccENTHuffmanDesignTreeNode tree_node;
  int current_length;
  QccListNode *current;
  QccENTHuffmanDesignTreeNode *current_tree_node;
  int num_used_symbols;

  QccListInitialize(&huffman_tree);
  QccListInitialize(&leaf_list);

  if (symbols == NULL)
    return(0);
  if (probabilities == NULL)
    return(0);
  
  if ((huffman_table->table_type != QCCENTHUFFMAN_DECODETABLE) &&
      (huffman_table->table_type != QCCENTHUFFMAN_ENCODETABLE))
    {
      QccErrorAddMessage("(QccENTHuffmanDesign): Invalid Huffman table type specified");
      goto Error;
    }

  if (num_symbols > QCCENTHUFFMAN_MAXSYMBOL + 1)
    {
      QccErrorAddMessage("(QccENTHuffmanDesign): Too many symbols");
      goto Error;
    }
  
  num_used_symbols = 0;
  for (symbol = 0; symbol < num_symbols; symbol++)
    {
      if ((probabilities[symbol] < 0) || (probabilities[symbol] > 1))
        {
          QccErrorAddMessage("(QccENTHuffmanDesign): Invalid probability found in probabilities list");
          goto Error;
        }
      
      if (probabilities[symbol] != 0)
        {
          tree_node.left = tree_node.right = NULL;
          tree_node.symbol = symbols[symbol];
          tree_node.probability = probabilities[symbol];
          
          if ((new_node =
               QccListCreateNode(sizeof(QccENTHuffmanDesignTreeNode),
                                 (void *)(&tree_node))) == NULL)
            {
              QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccListCreateNode()");
              goto Error;
            }
          
          if (QccListSortedInsertNode(&huffman_tree, new_node,
                                      QCCLIST_SORTASCENDING,
                                      QccENTHuffmanDesignCompareProbabilities))
            {
              QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccListSortedInsertNode()");
              goto Error;
            }

          num_used_symbols++;
        }
    }
  
  while (QccListLength(&huffman_tree) > 1)
    if (QccENTHuffmanDesignMergeNodes(&huffman_tree, &leaf_list))
      {
        QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccENTHuffmanDesignMergeNodes()");
        goto Error;
      }

  if (QccENTHuffmanDesignAssignLengths(huffman_tree.start, 0))
    {
      QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccENTHuffmanDesignAssignLengths()");
      goto Error;
    }
  
  if (QccListSort(&leaf_list, QCCLIST_SORTASCENDING,
                  QccENTHuffmanDesignCompareCodewordLengths))
    {
      QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccListSort()");
      goto Error;
    }

  huffman_table->num_codewords_list_length = QCCENTHUFFMAN_MAXCODEWORDLEN;
  if ((huffman_table->num_codewords_list =
       (int *)malloc(sizeof(int) *
                     huffman_table->num_codewords_list_length)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanDesign): Error allocating memory");
      goto Error;
    }
  for (current_length = 1; current_length <= QCCENTHUFFMAN_MAXCODEWORDLEN;
       current_length++)
    huffman_table->num_codewords_list[current_length - 1] = 0;

  huffman_table->symbol_list_length = num_used_symbols;
  if ((huffman_table->symbol_list =
       (int *)malloc(sizeof(int) * huffman_table->symbol_list_length)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanDesign): Error allocating memory");
      goto Error;
    }

  current_length = 1;
  current = leaf_list.start;
  symbol = 0;
  while (current != NULL)
    {
      current_tree_node = (QccENTHuffmanDesignTreeNode *)current->value;
      while (current_tree_node->codeword_length != current_length)
        current_length++;
      huffman_table->num_codewords_list[current_length - 1]++;
      huffman_table->symbol_list[symbol++] = current_tree_node->symbol;
      current = current->next;
    }

  if (huffman_table->table_type == QCCENTHUFFMAN_DECODETABLE)
    {
      if (QccENTHuffmanTableCreateDecodeTable(huffman_table))
        {
          QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccENTHuffmanTableCreateDecodeTable()");
          goto Error;
        }
    }
  else
    {
      if (QccENTHuffmanTableCreateEncodeTable(huffman_table))
        {
          QccErrorAddMessage("(QccENTHuffmanDesign): Error calling QccENTHuffmanTableCreateEncodeTable()");
          goto Error;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccENTHuffmanDesignFreeTree(huffman_tree.start);
  return(return_value);
}
