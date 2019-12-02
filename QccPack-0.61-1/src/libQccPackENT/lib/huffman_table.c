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


int QccENTHuffmanTableInitialize(QccENTHuffmanTable *table)
{
  if (table == NULL)
    return(0);

  table->table_type = QCCENTHUFFMAN_DECODETABLE;
  table->table = NULL;
  table->table_length = 0;

  table->num_codewords_list = NULL;
  table->num_codewords_list_length = 0;
  table->symbol_list = NULL;
  table->symbol_list_length = 0;

  return(0);
}


int QccENTHuffmanTableAlloc(QccENTHuffmanTable *table)
{
  if (table == NULL)
    return(0);

  if (table->table_length <= 0)
    return(0);

  if ((table->table =
       (QccENTHuffmanTableEntry *)malloc(sizeof(QccENTHuffmanTableEntry) *
                                         table->table_length)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableAlloc): Error allocating memory");
      return(1);
    }

  return(0);
}


void QccENTHuffmanTableFree(QccENTHuffmanTable *table)
{
  if (table == NULL)
    return;

  if (table->table != NULL)
    QccFree(table->table);
  if (table->num_codewords_list != NULL)
    QccFree(table->num_codewords_list);
  if (table->symbol_list != NULL)
    QccFree(table->symbol_list);
}


void QccENTHuffmanTablePrint(const QccENTHuffmanTable *table)
{
  int entry;
  int index;
  if (table == NULL)
    return;

  printf("Huffman Codewords:\n");

  for (entry = 0; entry < table->table_length; entry++)
    {
      printf("    Symbol: %.2x, Length: %2d, Codeword: ",
             table->table[entry].symbol,
             table->table[entry].codeword.length);
      QccENTHuffmanCodewordPrint(&table->table[entry].codeword);
    }

  printf("  Decode Table:\n");
  for (index = 0; index < QCCENTHUFFMAN_MAXCODEWORDLEN; index++)
    printf("    Length: %d, Min: %d, Max: %d, Ptr: %d\n",
           index + 1,
           table->codeword_min[index],
           table->codeword_max[index],
           table->codeword_ptr[index]);

}


static int QccENTHuffmanTableReadHeader(FILE *fileptr,
                                        QccENTHuffmanTable *table)
{
  if (QccFileReadMagicNumber(fileptr,
                             table->magic_num,
                             &table->major_version,
                             &table->minor_version))
    {
      QccErrorAddMessage("(QccENTHuffmanTableReadHeader): Error calling QccFileReadMagicNumber()");
      return(1);
    }

  if (!strcmp(table->magic_num, QCCCHANNEL_MAGICNUM))
    {
      QccErrorAddMessage("(QccENTHuffmanTableReadHeader): %s is not of Huffman table (%s) type",
                         table->filename, QCCENTHUFFMANTABLE_MAGICNUM);
      return(1);
    }

  fscanf(fileptr, "%d", &(table->num_codewords_list_length));
  if (ferror(fileptr) || feof(fileptr))
    {
      QccErrorAddMessage("(QccENTHuffmanTableReadHeader): Error reading number of codewords list length in %s",
                         table->filename);
      return(1);
    }

  fscanf(fileptr, "%d", &(table->symbol_list_length));
  if (ferror(fileptr) || feof(fileptr))
    {
      QccErrorAddMessage("(QccENTHuffmanTableReadHeader): Error reading symbol list length in %s",
                         table->filename);
      return(1);
    }

  return(0);
}


static int QccENTHuffmanTableReadData(FILE *fileptr,
                                      QccENTHuffmanTable *table)
{
  int index;

  for (index = 0; index < table->num_codewords_list_length; index++)
    {
      fscanf(fileptr, "%d", &(table->num_codewords_list[index]));
      if (ferror(fileptr) || feof(fileptr))
        {
          QccErrorAddMessage("(QccENTHuffmanTableReadData): Error reading number of codewords in %s",
                             table->filename);
          return(1);
        }
    }

  for (index = 0; index < table->symbol_list_length; index++)
    {
      fscanf(fileptr, "%d", &(table->symbol_list[index]));
      if (ferror(fileptr) || feof(fileptr))
        {
          QccErrorAddMessage("(QccENTHuffmanTableReadData): Error reading number of codewords in %s",
                             table->filename);
          return(1);
        }
    }

  return(0);
}


