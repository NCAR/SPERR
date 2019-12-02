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


#ifndef LIBQCCPACKSQ_H
#define LIBQCCPACKSQ_H


/*  scalar_quantizer.c  */
#define QCCSQSCALARQUANTIZER_MAGICNUM "SQ"
#define QCCSQSCALARQUANTIZER_GENERAL  0
#define QCCSQSCALARQUANTIZER_UNIFORM  1
#define QCCSQSCALARQUANTIZER_DEADZONE 2
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int type;
  int num_levels;
  QccVector levels;
  QccVector probs;
  QccVector boundaries;
  double stepsize;
  double deadzone;
} QccSQScalarQuantizer;
int QccSQScalarQuantizerInitialize(QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerAlloc(QccSQScalarQuantizer *scalar_quantizer);
void QccSQScalarQuantizerFree(QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerSetProbsFromPartitions(const QccSQScalarQuantizer 
                                               *scalar_quantizer,
                                               const int *partition, 
                                               int num_partitions);
int QccSQScalarQuantizerReadHeader(FILE *infile, 
                                   QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerReadData(FILE *infile, 
                                 QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerRead(QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerWriteHeader(FILE *outfile, 
                                    const
                                    QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerWriteData(FILE *outfile, 
                                  const
                                  QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerWrite(const QccSQScalarQuantizer *scalar_quantizer);
int QccSQScalarQuantizerPrint(const QccSQScalarQuantizer *scalar_quantizer);


/*  sq.c  */
#define QCCSQ_NOOVERLOAD 0
int QccSQScalarQuantization(double value,
                            const QccSQScalarQuantizer *quantizer,
                            double *distortion,
                            int *partition);
int QccSQInverseScalarQuantization(int partition,
                                   const QccSQScalarQuantizer *quantizer,
                                   double *value);
int QccSQScalarQuantizeVector(const QccVector data,
                              int data_length,
                              const QccSQScalarQuantizer *quantizer,
                              QccVector distortion,
                              int *partition);
int QccSQInverseScalarQuantizeVector(const int *partition,
                                     const QccSQScalarQuantizer *quantizer,
                                     QccVector data,
                                     int data_length);
int QccSQScalarQuantizeDataset(const QccDataset *dataset,
                               const QccSQScalarQuantizer *quantizer,
                               QccVector distortion,
                               int *partition);
int QccSQInverseScalarQuantizeDataset(const int *partition,
                                      const QccSQScalarQuantizer *quantizer,
                                      QccDataset *dataset);
int QccSQUniformMakeQuantizer(QccSQScalarQuantizer *quantizer,
                              double max_value,
                              double min_value,
                              int overload_region);
double QccSQULawExpander(double value,
                         double V, double u);
int QccSQULawMakeQuantizer(QccSQScalarQuantizer *quantizer,
                           double u,
                           double max_value,
                           double min_value);
double QccSQALawExpander(double value,
                         double V, double A);
int QccSQALawMakeQuantizer(QccSQScalarQuantizer *quantizer,
                           double A,
                           double max_value,
                           double min_value);

/*  lloyd.c  */
int QccSQLloydMakeQuantizer(QccSQScalarQuantizer *quantizer,
                            QccMathProbabilityDensity prob_density,
                            double variance,
                            double mean,
                            double max_value,
                            double min_value,
                            double stop_threshold,
                            int num_integration_intervals);

#endif /* LIBQCCPACKSQ_H */
