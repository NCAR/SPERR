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


void QccListInitialize(QccList *list)
{
  list->start = NULL;
  list->end = NULL;
}


void QccListFreeNode(QccListNode *node)
{
  if (node == NULL)
    return;

  if (node->value != NULL)
    {
      QccFree(node->value);
      node->value = NULL;
    }
  QccFree(node);
}


void QccListFree(QccList *list)
{
  QccListNode *current;
  QccListNode *next;

  if (list == NULL)
    return;

  current = list->start;
  while (current != NULL)
    {
      next = current->next;
      QccListFreeNode(current);
      current = next;
    }
  list->start = NULL;
  list->end = NULL;
}


QccListNode *QccListCreateNode(int value_size, const void *value)
{
  QccListNode *new_node = NULL;

  if ((new_node = (QccListNode *)malloc(sizeof(QccListNode))) == NULL)
    {
      QccErrorAddMessage("(QccListCreateNode): Error allocating memory");
      return(NULL);
    }
  new_node->previous = NULL;
  new_node->next = NULL;
  new_node->value_size = value_size;

  if ((new_node->value = (void *)malloc(value_size)) == NULL)
    {
      QccErrorAddMessage("(QccListCreateNode): Error allocating memory");
      return(NULL);
    }

  if (value != NULL)
    memcpy(new_node->value, value, value_size);

  return(new_node);
}


QccListNode *QccListCopyNode(const QccListNode *node)
{
  QccListNode *new_node = NULL;

  if (node != NULL)
    {
      if ((new_node = QccListCreateNode(node->value_size,
                                        node->value)) == NULL)
        {
          QccErrorAddMessage("(QccListCopyNode): Error calling QccListCreateNode()");
          return(NULL);
        }
    }
  return(new_node);
}


int QccListCompareNodes(const QccListNode *node1, const QccListNode *node2)
{
  if ((node1 != NULL) && (node2 != NULL))
    if ((node1->value != NULL) && (node2->value != NULL))
      return(memcmp(node1->value, node2->value, node1->value_size));

  return(0);
}


QccListNode *QccListFindNode(const QccList *list, void *value)
{
  QccListNode *current;
  QccListNode dummy_node;

  if ((list == NULL) || (value == NULL))
    return(NULL);

  dummy_node.value = value;

  current = list->start;

  while (current != NULL)
    {
      if (!QccListCompareNodes(current, &dummy_node))
        return(current);

      current = current->next;
    }

  return(NULL);
}


int QccListLength(const QccList *list)
{
  QccListNode *current;
  int length = 0;

  if (list == NULL)
    return(length);

  current = list->start;

  while (current != NULL)
    {
      length++;
      current = current->next;
    }
  return(length);
}


int QccListAppendNode(QccList *list, QccListNode *node)
{

  if (list == NULL)
    return(0);

  if (node == NULL)
    return(0);

  if (list->start == NULL)
    {
      node->previous = NULL;
      node->next = NULL;
      list->start = node;
      list->end = node;
      return(0);
    }

  node->previous = list->end;
  node->next = NULL;
  (list->end)->next = node;
  list->end = node;

  return(0);
}


int QccListInsertNode(QccList *list,
                      QccListNode *insertion_point, 
                      QccListNode *node)
{
  QccListNode *previous;
  
  if (list == NULL)
    return(0);
  if (insertion_point == NULL)
    return(0);
  if (node == NULL)
    return(0);

  if (list->start == NULL)
    return(0);

  previous = insertion_point->previous;
  insertion_point->previous = node;
  node->next = insertion_point;
  node->previous = previous;

  if (previous == NULL)
    list->start = node;
  else
    previous->next = node;

  return(0);
}


