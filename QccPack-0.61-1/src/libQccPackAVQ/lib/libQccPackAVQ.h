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


#ifndef LIBQCCPACKAVQ_H
#define LIBQCCPACKAVQ_H


/*  sideinfo.c  */
#define QCCAVQSIDEINFO_MAGICNUM "SID"
typedef struct
{
  QccString filename;
  FILE *fileptr;
  QccString magic_num;
  int major_version;
  int minor_version;
  QccString program_name;
  int max_codebook_size;
  int N;
  int vector_dimension;
  int vector_component_alphabet_size;
  QccSQScalarQuantizer codebook_coder;
} QccAVQSideInfo;
int QccAVQSideInfoInitialize(QccAVQSideInfo *sideinfo);
int QccAVQSideInfoReadHeader(QccAVQSideInfo *sideinfo);
int QccAVQSideInfoStartRead(QccAVQSideInfo *sideinfo);
int QccAVQSideInfoEndRead(QccAVQSideInfo *sideinfo);
int QccAVQSideInfoWriteHeader(const QccAVQSideInfo *sideinfo);
int QccAVQSideInfoStartWrite(QccAVQSideInfo *sideinfo);
int QccAVQSideInfoEndWrite(const QccAVQSideInfo *sideinfo);
int QccAVQSideInfoPrint(QccAVQSideInfo *sideinfo);


/*  sideinfo_symbol.c  */
#define QCCAVQSIDEINFO_NULLSYMBOL 0
#define QCCAVQSIDEINFO_FLAG 1
#define QCCAVQSIDEINFO_ADDRESS 2
#define QCCAVQSIDEINFO_VECTOR 3
#define QCCAVQSIDEINFO_FLAG_NOUPDATE 0
#define QCCAVQSIDEINFO_FLAG_UPDATE 1
typedef struct
{
  int type;
  int flag;
  int address;
  QccVector vector;
  int *vector_indices;
} QccAVQSideInfoSymbol;
int QccAVQSideInfoSymbolInitialize(QccAVQSideInfoSymbol *sideinfo_symbol);
QccAVQSideInfoSymbol *QccAVQSideInfoSymbolAlloc(int vector_dimension);
int QccAVQSideInfoSymbolFree(QccAVQSideInfoSymbol *symbol);
QccAVQSideInfoSymbol *QccAVQSideInfoSymbolRead(QccAVQSideInfo *sideinfo,
                                               QccAVQSideInfoSymbol *symbol);
QccAVQSideInfoSymbol *QccAVQSideInfoSymbolReadDesired(QccAVQSideInfo *sideinfo,
                                                      int desired_symbol,
                                                      QccAVQSideInfoSymbol
                                                      *symbol);
QccAVQSideInfoSymbol *QccAVQSideInfoSymbolReadNextFlag(QccAVQSideInfo 
                                                       *sideinfo,
                                                       QccAVQSideInfoSymbol 
                                                       *symbol);
int QccAVQSideInfoSymbolWrite(QccAVQSideInfo *sideinfo, 
                              const QccAVQSideInfoSymbol *symbol);
int QccAVQSideInfoSymbolPrint(QccAVQSideInfo *sideinfo, 
                              const QccAVQSideInfoSymbol *symbol);
int QccAVQSideInfoCodebookCoder(QccAVQSideInfo *sideinfo,
                                QccAVQSideInfoSymbol *new_codeword_symbol);


/*  gtr.c  */
int QccAVQgtrEncode(const QccDataset *dataset, 
                    QccVQCodebook *codebook,
                    QccChannel *channel,
                    QccAVQSideInfo *sideinfo,
                    double lambda,
                    double omega,
                    QccVQDistortionMeasure distortion_measure);
int QccAVQgtrDecode(QccDataset *dataset,
                    QccVQCodebook *codebook,
                    const QccChannel *channel,
                    QccAVQSideInfo *sideinfo);
int QccAVQgtrCalcRate(QccChannel *channel, 
                      QccAVQSideInfo *sideinfo,
                      FILE *outfile, 
                      double *rate, 
                      int verbose,
                      int entropy_code_sideinfo);

/*  paul.c  */
int QccAVQPaulEncode(const QccDataset *dataset,
                     QccVQCodebook *codebook,
                     QccChannel *channel,
                     QccAVQSideInfo *sideinfo,
                     double distortion_threshold,
                     QccVQDistortionMeasure distortion_measure);
int QccAVQPaulDecode(QccDataset *dataset,
                     QccVQCodebook *codebook,
                     const QccChannel *channel,
                     QccAVQSideInfo *sideinfo);
int QccAVQPaulCalcRate(QccChannel *channel, 
                       QccAVQSideInfo *sideinfo,
                       FILE *outfile, 
                       double *rate, 
                       int verbose,
                       int entropy_code_sideinfo);

/*  gy.c  */ 
#define QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS 2
int QccAVQGershoYanoEncode(const QccDataset *dataset,
                           QccVQCodebook *codebook, 
                           QccChannel *channel,
                           QccAVQSideInfo *sideinfo,
                           int adaption_interval,
                           QccVQDistortionMeasure distortion_measure,
                           QccVQGeneralizedLloydCentroids
                           centroid_calculation);
int QccAVQGershoYanoDecode(QccDataset *dataset,
                           QccVQCodebook *codebook,
                           QccChannel *channel,
                           QccAVQSideInfo *sideinfo);
int QccAVQGershoYanoCalcRate(QccChannel *channel, 
                             QccAVQSideInfo *sideinfo,
                             FILE *outfile, 
                             double *rate, 
                             int verbose,
                             int entropy_code_sideinfo);


#endif /* LIBQCCPACKAVQ_H */