int QccENTHuffmanTableRead(QccENTHuffmanTable *table)
{
  FILE *fileptr = NULL;

  if (table == NULL)
    return(0);

  if ((table->table_type != QCCENTHUFFMAN_DECODETABLE) &&
      (table->table_type != QCCENTHUFFMAN_ENCODETABLE))
    {
      QccErrorAddMessage("(QccENTHuffmanTableRead): Invalid Huffman table type specified");
      return(1);
    }

  if ((fileptr = QccFileOpen(table->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableRead): Error calling QccFileOpen()");
      return(1);
    }

  if (QccENTHuffmanTableReadHeader(fileptr, table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableRead): Error calling QccENTHuffmanTableReadHeader()");
      return(1);
    }

  if ((table->num_codewords_list =
       (int *)malloc(sizeof(int) *
                     table->num_codewords_list_length)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableRead): Error allocating memory");
      return(1);
    }

  if ((table->symbol_list =
       (int *)malloc(sizeof(int) *
                     table->symbol_list_length)) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableRead): Error allocating memory");
      return(1);
    }

  if (QccENTHuffmanTableReadData(fileptr, table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableRead): Error calling QccENTHuffmanTableReadData()");
      return(1);
    }

  if (table->table_type == QCCENTHUFFMAN_DECODETABLE)
    {
      if (QccENTHuffmanTableCreateDecodeTable(table))
        {
          QccErrorAddMessage("(QccENTHuffmanTableRead): Error calling QccENTHuffmanTableCreateDecodeTable()");
          return(1);
        }
    }
  else
    {
      if (QccENTHuffmanTableCreateEncodeTable(table))
        {
          QccErrorAddMessage("(QccENTHuffmanTableRead): Error calling QccENTHuffmanTableCreateEncodeTable()");
          return(1);
        }
    }

  QccFileClose(fileptr);

  return(0);
}


static int QccENTHuffmanTableWriteHeader(FILE *fileptr,
                                         QccENTHuffmanTable *table)
{
  if (QccFileWriteMagicNumber(fileptr, QCCENTHUFFMANTABLE_MAGICNUM))
    goto Error;

  fprintf(fileptr, "%d\n%d\n",
          table->num_codewords_list_length, table->symbol_list_length);
  if (ferror(fileptr))
    goto Error;
  
  return(0);

 Error:
  QccErrorAddMessage("(QccENTHuffmanTableWriteHeader): Error writing header to %s",
                     table->filename);
  return(1);
}


static int QccENTHuffmanTableWriteData(FILE *fileptr,
                                       QccENTHuffmanTable *table)
{
  int index;

  for (index = 0; index < table->num_codewords_list_length; index++)
    fprintf(fileptr, "%d\n",
            table->num_codewords_list[index]);
  for (index = 0; index < table->symbol_list_length; index++)
    fprintf(fileptr, "%d\n",
            table->symbol_list[index]);

  return(0);
}


int QccENTHuffmanTableWrite(QccENTHuffmanTable *table)
{
  FILE *fileptr = NULL;

  if (table == NULL)
    return(0);

  if (table->num_codewords_list == NULL)
    return(0);
  if (table->symbol_list == NULL)
    return(0);

  if ((fileptr = QccFileOpen(table->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableWrite): Error calling QccFileOpen()");
      return(1);
    }

  if (QccENTHuffmanTableWriteHeader(fileptr, table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableWrite): Error calling QccENTHuffmanTableWriteHeader()");
      return(1);
    }

  if (QccENTHuffmanTableWriteData(fileptr, table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableWrite): Error calling QccENTHuffmanTableWriteData()");
      return(1);
    }

  QccFileClose(fileptr);

  return(0);
}


static int QccENTHuffmanTableFindLengths(QccENTHuffmanTable *table)
{
  int index;
  int current_length;
  int codeword;

  index = 0;
  current_length = 1;

  for (codeword = 0; codeword < table->table_length; codeword++, index++)
    {
      while (index >= table->num_codewords_list[current_length - 1])
        {
          current_length++;
          index = 0;
        }

      table->table[codeword].codeword.length = current_length;
    }

  return(0);
}


static int QccENTHuffmanTableFindCodewords(QccENTHuffmanTable *table)
{
  int current_code = 0;
  int current_size = table->table[0].codeword.length;
  int entry;

  entry = 0;
  for (;;)
    {
      table->table[entry].codeword.codeword = current_code;
      current_code++;
      entry++;
      if (entry >= table->table_length)
        break;
      while (table->table[entry].codeword.length != current_size)
        {
          current_code <<= 1;
          current_size++;
        }
    }

  return(0);
}


static int QccENTHuffmanTableCreateDecodeTables(QccENTHuffmanTable *table)
{
  int index;
  int current_entry = 0;

  for (index = 0; index < QCCENTHUFFMAN_MAXCODEWORDLEN; index++)
    {
      table->codeword_max[index] = table->codeword_min[index] =
        table->codeword_ptr[index] = -1;
    }

  for (index = 0; index < table->num_codewords_list_length; index++)
    if (table->num_codewords_list[index] > 0)
      {
        table->codeword_ptr[index] = current_entry;
        table->codeword_min[index] =
          table->table[current_entry].codeword.codeword;
        current_entry += table->num_codewords_list[index] - 1;
        table->codeword_max[index] =
          table->table[current_entry].codeword.codeword;
        current_entry++;
      }

  return(0);
}


int QccENTHuffmanTableCreateDecodeTable(QccENTHuffmanTable *table)
{
  int return_value;
  int index;

  if (table == NULL)
    return(0);

  if (table->num_codewords_list == NULL)
    return(0);
  if (table->symbol_list == NULL)
    return(0);

  if ((table->num_codewords_list_length <= 0) ||
      (table->num_codewords_list_length > QCCENTHUFFMAN_MAXCODEWORDLEN))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): length of list be between 1 and %d",
                         QCCENTHUFFMAN_MAXCODEWORDLEN);
      return(1);
    }

  table->table_length = 0;

  for (index = 0; index < table->num_codewords_list_length; index++)
    table->table_length += table->num_codewords_list[index];

  if (table->table_length != table->symbol_list_length)
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): Symbol list length (%d) inconsistent with total number of symbols (%d) implied by list of number of codewords",
                         table->symbol_list_length, table->table_length);
      goto Error;
    }

  for (index = 0; index < table->symbol_list_length; index++)
    if ((table->symbol_list[index] < 0) ||
          (table->symbol_list[index] > QCCENTHUFFMAN_MAXSYMBOL))
      {
        QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): Invalid symbol (less than 0 or greater than %d) in symbol list",
                           QCCENTHUFFMAN_MAXSYMBOL);
        return(1);
      }

  if (QccENTHuffmanTableAlloc(table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): Error calling QccENTHuffmanTableAlloc()");
      goto Error;
    }

  for (index = 0; index < table->table_length; index++)
    table->table[index].symbol = table->symbol_list[index];

  if (QccENTHuffmanTableFindLengths(table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): Error calling QccENTHuffmanTableFindLengths()");
      goto Error;
    }

  if (QccENTHuffmanTableFindCodewords(table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): Error calling QccENTHuffmanTableFindCodewords()");
      goto Error;
    }

  if (QccENTHuffmanTableCreateDecodeTables(table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateDecodeTable): Error calling QccENTHuffmanTableCreateDecodeTables()");
      goto Error;
    }

  table->table_type = QCCENTHUFFMAN_DECODETABLE;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
  QccENTHuffmanTableFree(table);
 Return:
  return(return_value);
}