int QccListSortedInsertNode(QccList *list,
                            QccListNode *node,
                            int sort_direction,
                            int (*compare_values)(const void *value1,
                                                  const void *value2))
{
  QccListNode *current;

  if (list == NULL)
    return(0);
  if (node == NULL)
    return(0);
  if (compare_values == NULL)
    return(0);

  current = list->start;
  while (current != NULL)
    {
      if (sort_direction == QCCLIST_SORTASCENDING)
        {
          if (compare_values(node->value, current->value) <= 0)
            break;
        }
      else
        if (compare_values(node->value, current->value) >= 0)
          break;

      current = current->next;
    }

  if (current != NULL)
    {
      if (QccListInsertNode(list, current, node))
        {
          QccErrorAddMessage("(QccListSorrtedInsertNode): Error calling QccListInsertNode()");
          return(1);
        }
    }
  else
    if (QccListAppendNode(list, node))
      {
        QccErrorAddMessage("(QccListSorrtedInsertNode): Error calling QccListAppendNode()");
        return(1);
      }

  return(0);
}


int QccListRemoveNode(QccList *list, QccListNode *node)
{
  if (list == NULL)
    return(0);
  if (node == NULL)
    return(0);

  if (node->previous == NULL)
    {
      list->start = node->next;
      if (list->start != NULL)
        (list->start)->previous = NULL;
    }
  else
    (node->previous)->next = node->next;

  if (node->next == NULL)
    list->end = node->previous;
  else
    (node->next)->previous = node->previous;

  return(0);
}


int QccListDeleteNode(QccList *list, QccListNode *node)
{
  if (QccListRemoveNode(list, node))
    {
      QccErrorAddMessage("QccListDeleteNode): Error calling QccListRemoveNode()");
      return(1);
    }

  QccListFreeNode(node);

  return(0);
}


int QccListMoveNode(QccList *list1, QccList *list2, QccListNode *node)
{
  if (list1 == NULL)
    return(0);
  if (list2 == NULL)
    return(0);
  if (node == NULL)
    return(0);

  if (list1->start == NULL)
    return(0);

  if (QccListRemoveNode(list1, node))
    {
      QccErrorAddMessage("(QccListMoveNode): Error calling QccListRemoveNode()");
      return(1);
    }

  if (QccListAppendNode(list2, node))
    {
      QccErrorAddMessage("(QccListMoveNode): Error calling QccListAppendNode()");
      return(1);
    }

  return(0);
}


int QccListSort(QccList *list, int sort_direction,
                int (*compare_values)(const void *value1,
                                      const void *value2))
{
  QccList sorted_list;
  QccListNode *current;

  if (list == NULL)
    return(0);

  QccListInitialize(&sorted_list);

  while (list->start != NULL)
    {
      current = list->start;

      if (QccListRemoveNode(list, current))
        {
          QccErrorAddMessage("(QccListSort): Error calling QccListRemoveNode()");
          return(1);
        }
      
      if (QccListSortedInsertNode(&sorted_list, current,
                                  sort_direction, compare_values))
        {
          QccErrorAddMessage("(QccListSort): Error calling QccListInsertNode()");
          return(1);
        }
    }
        
  list->start = sorted_list.start;
  list->end = sorted_list.end;

  return(0);
}


int QccListConcatenate(QccList *list1, QccList *list2)
{
  QccListNode *current;
  QccListNode *next;

  if (list1 == NULL)
    return(0);
  if (list2 == NULL)
    return(0);

  current = list2->start;
  while (current != NULL)
    {
      next = current->next;

      QccListAppendNode(list1, current);

      current = next;
    }
  
  QccListInitialize(list2);

  return(0);
}


int QccListPrint(QccList *list,
                 void (*print_value)(const void *value))
{
  QccListNode *current;
  int count = 0;

  if (list == NULL)
    return(0);
  if (print_value == NULL)
    return(0);

  current = list->start;

  printf("Linked List:\n");

  while (current != NULL)
    {
      printf("  Node %d:\n", count++);
      print_value(current->value);
      current = current->next;
    }

  return(0);
}
