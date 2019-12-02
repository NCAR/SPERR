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


#ifndef LIBQCCPACKENT_H
#define LIBQCCPACKENT_H



/*  entropy.c  */
double QccENTEntropy(const QccVector probs, int num_symbols);
double QccENTJointEntropy(const QccMatrix probs_array,
                          int num_symbols1, int num_symbols2);
double QccENTConditionalEntropy(const QccMatrix probs_array,
                                int num_contexts, int num_symbols);


/*  arithmetic.c  */
typedef long QccENTCodeValue;
typedef int (*QccENTArithmeticGetContext)(const int *symbol_stream,
                                          int current_symbol_index);
#define QCCENT_MAXNUMSYMBOLS   1024
#define QCCENT_MAXNUMCONTEXTS  1024
#define QCCENT_FREQUENCY_BITS  14
#define QCCENT_MAXFREQUENCY    ((1 << QCCENT_FREQUENCY_BITS) - 1)
#define QCCENT_CODEVALUEBITS   16
#define QCCENT_TOPVALUE (((QccENTCodeValue)(1 << QCCENT_CODEVALUEBITS)) - 1)
#define QCCENT_FIRSTQUARTERVALUE (QCCENT_TOPVALUE/4 + 1)
#define QCCENT_HALFVALUE (QCCENT_FIRSTQUARTERVALUE * 2)
#define QCCENT_THIRDQUARTERVALUE (QCCENT_FIRSTQUARTERVALUE * 3)
#define QCCENT_ANYNUMBITS -1
#define QCCENT_NONADAPTIVE 0
#define QCCENT_ADAPTIVE 1
typedef struct
{
  int num_contexts;
  int *num_symbols;
  int **translate_symbol_to_index;
  int **translate_index_to_symbol;
  int **cumulative_frequencies;
  int **frequencies;
  QccENTCodeValue low, high;
  long bits_to_follow;
  int garbage_bits;
  int target_num_bits;
  int adaptive_model;
  QccENTArithmeticGetContext context_function;
  int current_context;
  QccENTCodeValue current_code_value;
} QccENTArithmeticModel;
void QccENTArithmeticFreeModel(QccENTArithmeticModel *model);
int QccENTArithmeticSetModelContext(QccENTArithmeticModel *model,
                                    int current_context);
void QccENTArithmeticSetModelAdaption(QccENTArithmeticModel *model,
                                      int adaptive);
int QccENTArithmeticSetModelProbabilities(QccENTArithmeticModel *model,
                                          const double *probabilities,
                                          int context);
int QccENTArithmeticResetModel(QccENTArithmeticModel *model,
                               int context);
QccENTArithmeticModel *QccENTArithmeticEncodeStart(const int *num_symbols, 
                                                   int num_contexts,
                                                   QccENTArithmeticGetContext
                                                   context_function,
                                                   int target_num_bits);
int QccENTArithmeticEncodeEnd(QccENTArithmeticModel *model,
                              int final_context,
                              QccBitBuffer *output_buffer);
int QccENTArithmeticEncode(const int *symbol_stream,
                           int symbol_stream_length,
                           QccENTArithmeticModel *model,
                           QccBitBuffer *output_buffer);
int QccENTArithmeticEncodeFlush(QccENTArithmeticModel *model, 
                                QccBitBuffer *output_buffer);
QccENTArithmeticModel *QccENTArithmeticDecodeStart(QccBitBuffer *input_buffer,
                                                   const int *num_symbols, 
                                                   int num_contexts,
                                                   QccENTArithmeticGetContext
                                                   context_function,
                                                   int target_num_bits);
int QccENTArithmeticDecodeRestart(QccBitBuffer *input_buffer,
                                  QccENTArithmeticModel *model,
                                  int target_num_bits);
int QccENTArithmeticDecode(QccBitBuffer *input_buffer,
                           QccENTArithmeticModel *model,
                           int *symbol_stream,
                           int symbol_stream_length);


/*  arithmetic_channel.c  */
int QccENTArithmeticEncodeChannel(const QccChannel *channel,
                                  QccBitBuffer *buffer,
                                  int order);
int QccENTArithmeticDecodeChannel(QccBitBuffer *buffer, QccChannel *channel,
                                  int order);


/*  huffman_codeword.c  */
#define QCCENTHUFFMAN_MAXCODEWORDLEN 31
typedef struct
{
  int length;
  int codeword;
} QccENTHuffmanCodeword;
int QccENTHuffmanCodewordWrite(QccBitBuffer *output_buffer,
                               const QccENTHuffmanCodeword *codeword);
void QccENTHuffmanCodewordPrint(const QccENTHuffmanCodeword *codeword);