int QccENTHuffmanTableCreateEncodeTable(QccENTHuffmanTable *table)
{
  int return_value;
  int entry;
  int index;
  int max_symbol = -MAXINT;
  QccENTHuffmanTable decode_table;

  QccENTHuffmanTableInitialize(&decode_table);

  if (table == NULL)
    return(0);
  if (table->num_codewords_list == NULL)
    return(0);
  if (table->symbol_list == NULL)
    return(0);

  decode_table.num_codewords_list_length =
    table->num_codewords_list_length;
  if ((decode_table.num_codewords_list =
       (int *)malloc(sizeof(int) * decode_table.num_codewords_list_length))
      == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateEncodeTable): Error allocating memory");
      goto Error;
    }
  for (index = 0; index < decode_table.num_codewords_list_length; index++)
    decode_table.num_codewords_list[index] =
      table->num_codewords_list[index];

  decode_table.symbol_list_length =
    table->symbol_list_length;
  if ((decode_table.symbol_list =
       (int *)malloc(sizeof(int) * decode_table.symbol_list_length))
      == NULL)
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateEncodeTable): Error allocating memory");
      goto Error;
    }
  for (index = 0; index < decode_table.symbol_list_length; index++)
    decode_table.symbol_list[index] =
      table->symbol_list[index];

  if (QccENTHuffmanTableCreateDecodeTable(&decode_table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateEncodeTable): Error calling QccENTHuffmanTableCreateDecodeTable()");
      goto Error;
    }

  for (index = 0; index < table->symbol_list_length; index++)
    if (table->symbol_list[index] > max_symbol)
      max_symbol = table->symbol_list[index];

  table->table_length = max_symbol + 1;
  if (QccENTHuffmanTableAlloc(table))
    {
      QccErrorAddMessage("(QccENTHuffmanTableCreateEncodeTable): Error calling QccENTHuffmanTableAlloc()");
      goto Error;
    }

  for (entry = 0; entry < table->table_length; entry++)
    {
      table->table[entry].symbol = entry;
      table->table[entry].codeword.length = -1;
    }

  for (entry = 0; entry < decode_table.table_length; entry++)
    {
      table->table[decode_table.table[entry].symbol].codeword.length =
        decode_table.table[entry].codeword.length;
      table->table[decode_table.table[entry].symbol].codeword.codeword =
        decode_table.table[entry].codeword.codeword;
    }

  table->table_type = QCCENTHUFFMAN_ENCODETABLE;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
  QccENTHuffmanTableFree(table);
 Return:
  QccENTHuffmanTableFree(&decode_table);
  return(return_value);
}
