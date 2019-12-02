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


#ifndef LIBQCCPACKECC_H
#define LIBQCCPACKECC_H


/*  field.c  */
#define QCCECCFIELD_MAX_SIZEEXPONENT 16
#define QCCECCFIELDELEMENT_DECIMAL 0
#define QCCECCFIELDELEMENT_BINARY 1
#define QCCECCFIELDELEMENT_HEX 2
#define QCCECCFIELDELEMENT_POLYNOMIAL 3
typedef unsigned int QccECCFieldElement;
typedef QccECCFieldElement **QccECCFieldMatrix;
typedef struct
{
  int size_exponent;
  QccECCFieldElement mask;
  QccECCFieldElement high_bit;
  QccECCFieldElement minimum_polynomial;
  int *log_table;
  QccECCFieldElement *exponential_table;
} QccECCField;

int QccECCFieldInitialize(QccECCField *field);
int QccECCFieldAllocate(QccECCField *field);
void QccECCFieldFree(QccECCField *field);
QccECCFieldMatrix QccECCFieldMatrixAllocate(int num_rows, int num_cols);
void QccECCFieldMatrixFree(QccECCFieldMatrix matrix, int num_rows);
void QccECCFieldElementPrint(QccECCFieldElement element,
                             const QccECCField *field,
                             int representation);
void QccECCFieldMatrixPrint(const QccECCFieldMatrix matrix,
                            int num_rows, int num_cols,
                            const QccECCField *field,
                            int representation);
void QccECCFieldPrint(const QccECCField *field);


/*  field_math.c  */
QccECCFieldElement QccECCFieldExponential(int exponent,
                                          const QccECCField *field);
int QccECCFieldLogarithm(QccECCFieldElement element, const QccECCField *field);
QccECCFieldElement QccECCFieldAdd(QccECCFieldElement element1,
                                  QccECCFieldElement element2,
                                  const QccECCField *field);
QccECCFieldElement QccECCFieldMultiply(QccECCFieldElement element1,
                                       QccECCFieldElement element2,
                                       const QccECCField *field);
QccECCFieldElement QccECCFieldDivide(QccECCFieldElement element1,
                                     QccECCFieldElement element2,
                                     const QccECCField *field);
int QccECCFieldMatrixGaussianElimination(QccECCFieldMatrix matrix,
                                         int num_rows, int num_cols,
                                         const QccECCField *field);

/*  reed_solomon.c  */
typedef struct
{
  int message_length;    /*  k  */
  int code_length;       /*  n  */
  QccECCField field;
  QccECCFieldMatrix generator_matrix;
} QccECCReedSolomonCode;

int QccECCReedSolomonCodeInitialize(QccECCReedSolomonCode *code);
int QccECCReedSolomonCodeAllocate(QccECCReedSolomonCode *code);
void QccECCReedSolomonCodeFree(QccECCReedSolomonCode *code);
void QccECCReedSolomonCodePrint(const QccECCReedSolomonCode *code);
int QccECCReedSolomonEncode(const QccECCFieldElement *message_symbols,
                            QccECCFieldElement *code_symbols,
                            const QccECCReedSolomonCode *code);
int QccECCReedSolomonDecode(const QccECCFieldElement *code_symbols,
                            QccECCFieldElement *message_symbols,
                            const int *erasure_vector,
                            const QccECCReedSolomonCode *code);


/*  crc.c  */
#define QCCECCCRC_MAGICNUM "CRC"
#define QCCECCCRC_PATH_ENV "QCCPACK_CODES_PATH"
#define QCCECCCRC_CRC32 0
#define QCCECCCRC_CRC32_WIDTH 32
#define QCCECCCRC_CRC32_POLYNOMIAL 0x04c11db7
typedef struct QccECCcrc
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int width;
  unsigned int polynomial;
  unsigned int current_state;
} QccECCcrc;
void QccECCcrcInitialize(QccECCcrc *crc);
int QccECCcrcRead(QccECCcrc *crc);
int QccECCcrcPredefined(QccECCcrc *crc, int predefined_crc_code);
int QccECCcrcPrint(const QccECCcrc *crc);
int QccECCcrcProcessStart(unsigned int initial_state, QccECCcrc *crc);
int QccECCcrcProcessBit(int bit_value, QccECCcrc *crc);
int QccECCcrcProcessBits(int val, int num_bits, QccECCcrc *crc);
int QccECCcrcProcessChar(char ch, QccECCcrc *crc);
int QccECCcrcProcessInt(int val, QccECCcrc *crc);
int QccECCcrcFlush(unsigned int *checksum, QccECCcrc *crc);
int QccECCcrcCheck(unsigned int checksum, QccECCcrc *crc);
int QccECCcrcPutChecksum(QccBitBuffer *output_buffer,
                         unsigned int checksum,
                         const QccECCcrc *crc);