/*  huffman_table.c  */
#define QCCENTHUFFMAN_DECODETABLE 0
#define QCCENTHUFFMAN_ENCODETABLE 1
#define QCCENTHUFFMAN_MAXSYMBOL 100000
#define QCCENTHUFFMANTABLE_MAGICNUM "HUF"
typedef struct
{
  int symbol;
  QccENTHuffmanCodeword codeword;
} QccENTHuffmanTableEntry;
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int table_type;
  int table_length;
  QccENTHuffmanTableEntry *table;
  int *num_codewords_list;
  int num_codewords_list_length;
  int *symbol_list;
  int symbol_list_length;
  int codeword_max[QCCENTHUFFMAN_MAXCODEWORDLEN];
  int codeword_min[QCCENTHUFFMAN_MAXCODEWORDLEN];
  int codeword_ptr[QCCENTHUFFMAN_MAXCODEWORDLEN];
} QccENTHuffmanTable;
int QccENTHuffmanTableInitialize(QccENTHuffmanTable *table);
int QccENTHuffmanTableAlloc(QccENTHuffmanTable *table);
void QccENTHuffmanTableFree(QccENTHuffmanTable *table);
void QccENTHuffmanTablePrint(const QccENTHuffmanTable *table);
int QccENTHuffmanTableRead(QccENTHuffmanTable *table);
int QccENTHuffmanTableWrite(QccENTHuffmanTable *table);
int QccENTHuffmanTableCreateDecodeTable(QccENTHuffmanTable *table);
int QccENTHuffmanTableCreateEncodeTable(QccENTHuffmanTable *table);


/*  huffman.c  */
int QccENTHuffmanEncode(QccBitBuffer *output_buffer,
                        const int *symbols,
                        int num_symbols,
                        const QccENTHuffmanTable *table);
int QccENTHuffmanDecode(QccBitBuffer *input_buffer,
                        int *symbols,
                        int num_symbols,
                        const QccENTHuffmanTable *table);


/*  huffman_design.c  */
typedef struct
{
  int symbol;
  double probability;
  int codeword_length;
  QccListNode *left;
  QccListNode *right;
} QccENTHuffmanDesignTreeNode;
int QccENTHuffmanDesign(const int *symbols,
                        const double *probabilities,
                        int num_symbols,
                        QccENTHuffmanTable *huffman_table);


/*  huffman_channel.c  */
int QccENTHuffmanEncodeChannel(const QccChannel *channel,
                               QccBitBuffer *output_buffer,
                               QccENTHuffmanTable *huffman_table);
int QccENTHuffmanDecodeChannel(QccBitBuffer *input_buffer,
                               const QccChannel *channel,
                               const QccENTHuffmanTable *huffman_table);


/*  golomb.c  */
#define QCCENTGOLOMB_RUN_SYMBOL 0
#define QCCENTGOLOMB_STOP_SYMBOL 1
int QccENTGolombEncode(QccBitBuffer *output_buffer,
                       const int *symbols,
                       int num_symbols,
                       const float *p,
                       const int *m);
int QccENTGolombDecode(QccBitBuffer *input_buffer,
                       int *symbols,
                       int num_symbols);


/*  golomb_channel.c  */
int QccENTGolombEncodeChannel(const QccChannel *channel,
                              QccBitBuffer *output_buffer, 
                              const float *p,
                              const int *m);
int QccENTGolombDecodeChannel(QccBitBuffer *input_buffer,
                              const QccChannel *channel);


/*  adaptive_golomb.c  */
#define QCCENTADAPTIVEGOLOMB_RUN_SYMBOL 0
#define QCCENTADAPTIVEGOLOMB_STOP_SYMBOL 1
#define QCCENTADAPTIVEGOLOMB_KMAX 31
int QccENTAdaptiveGolombEncode(QccBitBuffer *output_buffer,
                               const int *symbols, 
                               int num_symbols);
int QccENTAdaptiveGolombDecode(QccBitBuffer *input_buffer,
                               int *symbols, 
                               int num_symbols);


/*  adaptive_golomb_channel.c  */
int QccENTAdaptiveGolombEncodeChannel(const QccChannel *channel,
                                      QccBitBuffer *output_buffer);
int QccENTAdaptiveGolombDecodeChannel(QccBitBuffer *input_buffer,
                                      const QccChannel *channel);


/*  exponential_golomb.c  */
int QccENTExponentialGolombEncodeSymbol(QccBitBuffer *output_buffer,
                                        int symbol,
                                        int signed_symbol);
int QccENTExponentialGolombDecodeSymbol(QccBitBuffer *input_buffer,
                                        int *symbol,
                                        int signed_symbol);
int QccENTExponentialGolombEncode(QccBitBuffer *output_buffer,
                                  const int *symbols,
                                  int num_symbols,
                                  int signed_symbols);
int QccENTExponentialGolombDecode(QccBitBuffer *input_buffer,
                                  int *symbols, 
                                  int num_symbols,
                                  int signed_symbols);


#endif /* LIBQCCPACKENT_H */


