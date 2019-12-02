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


#ifndef LIBQCCPACKVQ_H
#define LIBQCCPACKVQ_H


/*  codebook.c  */
#define QCCVQCODEBOOK_MAGICNUM "CBK"
typedef double *QccVQCodeword;
typedef QccVQCodeword *QccVQCodewordArray;
typedef struct 
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_codewords;
  int codeword_dimension;
  int index_length;
  QccVQCodewordArray codewords;
  QccVector codeword_probs;
  QccVector codeword_codelengths;
} QccVQCodebook;
int QccVQCodebookInitialize(QccVQCodebook *codebook);
int QccVQCodebookAlloc(QccVQCodebook *codebook);
void QccVQCodebookFree(QccVQCodebook *codebook);
int QccVQCodebookRealloc(QccVQCodebook *codebook,
                         int new_num_codewords);
int QccVQCodebookCopy(QccVQCodebook *codebook1,
                      const QccVQCodebook *codebook2);
int QccVQCodebookMoveCodewordToFront(QccVQCodebook *codebook, int index);
int QccVQCodebookSetProbsFromPartitions(QccVQCodebook *codebook,
                                        const int *partition,
                                        int num_partitions);
int QccVQCodebookSetCodewordLengths(QccVQCodebook *codebook);
int QccVQCodebookSetIndexLength(QccVQCodebook *codebook);
int QccVQCodebookReadHeader(FILE *infile, QccVQCodebook *codebook);
int QccVQCodebookReadData(FILE *infile, QccVQCodebook *codebook);
int QccVQCodebookRead(QccVQCodebook *codebook);
int QccVQCodebookWriteHeader(FILE *outfile, const QccVQCodebook *codebook);
int QccVQCodebookWriteData(FILE *outfile, const QccVQCodebook *codebook);
int QccVQCodebookWrite(const QccVQCodebook *codebook);
int QccVQCodebookPrint(const QccVQCodebook *codebook);
int QccVQCodebookCreateRandomCodebook(QccVQCodebook *codebook, 
                                      double max, double min);
int QccVQCodebookAddCodeword(QccVQCodebook *codebook,
                             QccVQCodeword codeword);


/*  multistage_codebook  */
#define QCCVQMULTISTAGECODEBOOK_MAGICNUM "MSC"
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_codebooks;
  QccVQCodebook *codebooks;
} QccVQMultiStageCodebook;
int QccVQMultiStageCodebookInitialize(QccVQMultiStageCodebook
                                      *multistage_codebook);
int QccVQMultiStageCodebookAllocCodebooks(QccVQMultiStageCodebook 
                                          *multistage_codebook);
void QccVQMultiStageCodebookFreeCodebooks(QccVQMultiStageCodebook
                                          *multistage_codebook);
int QccVQMultiStageCodebookReadHeader(FILE *infile, 
                                      QccVQMultiStageCodebook 
                                      *multistage_codebook);
int QccVQMultiStageCodebookRead(QccVQMultiStageCodebook *multistage_codebook);
int QccVQMultiStageCodebookWriteHeader(FILE *outfile, 
                                       const QccVQMultiStageCodebook 
                                       *multistage_codebook);
int QccVQMultiStageCodebookWrite(const
                                 QccVQMultiStageCodebook *multistage_codebook);
int QccVQMultiStageCodebookPrint(const
                                 QccVQMultiStageCodebook *multistage_codebook);
int QccVQMultiStageCodebookToCodebook(const QccVQMultiStageCodebook
                                      *multistage_codebook,
                                      QccVQCodebook *codebook);


/*  sqcbk.o  */
int QccVQScalarQuantizerToVQCodebook(const
                                     QccSQScalarQuantizer *scalar_quantizer,
                                     QccVQCodebook *codebook);


/*  vq.c  */
typedef double (*QccVQDistortionMeasure)(const QccVector vector1,
                                         const QccVector vector2,
                                         int vector_dimension,
                                         int index);
int QccVQVectorQuantizeVector(const QccVector vector,
                              const QccVQCodebook *codebook,
                              double *distortion,
                              int *partition,
                              QccVQDistortionMeasure distortion_measure);
int QccVQVectorQuantization(const QccDataset *dataset,
                            const QccVQCodebook *codebook,
                            QccVector distortion,
                            int *partition,
                            QccVQDistortionMeasure distortion_measure);
int QccVQInverseVectorQuantization(const int *partition, 
                                   const QccVQCodebook *codebook, 
                                   QccDataset *dataset);


/*  ecvq.c  */
int QccVQEntropyConstrainedVQ(const QccDataset *dataset,
                              const QccVQCodebook *codebook,
                              double lambda,
                              QccVector distortion,
                              int *partition,
                              QccVQDistortionMeasure distortion_measure);
int QccVQEntropyConstrainedVQTraining(const QccDataset *dataset,
                                      QccVQCodebook *codebook,
                                      double lambda,
                                      int num_iterations,
                                      double threshold,
                                      QccVQDistortionMeasure
                                      distortion_measure);


/*  gla.c  */
typedef int (*QccVQGeneralizedLloydCentroids)(const QccDataset *dataset,
                                              QccVQCodebook *codebook,
                                              const int *partition);
int QccVQGeneralizedLloydVQTraining(const QccDataset *dataset,
                                    QccVQCodebook *codebook,
                                    int num_iterations,
                                    double threshold,
                                    QccVQDistortionMeasure distortion_measure,
                                    QccVQGeneralizedLloydCentroids
                                    centroid_calculation,
                                    int verbose);

/*  msvq.c  */
int QccVQMultiStageVQTraining(const QccDataset *dataset,
                              QccVQMultiStageCodebook *multistage_codebook,
                              int num_iterations,
                              double threshold,
                              QccVQDistortionMeasure distortion_measure,
                              QccVQGeneralizedLloydCentroids
                              centroid_calculation,
                              int verbose);
int QccVQMultiStageVQEncode(const QccDataset *dataset,
                            const QccVQMultiStageCodebook *multistage_codebook,
                            int *partition,
                            QccVQDistortionMeasure distortion_measure);
int QccVQMultiStageVQDecode(const int *partition, 
                            const
                            QccVQMultiStageCodebook *multistage_codebook, 
                            QccDataset *dataset,
                            int num_stages_to_decode);


#endif /* LIBQCCPACKVQ_H */