int QccECCcrcGetChecksum(QccBitBuffer *input_buffer,
                         unsigned int *checksum,
                         const QccECCcrc *crc);


/*  trellis.c  */
#define QCCECCTRELLIS_INVALIDSTATE -1
typedef struct
{
  unsigned int next_state;
  unsigned int output_symbol;
} QccECCTrellisStateTableEntry;
typedef struct
{
  QccECCTrellisStateTableEntry ***state_table;
  int num_states;
  int num_input_symbols;
  int num_syndromes;
} QccECCTrellisStateTable;
void QccECCTrellisStateTableInitialize(QccECCTrellisStateTable *state_table);
int QccECCTrellisStateTableAlloc(QccECCTrellisStateTable *state_table);
void QccECCTrellisStateTableFree(QccECCTrellisStateTable *state_table);
void QccECCTrellisStateTablePrint(const QccECCTrellisStateTable *state_table);


/*  viterbi.c  */
typedef double (*QccECCViterbiErrorMetricCallback)(unsigned int output_symbol,
                                                   double modulated_symbol,
                                                   int index,
                                                   const void *callback_data);
double QccECCViterbiSquaredErrorCallback(unsigned int output_symbol,
                                         double modulated_symbol,
                                         int index,
                                         const void *callback_data);
int QccECCViterbiDecode(unsigned int *coded_message,
                        int coded_message_length,
                        const double *modulated_message,
                        QccECCViterbiErrorMetricCallback callback,
                        const void *callback_data,
                        const unsigned int *syndrome,
                        const QccECCTrellisStateTable *state_table,
                        int traceback_depth,
                        double *distance);


/*  trellis_code.c  */
#define QCCECCTRELLISCODE_MAGICNUM "TCD"
#define QCCECCTRELLISCODE_PATH_ENV "QCCPACK_CODES_PATH"
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int memory_order;
  int num_inputs;
  int num_outputs;
  unsigned int *parity_check_matrix;
} QccECCTrellisCode;
void QccECCTrellisCodeInitialize(QccECCTrellisCode *trellis_code);
int QccECCTrellisCodeAlloc(QccECCTrellisCode *trellis_code);
void QccECCTrellisCodeFree(QccECCTrellisCode *trellis_code);
int QccECCTrellisCodeRead(QccECCTrellisCode *trellis_code);
int QccECCTrellisCodeWrite(const QccECCTrellisCode *trellis_code);
int QccECCTrellisCodePrint(const QccECCTrellisCode *trellis_code);
int QccECCTrellisCodeHammingWeight(unsigned int symbol);
unsigned int QccECCTrellisCodeGetOutput(unsigned int input_symbol,
                                        unsigned int syndrome,
                                        unsigned int current_state,
                                        const QccECCTrellisCode *trellis_code);
unsigned int QccECCTrellisCodeGetNextState(unsigned int input_symbol,
                                           unsigned int current_state,
                                           const QccECCTrellisCode
                                           *trellis_code);
int QccECCTrellisCodeCreateStateTable(const QccECCTrellisCode *trellis_code,
                                      QccECCTrellisStateTable *state_table);
int QccECCTrellisCodeEncode(const unsigned int *message,
                            int message_length,
                            unsigned int *coded_message,
                            const QccECCTrellisCode *trellis_code);
int QccECCTrellisCodeSyndrome(const unsigned int *coded_message,
                              int coded_message_length,
                              unsigned int *syndrome,
                              const QccECCTrellisCode *trellis_code);


#endif /* LIBQCCPACKECC_H */
