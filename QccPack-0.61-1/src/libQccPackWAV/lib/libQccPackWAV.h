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


#ifndef LIBQCCPACKWAV_H
#define LIBQCCPACKWAV_H


/*  filter_bank.c  */
#define QCCWAVFILTERBANK_MAGICNUM "FBK"
#define QCCWAVFILTERBANK_DEFAULT "CohenDaubechiesFeauveau.9-7.fbk"
#define QCCWAVFILTERBANK_ORTHOGONAL 0
#define QCCWAVFILTERBANK_BIORTHOGONAL 1
#define QCCWAVFILTERBANK_GENERAL 2
#define QCCWAVFILTERBANK_PHASE_EVEN 0
#define QCCWAVFILTERBANK_PHASE_ODD 1
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int orthogonality;
  QccFilter lowpass_analysis_filter;
  QccFilter highpass_analysis_filter;
  QccFilter lowpass_synthesis_filter;
  QccFilter highpass_synthesis_filter;
} QccWAVFilterBank;
int QccWAVFilterBankInitialize(QccWAVFilterBank *filter_bank);
int QccWAVFilterBankAlloc(QccWAVFilterBank *filter_bank);
void QccWAVFilterBankFree(QccWAVFilterBank *filter_bank);
int QccWAVFilterBankSetFilterTapIncrement(QccWAVFilterBank *filter_bank,
                                          int filter_tap_increment);
int QccWAVFilterBankPrint(const QccWAVFilterBank *filter_bank);
int QccWAVFilterBankMakeOrthogonal(QccWAVFilterBank *filter_bank,
                                   const QccFilter *primary_filter);
int QccWAVFilterBankMakeBiorthogonal(QccWAVFilterBank *filter_bank,
                                     const QccFilter *primary_filter,
                                     const QccFilter *dual_filter);
int QccWAVFilterBankBiorthogonal(const QccWAVFilterBank *filter_bank);
int QccWAVFilterBankRead(QccWAVFilterBank *filter_bank);
int QccWAVFilterBankWrite(const QccWAVFilterBank *filter_bank);
int QccWAVFilterBankAnalysis(QccVector signal,
                             int signal_length,
                             int phase,
                             const QccWAVFilterBank *filter_bank,
                             int boundary_extension);
int QccWAVFilterBankSynthesis(QccVector signal,
                              int signal_length,
                              int phase,
                              const QccWAVFilterBank *filter_bank,
                              int boundary_extension);
int QccWAVFilterBankRedundantAnalysis(const QccVector input_signal,
                                      QccVector output_signal_low,
                                      QccVector output_signal_high,
                                      int signal_length,
                                      const QccWAVFilterBank *filter_bank,
                                      int boundary_extension);
int QccWAVFilterBankRedundantSynthesis(const QccVector input_signal_low,
                                       const QccVector input_signal_high,
                                       QccVector output_signal,
                                       int signal_length,
                                       const QccWAVFilterBank *filter_bank,
                                       int boundary_extension);


/*  vector_filter.c  */
#define QCCWAVVECTORFILTER_CAUSAL 0
#define QCCWAVVECTORFILTER_ANTICAUSAL 1
typedef struct
{
  int dimension;
  int length;
  int causality;
  QccMatrix *coefficients;
} QccWAVVectorFilter;
int QccWAVVectorFilterInitialize(QccWAVVectorFilter *vector_filter);
int QccWAVVectorFilterAlloc(QccWAVVectorFilter *vector_filter);
void QccWAVVectorFilterFree(QccWAVVectorFilter *vector_filter);
int QccWAVVectorFilterCopy(QccWAVVectorFilter *vector_filter1,
                           const QccWAVVectorFilter *vector_filter2,
                           int transpose);
int QccWAVVectorFilterReversal(const QccWAVVectorFilter *vector_filter1,
                               QccWAVVectorFilter *vector_filter2);
int QccWAVVectorFilterRead(FILE *infile,
                           QccWAVVectorFilter *vector_filter);
int QccWAVVectorFilterWrite(FILE *outfile,
                            const QccWAVVectorFilter *vector_filter);
int QccWAVVectorFilterPrint(const QccWAVVectorFilter *vector_filter);


/*  vector_filter_bank.c  */
#define QCCWAVVECTORFILTERBANK_MAGICNUM "VFB"
#define QCCWAVVECTORFILTERBANK_DEFAULT "LebrunVetterli.Balanced.2x2.7.vfb"
#define QCCWAVVECTORFILTERBANK_ORTHOGONAL 0
#define QCCWAVVECTORFILTERBANK_BIORTHOGONAL 1
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int orthogonality;
  QccWAVVectorFilter lowpass_analysis_filter;
  QccWAVVectorFilter highpass_analysis_filter;
  QccWAVVectorFilter lowpass_synthesis_filter;
  QccWAVVectorFilter highpass_synthesis_filter;
} QccWAVVectorFilterBank;
int QccWAVVectorFilterBankInitialize(QccWAVVectorFilterBank
                                     *vector_filter_bank);
int QccWAVVectorFilterBankAlloc(QccWAVVectorFilterBank *vector_filter_bank);
void QccWAVVectorFilterBankFree(QccWAVVectorFilterBank *vector_filter_bank);
int QccWAVVectorFilterBankMakeOrthogonal(QccWAVVectorFilterBank
                                         *vector_filter_bank,
                                         const QccWAVVectorFilter
                                         *lowpass_vector_filter,
                                         const QccWAVVectorFilter
                                         *highpass_vector_filter);
int QccWAVVectorFilterBankMakeBiorthogonal(QccWAVVectorFilterBank
                                           *vector_filter_bank,
                                           const QccWAVVectorFilter
                                           *primary_lowpass_vector_filter,
                                           const QccWAVVectorFilter
                                           *primary_highpass_vector_filter,
                                           const QccWAVVectorFilter
                                           *dual_lowpass_vector_filter,
                                           const QccWAVVectorFilter
                                           *dual_highpass_vector_filter);
int QccWAVVectorFilterBankRead(QccWAVVectorFilterBank *vector_filter_bank);
int QccWAVVectorFilterBankWrite(const QccWAVVectorFilterBank
                                *vector_filter_bank);
int QccWAVVectorFilterBankPrint(const QccWAVVectorFilterBank
                                *vector_filter_bank);
int QccWAVVectorFilterBankAnalysis(const QccDataset *input_signal,
                                   QccDataset *output_signal,
                                   const QccWAVVectorFilterBank
                                   *vector_filter_bank);
int QccWAVVectorFilterBankSynthesis(const QccDataset *input_signal,
                                    QccDataset *output_signal,
                                    const QccWAVVectorFilterBank
                                    *vector_filter_bank);


/*  lazy_wavelet.c  */
int QccWAVWaveletLWT(const QccVector input_signal,
                     QccVector output_signal,
                     int signal_length,
                     int signal_origin,
                     int subsample_pattern);
int QccWAVWaveletInverseLWT(const QccVector input_signal,
                            QccVector output_signal,
                            int signal_length,
                            int signal_origin,
                            int subsample_pattern);
int QccWAVWaveletLWT2D(const QccMatrix input_matrix,
                       QccMatrix output_matrix,
                       int num_rows,
                       int num_cols,
                       int origin_row,
                       int origin_col,
                       int subsample_pattern_row,
                       int subsample_pattern_col);
int QccWAVWaveletInverseLWT2D(const QccMatrix input_matrix,
                              QccMatrix output_matrix,
                              int num_rows,
                              int num_cols,
                              int origin_row,
                              int origin_col,
                              int subsample_pattern_row,
                              int subsample_pattern_col);
int QccWAVWaveletLWT3D(const QccVolume input_volume,
                       QccVolume output_volume,
                       int num_frames,
                       int num_rows,
                       int num_cols,
                       int origin_frame,
                       int origin_row,
                       int origin_col,
                       int subsample_pattern_frame,
                       int subsample_pattern_row,
                       int subsample_pattern_col);
int QccWAVWaveletInverseLWT3D(const QccVolume input_volume,
                              QccVolume output_volume,
                              int num_frames,
                              int num_rows,
                              int num_cols,
                              int origin_frame,
                              int origin_row,
                              int origin_col,
                              int subsample_pattern_frame,
                              int subsample_pattern_row,
                              int subsample_pattern_col);


/*  lazy_wavelet_int.c  */
int QccWAVWaveletLWTInt(const QccVectorInt input_signal,
                        QccVectorInt output_signal,
                        int signal_length,
                        int signal_origin,
                        int subsample_pattern);
int QccWAVWaveletInverseLWTInt(const QccVectorInt input_signal,
                               QccVectorInt output_signal,
                               int signal_length,
                               int signal_origin,
                               int subsample_pattern);
int QccWAVWaveletLWT2DInt(const QccMatrixInt input_matrix,
                          QccMatrixInt output_matrix,
                          int num_rows,
                          int num_cols,
                          int origin_row,
                          int origin_col,
                          int subsample_pattern_row,
                          int subsample_pattern_col);
int QccWAVWaveletInverseLWT2DInt(const QccMatrixInt input_matrix,
                                 QccMatrixInt output_matrix,
                                 int num_rows,
                                 int num_cols,
                                 int origin_row,
                                 int origin_col,
                                 int subsample_pattern_row,
                                 int subsample_pattern_col);
int QccWAVWaveletLWT3DInt(const QccVolumeInt input_volume,
                          QccVolumeInt output_volume,
                          int num_frames,
                          int num_rows,
                          int num_cols,
                          int origin_frame,
                          int origin_row,
                          int origin_col,
                          int subsample_pattern_frame,
                          int subsample_pattern_row,
                          int subsample_pattern_col);
int QccWAVWaveletInverseLWT3DInt(const QccVolumeInt input_volume,
                                 QccVolumeInt output_volume,
                                 int num_frames,
                                 int num_rows,
                                 int num_cols,
                                 int origin_frame,
                                 int origin_row,
                                 int origin_col,
                                 int subsample_pattern_frame,
                                 int subsample_pattern_row,
                                 int subsample_pattern_col);


/*  lifting_scheme.c  */
#define QCCWAVLIFTINGSCHEME_MAGICNUM "LFT"
#define QCCWAVLIFTINGSCHEME_PATH_ENV "QCCPACK_WAVELET_PATH"
#define QCCWAVLIFTINGSCHEME_LWT 0
#define QCCWAVLIFTINGSCHEME_Daubechies4 1
#define QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau9_7 2
#define QCCWAVLIFTINGSCHEME_CohenDaubechiesFeauveau5_3 3
#define QCCWAVLIFTINGSCHEME_IntLWT 4
#define QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau9_7 5
#define QCCWAVLIFTINGSCHEME_IntCohenDaubechiesFeauveau5_3 6
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int scheme;
} QccWAVLiftingScheme;
int QccWAVLiftingSchemeInitialize(QccWAVLiftingScheme *lifting_scheme);
int QccWAVLiftingSchemePrint(const QccWAVLiftingScheme *lifting_scheme);
int QccWAVLiftingSchemeRead(QccWAVLiftingScheme *lifting_scheme);
int QccWAVLiftingSchemeBiorthogonal(const QccWAVLiftingScheme *lifting_scheme);
int QccWAVLiftingSchemeInteger(const QccWAVLiftingScheme *lifting_scheme);


/*  lifting.c  */
int QccWAVLiftingAnalysis(QccVector signal,
                          int signal_length,
                          int phase,
                          const QccWAVLiftingScheme *lifting_scheme,
                          int boundary);
int QccWAVLiftingSynthesis(QccVector signal,
                           int signal_length,
                           int phase,
                           const QccWAVLiftingScheme *lifting_scheme,
                           int boundary);
int QccWAVLiftingRedundantAnalysis(const QccVector input_signal,
                                   QccVector output_signal_low,
                                   QccVector output_signal_high,
                                   int signal_length,
                                   const QccWAVLiftingScheme *lifting_scheme,
                                   int boundary);
int QccWAVLiftingRedundantSynthesis(const QccVector input_signal_low,
                                    const QccVector input_signal_high,
                                    QccVector output_signal,
                                    int signal_length,
                                    const QccWAVLiftingScheme *lifting_scheme,
                                    int boundary);


/*  lifting_daubechies4.c  */
int QccWAVLiftingAnalysisDaubechies4(QccVector signal,
                                     int signal_length,
                                     int phase,
                                     int boundary);
int QccWAVLiftingSynthesisDaubechies4(QccVector signal,
                                      int signal_length,
                                      int phase,
                                      int boundary);


/*  lifting_cdf9_7.c  */
int QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7(QccVector signal,
                                                    int signal_length,
                                                    int phase,
                                                    int boundary);
int QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7(QccVector signal,
                                                     int signal_length,
                                                     int phase,
                                                     int boundary);


/*  lifting_cdf5_3.c  */
int QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3(QccVector signal,
                                                    int signal_length,
                                                    int phase,
                                                    int boundary);
int QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3(QccVector signal,
                                                     int signal_length,
                                                     int phase,
                                                     int boundary);


/*  lifting_int.c  */
#define QccWAVLiftingIntRound(v, a, b) (((((a)*(v))>>(b))+1)>>1)
int QccWAVLiftingAnalysisInt(QccVectorInt signal,
                             int signal_length,
                             int phase,
                             const QccWAVLiftingScheme *lifting_scheme,
                             int boundary);
int QccWAVLiftingSynthesisInt(QccVectorInt signal,
                              int signal_length,
                              int phase,
                              const QccWAVLiftingScheme *lifting_scheme,
                              int boundary);


/*  lifting_int_cdf9_7.c  */
int QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7(QccVectorInt signal,
                                                       int signal_length,
                                                       int phase,
                                                       int boundary);
int QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7(QccVectorInt signal,
                                                        int signal_length,
                                                        int phase,
                                                        int boundary);


/*  lifting_int_cdf5_3.c  */
int QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3(QccVectorInt signal,
                                                       int signal_length,
                                                       int phase,
                                                       int boundary);
int QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3(QccVectorInt signal,
                                                        int signal_length,
                                                        int phase,
                                                        int boundary);


/*  wavelet.c  */
#define QCCWAVWAVELET_PATH_ENV "QCCPACK_WAVELET_PATH"
#define QCCWAVWAVELET_DEFAULT_WAVELET "CohenDaubechiesFeauveau.9-7.lft"
#define QCCWAVWAVELET_IMPLEMENTATION_FILTERBANK 0
#define QCCWAVWAVELET_IMPLEMENTATION_LIFTED 1
#define QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION \
QCCFILTER_SYMMETRIC_EXTENSION
#define QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION QCCFILTER_PERIODIC_EXTENSION
#define QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET \
(QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION + 1)
#define QCCWAVWAVELET_PHASE_EVEN QCCWAVFILTERBANK_PHASE_EVEN
#define QCCWAVWAVELET_PHASE_ODD QCCWAVFILTERBANK_PHASE_ODD
typedef struct
{
  int implementation;
  int boundary;
  QccWAVLiftingScheme lifting_scheme;
  QccWAVFilterBank filter_bank;
} QccWAVWavelet;
int QccWAVInit();
int QccWAVWaveletInitialize(QccWAVWavelet *wavelet);
int QccWAVWaveletAlloc(QccWAVWavelet *wavelet);
void QccWAVWaveletFree(QccWAVWavelet *wavelet);
void QccWAVWaveletPrint(const QccWAVWavelet *wavelet);
int QccWAVWaveletCreate(QccWAVWavelet *wavelet,
                        const QccString wavelet_filename,
                        const QccString boundary);
int QccWAVWaveletBiorthogonal(const QccWAVWavelet *wavelet);


/*  wavelet_analysis_synthesis.c  */
int QccWAVWaveletAnalysis1D(QccVector signal,
                            int signal_length,
                            int phase,
                            const QccWAVWavelet *wavelet);
int QccWAVWaveletSynthesis1D(QccVector signal,
                             int signal_length,
                             int phase,
                             const QccWAVWavelet *wavelet);
int QccWAVWaveletAnalysis2D(QccMatrix matrix,
                            int num_rows,
                            int num_cols,
                            int phase_row,
                            int phase_col,
                            const QccWAVWavelet *wavelet);
int QccWAVWaveletSynthesis2D(QccMatrix matrix,
                             int num_rows,
                             int num_cols,
                             int phase_row,
                             int phase_col,
                             const QccWAVWavelet *wavelet);
int QccWAVWaveletAnalysis3D(QccVolume volume,
                            int num_frames,
                            int num_rows,
                            int num_cols,
                            int phase_frame,
                            int phase_row,
                            int phase_col,
                            const QccWAVWavelet *wavelet);
int QccWAVWaveletSynthesis3D(QccVolume volume,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             int phase_frame,
                             int phase_row,
                             int phase_col,
                             const QccWAVWavelet *wavelet);


/*  wavelet_analysis_synthesis_int.c  */
int QccWAVWaveletAnalysis1DInt(QccVectorInt signal,
                               int signal_length,
                               int phase,
                               const QccWAVWavelet *wavelet);
int QccWAVWaveletSynthesis1DInt(QccVectorInt signal,
                                int signal_length,
                                int phase,
                                const QccWAVWavelet *wavelet);
int QccWAVWaveletAnalysis2DInt(QccMatrixInt matrix,
                               int num_rows,
                               int num_cols,
                               int phase_row,
                               int phase_col,
                               const QccWAVWavelet *wavelet);
int QccWAVWaveletSynthesis2DInt(QccMatrixInt matrix,
                                int num_rows,
                                int num_cols,
                                int phase_row,
                                int phase_col,
                                const QccWAVWavelet *wavelet);
int QccWAVWaveletAnalysis3DInt(QccVolumeInt volume,
                               int num_frames,
                               int num_rows,
                               int num_cols,
                               int phase_frame,
                               int phase_row,
                               int phase_col,
                               const QccWAVWavelet *wavelet);
int QccWAVWaveletSynthesis3DInt(QccVolumeInt volume,
                                int num_frames,
                                int num_rows,
                                int num_cols,
                                int phase_frame,
                                int phase_row,
                                int phase_col,
                                const QccWAVWavelet *wavelet);


/*  multiwavelet.c  */
#define QCCWAVWAVELET_DEFAULT_MULTIWAVELET "LebrunVetterli.Balanced.2x2.7.vfb"
typedef struct
{
  QccWAVVectorFilterBank vector_filter_bank;
} QccWAVMultiwavelet;
int QccWAVMultiwaveletInitialize(QccWAVMultiwavelet *multiwavelet);
int QccWAVMultiwaveletAlloc(QccWAVMultiwavelet *multiwavelet);
void QccWAVMultiwaveletFree(QccWAVMultiwavelet *multiwavelet);
int QccWAVMultiwaveletCreate(QccWAVMultiwavelet *multiwavelet,
                             const QccString multiwavelet_filename);
void QccWAVMultiwaveletPrint(const QccWAVMultiwavelet *multiwavelet);


/*  dwt.c  */
int QccWAVWaveletDWTSubbandLength(int original_length, int level,
                                  int highband, int signal_origin,
                                  int subsample_pattern);
int QccWAVWaveletDWT1D(QccVector signal,
                       int signal_length,
                       int signal_origin,
                       int subsample_pattern,
                       int num_scales,
                       const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseDWT1D(QccVector signal,
                              int signal_length,
                              int origin,
                              int subsample_pattern,
                              int num_scales,
                              const QccWAVWavelet *wavelet);
int QccWAVWaveletDWT2D(QccMatrix matrix,
                       int num_rows,
                       int num_cols,
                       int origin_row,
                       int origin_col,
                       int subsample_pattern_row,
                       int subsample_pattern_col,
                       int num_scales,
                       const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseDWT2D(QccMatrix matrix,
                              int num_rows,
                              int num_cols,
                              int origin_row,
                              int origin_col,
                              int subsample_pattern_row,
                              int subsample_pattern_col,
                              int num_scales,
                              const QccWAVWavelet *wavelet);
int QccWAVWaveletDyadicDWT3D(QccVolume volume,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             int origin_frame,
                             int origin_row,
                             int origin_col,
                             int subsample_pattern_frame,
                             int subsample_pattern_row,
                             int subsample_pattern_col,
                             int num_scales,
                             const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseDyadicDWT3D(QccVolume volume,
                                    int num_frames,
                                    int num_rows,
                                    int num_cols,
                                    int origin_frame,
                                    int origin_row,
                                    int origin_col,
                                    int subsample_pattern_frame,
                                    int subsample_pattern_row,
                                    int subsample_pattern_col,
                                    int num_scales,
                                    const QccWAVWavelet *wavelet);
int QccWAVWaveletPacketDWT3D(QccVolume volume,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             int origin_frame,
                             int origin_row,
                             int origin_col,
                             int subsample_pattern_frame,
                             int subsample_pattern_row,
                             int subsample_pattern_col,
                             int temporal_num_scales,
                             int spatial_num_scales,
                             const QccWAVWavelet *wavelet);
int QccWAVWaveletInversePacketDWT3D(QccVolume volume,
                                    int num_frames,
                                    int num_rows,
                                    int num_cols,
                                    int origin_frame,
                                    int origin_row,
                                    int origin_col,
                                    int subsample_pattern_frame,
                                    int subsample_pattern_row,
                                    int subsample_pattern_col,
                                    int temporal_num_scales,
                                    int spatial_num_scales,
                                    const QccWAVWavelet *wavelet);


/*  dwt_int.c  */
int QccWAVWaveletDWT1DInt(QccVectorInt signal,
                          int signal_length,
                          int signal_origin,
                          int subsample_pattern,
                          int num_scales,
                          const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseDWT1DInt(QccVectorInt signal,
                                 int signal_length,
                                 int origin,
                                 int subsample_pattern,
                                 int num_scales,
                                 const QccWAVWavelet *wavelet);
int QccWAVWaveletDWT2DInt(QccMatrixInt matrix,
                          int num_rows,
                          int num_cols,
                          int origin_row,
                          int origin_col,
                          int subsample_pattern_row,
                          int subsample_pattern_col,
                          int num_scales,
                          const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseDWT2DInt(QccMatrixInt matrix,
                                 int num_rows,
                                 int num_cols,
                                 int origin_row,
                                 int origin_col,
                                 int subsample_pattern_row,
                                 int subsample_pattern_col,
                                 int num_scales,
                                 const QccWAVWavelet *wavelet);
int QccWAVWaveletDyadicDWT3DInt(QccVolumeInt volume,
                                int num_frames,
                                int num_rows,
                                int num_cols,
                                int origin_frame,
                                int origin_row,
                                int origin_col,
                                int subsample_pattern_frame,
                                int subsample_pattern_row,
                                int subsample_pattern_col,
                                int num_scales,
                                const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseDyadicDWT3DInt(QccVolumeInt volume,
                                       int num_frames,
                                       int num_rows,
                                       int num_cols,
                                       int origin_frame,
                                       int origin_row,
                                       int origin_col,
                                       int subsample_pattern_frame,
                                       int subsample_pattern_row,
                                       int subsample_pattern_col,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet);
int QccWAVWaveletPacketDWT3DInt(QccVolumeInt volume,
                                int num_frames,
                                int num_rows,
                                int num_cols,
                                int origin_frame,
                                int origin_row,
                                int origin_col,
                                int subsample_pattern_frame,
                                int subsample_pattern_row,
                                int subsample_pattern_col,
                                int temporal_num_scales,
                                int spatial_num_scales,
                                const QccWAVWavelet *wavelet);
int QccWAVWaveletInversePacketDWT3DInt(QccVolumeInt volume,
                                       int num_frames,
                                       int num_rows,
                                       int num_cols,
                                       int origin_frame,
                                       int origin_row,
                                       int origin_col,
                                       int subsample_pattern_frame,
                                       int subsample_pattern_row,
                                       int subsample_pattern_col,
                                       int temporal_num_scales,
                                       int spatial_num_scales,
                                       const QccWAVWavelet *wavelet);


/*  sadwt.c  */
int QccWAVWaveletShapeAdaptiveDWT1D(QccVector signal,
                                    QccVector mask,
                                    int signal_length,
                                    int num_scales,
                                    const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptiveDWT1D(QccVector signal,
                                           QccVector mask,
                                           int signal_length,
                                           int num_scales,
                                           const QccWAVWavelet *wavelet);
int QccWAVWaveletShapeAdaptiveDWT2D(QccMatrix signal, 
                                    QccMatrix mask, 
                                    int num_rows,
                                    int num_cols,
                                    int num_scales,
                                    const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptiveDWT2D(QccMatrix signal, 
                                           QccMatrix mask, 
                                           int num_rows,
                                           int num_cols,
                                           int num_scales,
                                           const QccWAVWavelet *wavelet);
int QccWAVWaveletShapeAdaptiveDyadicDWT3D(QccVolume signal,
                                          QccVolume mask,
                                          int num_frames,
                                          int num_rows,
                                          int num_cols,
                                          int spatial_num_scales,
                                          const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptiveDyadicDWT3D(QccVolume signal,
                                                 QccVolume mask,
                                                 int num_frames,
                                                 int num_rows,
                                                 int num_cols,
                                                 int spatial_num_scales,
                                                 const QccWAVWavelet *wavelet);
int QccWAVWaveletShapeAdaptivePacketDWT3D(QccVolume signal,
                                          QccVolume mask,
                                          int num_frames,
                                          int num_rows,
                                          int num_cols,
                                          int temporal_num_scales,
                                          int spatial_num_scales,
                                          const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptivePacketDWT3D(QccVolume signal,
                                                 QccVolume mask,
                                                 int num_frames,
                                                 int num_rows,
                                                 int num_cols,
                                                 int temporal_num_scales,
                                                 int spatial_num_scales,
                                                 const QccWAVWavelet *wavelet);


/*  sadwt_int.c  */
int QccWAVWaveletShapeAdaptiveDWT1DInt(QccVectorInt signal,
                                       QccVectorInt mask,
                                       int signal_length,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptiveDWT1DInt(QccVectorInt signal,
                                              QccVectorInt mask,
                                              int signal_length,
                                              int num_scales,
                                              const QccWAVWavelet *wavelet);
int QccWAVWaveletShapeAdaptiveDWT2DInt(QccMatrixInt signal, 
                                       QccMatrixInt mask, 
                                       int num_rows,
                                       int num_cols,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptiveDWT2DInt(QccMatrixInt signal, 
                                              QccMatrixInt mask, 
                                              int num_rows,
                                              int num_cols,
                                              int num_scales,
                                              const QccWAVWavelet *wavelet);
int QccWAVWaveletShapeAdaptiveDyadicDWT3DInt(QccVolumeInt signal,
                                             QccVolumeInt mask,
                                             int num_frames,
                                             int num_rows,
                                             int num_cols,
                                             int spatial_num_scales,
                                             const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptiveDyadicDWT3DInt(QccVolumeInt signal,
                                                    QccVolumeInt mask,
                                                    int num_frames,
                                                    int num_rows,
                                                    int num_cols,
                                                    int spatial_num_scales,
                                                    const QccWAVWavelet
                                                    *wavelet);
int QccWAVWaveletShapeAdaptivePacketDWT3DInt(QccVolumeInt signal,
                                             QccVolumeInt mask,
                                             int num_frames,
                                             int num_rows,
                                             int num_cols,
                                             int temporal_num_scales,
                                             int spatial_num_scales,
                                             const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseShapeAdaptivePacketDWT3DInt(QccVolumeInt signal,
                                                    QccVolumeInt mask,
                                                    int num_frames,
                                                    int num_rows,
                                                    int num_cols,
                                                    int temporal_num_scales,
                                                    int spatial_num_scales,
                                                    const QccWAVWavelet
                                                    *wavelet);


/*  dmwt.c  */
int QccWAVMultiwaveletDMWT1D(const QccDataset *input_signal,
                             QccDataset *output_signal,
                             int num_scales,
                             const QccWAVMultiwavelet *multiwavelet);
int QccWAVMultiwaveletInverseDMWT1D(const QccDataset *input_signal,
                                    QccDataset *output_signal,
                                    int num_scales,
                                    const QccWAVMultiwavelet *multiwavelet);


/*  rdwt.c  */
int QccWAVWaveletRedundantDWT1D(const QccVector input_signal,
                                QccMatrix output_signals,
                                int signal_length,
                                int num_scales,
                                const QccWAVWavelet *wavelet);
void QccWAVWaveletRedundantDWT1DSubsampleGetPattern(int subband,
                                                    int num_scales,
                                                    int pattern,
                                                    int *index_skip,
                                                    int *start_index,
                                                    const QccWAVWavelet
                                                    *wavelet);
int QccWAVWaveletRedundantDWT1DSubsample(const QccMatrix input_signals,
                                         QccVector output_signal,
                                         int signal_length,
                                         int num_scales,
                                         int subsample_pattern,
                                         const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseRedundantDWT1D(const QccMatrix input_signals,
                                       QccVector output_signal,
                                       int signal_length,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet);
QccMatrix *QccWAVWaveletRedundantDWT2DAlloc(int num_rows,
                                            int num_cols,
                                            int num_scales);
void QccWAVWaveletRedundantDWT2DFree(QccMatrix *rdwt,
                                     int num_rows,
                                     int num_scales);
int QccWAVWaveletRedundantDWT2D(const QccMatrix input_matrix,
                                QccMatrix *output_matrices,
                                int num_rows,
                                int num_cols,
                                int num_scales,
                                const QccWAVWavelet *wavelet);
void QccWAVWaveletRedundantDWT2DSubsampleGetPattern(int subband,
                                                    int num_scales,
                                                    int pattern_row,
                                                    int pattern_col,
                                                    int *index_skip,
                                                    int *offset_row,
                                                    int *offset_col,
                                                    const QccWAVWavelet
                                                    *wavelet);
int QccWAVWaveletRedundantDWT2DSubsample(const QccMatrix *input_matrices,
                                         QccMatrix output_matrix,
                                         int num_rows,
                                         int num_cols,
                                         int num_scales,
                                         int subsample_pattern_row,
                                         int subsample_pattern_col,
                                         const QccWAVWavelet *wavelet);
int QccWAVWaveletInverseRedundantDWT2D(const QccMatrix *input_matrices,
                                       QccMatrix output_matrix,
                                       int num_rows,
                                       int num_cols,
                                       int num_scales,
                                       const QccWAVWavelet *wavelet);
int QccWAVWaveletRedundantDWT2DCorrelationMask(const QccMatrix *input_matrices,
                                               QccMatrix output_matrix,
                                               int num_rows,
                                               int num_cols,
                                               int num_scales,
                                               int start_scale,
                                               int stop_scale);


/*  subband_pyramid.c  */
#define QCCWAVSUBBANDPYRAMID_MAGICNUM "SBP"

typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_levels;
  int num_rows;
  int num_cols;
  int origin_row;
  int origin_col;
  int subsample_pattern_row;
  int subsample_pattern_col;
  QccMatrix matrix;
} QccWAVSubbandPyramid;

int QccWAVSubbandPyramidInitialize(QccWAVSubbandPyramid *subband_pyramid);
int QccWAVSubbandPyramidAlloc(QccWAVSubbandPyramid *subband_pyramid);
void QccWAVSubbandPyramidFree(QccWAVSubbandPyramid *subband_pyramid);
int QccWAVSubbandPyramidNumLevelsToNumSubbands(int num_levels);
int QccWAVSubbandPyramidNumSubbandsToNumLevels(int num_subbands);
int QccWAVSubbandPyramidCalcLevelFromSubband(int subband, int num_levels);
int QccWAVSubbandPyramidSubbandSize(const QccWAVSubbandPyramid
                                    *subband_pyramid,
                                    int subband,
                                    int *subband_num_rows,
                                    int *subband_num_cols);
int QccWAVSubbandPyramidSubbandOffsets(const QccWAVSubbandPyramid
                                       *subband_pyramid,
                                       int subband,
                                       int *row_offset,
                                       int *col_offset);
void QccWAVSubbandPyramidSubbandToQccString(int subband, int num_levels,
                                            QccString qccstring);
int QccWAVSubbandPyramidPrint(const QccWAVSubbandPyramid *subband_pyramid);
int QccWAVSubbandPyramidRead(QccWAVSubbandPyramid *subband_pyramid);
int QccWAVSubbandPyramidWrite(const QccWAVSubbandPyramid *subband_pyramid);
int QccWAVSubbandPyramidCopy(QccWAVSubbandPyramid *subband_pyramid1,
                             const QccWAVSubbandPyramid *subband_pyramid2);
int QccWAVSubbandPyramidZeroSubband(QccWAVSubbandPyramid *subband_pyramid,
                                    int subband);
int QccWAVSubbandPyramidSubtractMean(QccWAVSubbandPyramid *subband_pyramid,
                                     double *mean,
                                     const QccSQScalarQuantizer *quantizer);
int QccWAVSubbandPyramidAddMean(QccWAVSubbandPyramid *subband_pyramid,
                                double mean);
int QccWAVSubbandPyramidCalcCoefficientRange(const QccWAVSubbandPyramid
                                             *subband_pyramid,
                                             const QccWAVWavelet *wavelet,
                                             int level,
                                             double *max_value,
                                             double *min_value);
int QccWAVSubbandPyramidAddNoiseSquare(QccWAVSubbandPyramid *subband_pyramid,
                                       int subband,
                                       double start,
                                       double width,
                                       double noise_variance);
int QccWAVSubbandPyramidSplitToImageComponent(const QccWAVSubbandPyramid
                                              *subband_pyramid,
                                              QccIMGImageComponent
                                              *output_images);
int QccWAVSubbandPyramidSplitToDat(const QccWAVSubbandPyramid *subband_pyramid,
                                   QccDataset *output_datasets,
                                   const int *vector_dimensions);
int QccWAVSubbandPyramidAssembleFromImageComponent(const QccIMGImageComponent
                                                   *input_images,
                                                   QccWAVSubbandPyramid
                                                   *subband_pyramid);
int QccWAVSubbandPyramidAssembleFromDat(const QccDataset *input_datasets,
                                        QccWAVSubbandPyramid *subband_pyramid,
                                        const int *vector_dimensions);
int QccWAVSubbandPyramidRasterScan(const QccWAVSubbandPyramid *subband_pyramid,
                                   QccVector scanned_coefficients);
int QccWAVSubbandPyramidInverseRasterScan(QccWAVSubbandPyramid
                                          *subband_pyramid,
                                          const QccVector
                                          scanned_coefficients);
int QccWAVSubbandPyramidDWT(QccWAVSubbandPyramid *subband_pyramid,
                            int num_levels,
                            const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidInverseDWT(QccWAVSubbandPyramid *subband_pyramid,
                                   const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidShapeAdaptiveDWT(QccWAVSubbandPyramid *subband_pyramid,
                                         QccWAVSubbandPyramid *mask,
                                         int num_levels,
                                         const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidInverseShapeAdaptiveDWT(QccWAVSubbandPyramid
                                                *subband_pyramid,
                                                QccWAVSubbandPyramid *mask,
                                                const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidRedundantDWTSubsample(const QccMatrix *input_matrices,
                                              QccWAVSubbandPyramid
                                              *subband_pyramid,
                                              const QccWAVWavelet *wavelet);


/*  subband_pyramid_int.c  */
#define QCCWAVSUBBANDPYRAMIDINT_MAGICNUM "SBPI"
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_levels;
  int num_rows;
  int num_cols;
  int origin_row;
  int origin_col;
  int subsample_pattern_row;
  int subsample_pattern_col;
  QccMatrixInt matrix;
} QccWAVSubbandPyramidInt;
int QccWAVSubbandPyramidIntInitialize(QccWAVSubbandPyramidInt
                                      *subband_pyramid);
int QccWAVSubbandPyramidIntAlloc(QccWAVSubbandPyramidInt *subband_pyramid);
void QccWAVSubbandPyramidIntFree(QccWAVSubbandPyramidInt *subband_pyramid);
int QccWAVSubbandPyramidIntNumLevelsToNumSubbands(int num_levels);
int QccWAVSubbandPyramidIntNumSubbandsToNumLevels(int num_subbands);
int QccWAVSubbandPyramidIntCalcLevelFromSubband(int subband, int num_levels);
int QccWAVSubbandPyramidIntSubbandSize(const QccWAVSubbandPyramidInt
                                       *subband_pyramid,
                                       int subband,
                                       int *subband_num_rows,
                                       int *subband_num_cols);
int QccWAVSubbandPyramidIntSubbandOffsets(const QccWAVSubbandPyramidInt
                                          *subband_pyramid,
                                          int subband,
                                          int *row_offset,
                                          int *col_offset);
void QccWAVSubbandPyramidIntSubbandToQccString(int subband, int num_levels,
                                               QccString qccstring);
int QccWAVSubbandPyramidIntPrint(const QccWAVSubbandPyramidInt
                                 *subband_pyramid);
int QccWAVSubbandPyramidIntRead(QccWAVSubbandPyramidInt *subband_pyramid);
int QccWAVSubbandPyramidIntWrite(const QccWAVSubbandPyramidInt
                                 *subband_pyramid);
int QccWAVSubbandPyramidIntCopy(QccWAVSubbandPyramidInt *subband_pyramid1,
                                const QccWAVSubbandPyramidInt
                                *subband_pyramid2);
int QccWAVSubbandPyramidIntZeroSubband(QccWAVSubbandPyramidInt
                                       *subband_pyramid,
                                       int subband);
int QccWAVSubbandPyramidIntSubtractMean(QccWAVSubbandPyramidInt
                                        *subband_pyramid,
                                        int *mean);
int QccWAVSubbandPyramidIntAddMean(QccWAVSubbandPyramidInt *subband_pyramid,
                                   int mean);
int QccWAVSubbandPyramidIntRasterScan(const QccWAVSubbandPyramidInt
                                      *subband_pyramid,
                                      QccVectorInt scanned_coefficients);
int QccWAVSubbandPyramidIntInverseRasterScan(QccWAVSubbandPyramidInt
                                             *subband_pyramid,
                                             const QccVectorInt
                                             scanned_coefficients);
int QccWAVSubbandPyramidIntDWT(QccWAVSubbandPyramidInt *subband_pyramid,
                               int num_levels,
                               const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidIntInverseDWT(QccWAVSubbandPyramidInt *subband_pyramid,
                                      const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidIntShapeAdaptiveDWT(QccWAVSubbandPyramidInt
                                            *subband_pyramid,
                                            QccWAVSubbandPyramidInt *mask,
                                            int num_levels,
                                            const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramidIntInverseShapeAdaptiveDWT(QccWAVSubbandPyramidInt
                                                   *subband_pyramid,
                                                   QccWAVSubbandPyramidInt
                                                   *mask,
                                                   const QccWAVWavelet
                                                   *wavelet);


/*  samask.c  */
int QccWAVShapeAdaptiveMaskBAR(const QccMatrix mask,
                               int num_rows,
                               int num_cols,
                               double *bar_value);
int QccWAVShapeAdaptiveMaskBoundingBox(const QccWAVSubbandPyramid *mask,
                                       int subband,
                                       int *bounding_box_origin_row,
                                       int *bounding_box_origin_col,
                                       int *bounding_box_num_rows,
                                       int *bounding_box_num_cols);


/*  perceptual_weights.c  */
#define QCCWAVPERCEPTUALWEIGHTS_MAGICNUM "PCP"
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_subbands;
  QccVector perceptual_weights;
} QccWAVPerceptualWeights;
int QccWAVPerceptualWeightsInitialize(QccWAVPerceptualWeights 
                                      *perceptual_weights);
int QccWAVPerceptualWeightsAlloc(QccWAVPerceptualWeights *perceptual_weights);
void QccWAVPerceptualWeightsFree(QccWAVPerceptualWeights *perceptual_weights);
int QccWAVPerceptualWeightsRead(QccWAVPerceptualWeights *perceptual_weights);
int QccWAVPerceptualWeightsWrite(const QccWAVPerceptualWeights
                                 *perceptual_weights);
int QccWAVPerceptualWeightsPrint(const QccWAVPerceptualWeights
                                 *perceptual_weights);
int QccWAVPerceptualWeightsApply(QccWAVSubbandPyramid *subband_pyramid,
                                 const QccWAVPerceptualWeights 
                                 *perceptual_weights);
int QccWAVPerceptualWeightsRemove(QccWAVSubbandPyramid 
                                  *subband_pyramid,
                                  const QccWAVPerceptualWeights 
                                  *perceptual_weights);


/*  subband_pyramid_dwt_sq.c  */
int QccWAVSubbandPyramidDWTSQEncode(const QccIMGImageComponent *image,
                                    QccChannel *channels,
                                    const QccSQScalarQuantizer *quantizer,
                                    double *image_mean,
                                    int num_levels,
                                    const QccWAVWavelet *wavelet,
                                    const QccWAVPerceptualWeights
                                    *perceptual_weights);
int QccWAVSubbandPyramidDWTSQDecode(const QccChannel *channels,
                                    QccIMGImageComponent *image,
                                    const QccSQScalarQuantizer *quantizer,
                                    double image_mean,
                                    int num_levels,
                                    const QccWAVWavelet *wavelet,
                                    const QccWAVPerceptualWeights
                                    *perceptual_weights);


/*  zerotree.c  */
#define QCCWAVZEROTREE_MAGICNUM "ZT"
#define QCCWAVZEROTREE_SYMBOLSIGNIFICANT    1
#define QCCWAVZEROTREE_SYMBOLINSIGNIFICANT  2   /* isolated zero */
#define QCCWAVZEROTREE_SYMBOLZTROOT         3
#define QCCWAVZEROTREE_SYMBOLTEMP           4
#define QCCWAVZEROTREE_NUMSYMBOLS           4
#define QCCWAVZEROTREE_WHOLETREE -1
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int image_num_cols;
  int image_num_rows;
  double image_mean;
  int num_levels;               /* num_subbands = 3*num_levels + 1 */
  int num_subbands;
  int alphabet_size;
  int *num_cols;                /* 0:(num_subbands - 1) */
  int *num_rows;                /* 0:(num_subbands - 1) */
  char ***zerotree;             /* 0:(num_subbands - 1) */
} QccWAVZerotree;
int QccWAVZerotreeInitialize(QccWAVZerotree *zerotree);
int QccWAVZerotreeCalcSizes(const QccWAVZerotree *zerotree);
int QccWAVZerotreeAlloc(QccWAVZerotree *zerotree);
void QccWAVZerotreeFree(QccWAVZerotree *zerotree);
int QccWAVZerotreePrint(const QccWAVZerotree *zerotree);
int QccWAVZerotreeNullSymbol(const char symbol);
int QccWAVZerotreeMakeSymbolNull(char *symbol);
int QccWAVZerotreeMakeSymbolNonnull(char *symbol);
int QccWAVZerotreeMakeFullTree(QccWAVZerotree *zerotree);
int QccWAVZerotreeMakeEmptyTree(QccWAVZerotree *zerotree);
int QccWAVZerotreeRead(QccWAVZerotree *zerotree);
int QccWAVZerotreeWrite(const QccWAVZerotree *zerotree);
int QccWAVZerotreeCarveOutZerotree(QccWAVZerotree *zerotree,
                                   int subband, int row, int col);
int QccWAVZerotreeUndoZerotree(QccWAVZerotree *zerotree,
                               int subband, int row, int col);


/*  subband_pyramid3D.c  */
#define QCCWAVSUBBANDPYRAMID3D_MAGICNUM "SPBT"
#define QCCWAVSUBBANDPYRAMID3D_DYADIC 0
#define QCCWAVSUBBANDPYRAMID3D_PACKET 1
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int transform_type;
  int temporal_num_levels;
  int spatial_num_levels;
  int num_frames;
  int num_rows;
  int num_cols;
  int origin_frame;
  int origin_row;
  int origin_col;
  int subsample_pattern_frame;
  int subsample_pattern_row;
  int subsample_pattern_col;
  QccVolume volume;
} QccWAVSubbandPyramid3D;
int QccWAVSubbandPyramid3DInitialize(QccWAVSubbandPyramid3D
                                     *subband_pyramid);
int QccWAVSubbandPyramid3DAlloc(QccWAVSubbandPyramid3D *subband_pyramid);
void QccWAVSubbandPyramid3DFree(QccWAVSubbandPyramid3D *subband_pyramid);
int QccWAVSubbandPyramid3DNumLevelsToNumSubbandsDyadic(int num_levels);
int QccWAVSubbandPyramid3DNumLevelsToNumSubbandsPacket(int temporal_num_levels,
                                                       int spatial_num_levels);
int QccWAVSubbandPyramid3DNumSubbandsToNumLevelsDyadic(int num_subbands);
int QccWAVSubbandPyramid3DCalcLevelFromSubbandDyadic(int subband,
                                                     int num_levels);
int QccWAVSubbandPyramid3DCalcLevelFromSubbandPacket(int subband,
                                                     int temporal_num_levels,
                                                     int spatial_num_levels,
                                                     int *temporal_level,
                                                     int *spatial_level);
int QccWAVSubbandPyramid3DSubbandSize(const QccWAVSubbandPyramid3D
                                      *subband_pyramid,
                                      int subband,
                                      int *subband_num_frames,
                                      int *subband_num_rows,
                                      int *subband_num_cols);
int QccWAVSubbandPyramid3DSubbandOffsets(const QccWAVSubbandPyramid3D
                                         *subband_pyramid,
                                         int subband,
                                         int *frame_offset,
                                         int *row_offset,
                                         int *col_offset);
int QccWAVSubbandPyramid3DPrint(const QccWAVSubbandPyramid3D
                                *subband_pyramid);
int QccWAVSubbandPyramid3DRead(QccWAVSubbandPyramid3D *subband_pyramid);
int QccWAVSubbandPyramid3DWrite(const QccWAVSubbandPyramid3D
                                *subband_pyramid);
int QccWAVSubbandPyramid3DCopy(QccWAVSubbandPyramid3D *subband_pyramid1,
                               const QccWAVSubbandPyramid3D *subband_pyramid2);
int QccWAVSubbandPyramid3DZeroSubband(QccWAVSubbandPyramid3D
                                      *subband_pyramid,
                                      int subband);
int QccWAVSubbandPyramid3DSubtractMean(QccWAVSubbandPyramid3D
                                       *subband_pyramid,
                                       double *mean,
                                       const QccSQScalarQuantizer *quantizer);
int QccWAVSubbandPyramid3DAddMean(QccWAVSubbandPyramid3D *subband_pyramid,
                                  double mean);
int QccWAVSubbandPyramid3DDWT(QccWAVSubbandPyramid3D *subband_pyramid,
                              int transform_type,
                              int temporal_num_levels,
                              int spatial_num_levels,
                              const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramid3DInverseDWT(QccWAVSubbandPyramid3D *subband_pyramid,
                                     const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramid3DShapeAdaptiveDWT(QccWAVSubbandPyramid3D
                                           *subband_pyramid,
                                           QccWAVSubbandPyramid3D *mask,
                                           int transform_type,
                                           int temporal_num_levels,
                                           int spatial_num_levels,
                                           const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramid3DInverseShapeAdaptiveDWT(QccWAVSubbandPyramid3D
                                                  *subband_pyramid,
                                                  QccWAVSubbandPyramid3D *mask,
                                                  const QccWAVWavelet
                                                  *wavelet);
int QccWAVSubbandPyramid3DPacketToDyadic(QccWAVSubbandPyramid3D
                                         *subband_pyramid,
                                         int num_levels);
int QccWAVSubbandPyramid3DDyadicToPacket(QccWAVSubbandPyramid3D
                                         *subband_pyramid,
                                         int temporal_num_levels,
                                         int spatial_num_levels);


/*  subband_pyramid3D_int.c  */
#define QCCWAVSUBBANDPYRAMID3DINT_MAGICNUM "SBPTI"
#define QCCWAVSUBBANDPYRAMID3DINT_DYADIC QCCWAVSUBBANDPYRAMID3D_DYADIC
#define QCCWAVSUBBANDPYRAMID3DINT_PACKET QCCWAVSUBBANDPYRAMID3D_PACKET
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int transform_type;
  int temporal_num_levels;
  int spatial_num_levels;
  int num_frames;
  int num_rows;
  int num_cols;
  int origin_frame;
  int origin_row;
  int origin_col;
  int subsample_pattern_frame;
  int subsample_pattern_row;
  int subsample_pattern_col;
  QccVolumeInt volume;
} QccWAVSubbandPyramid3DInt;
int QccWAVSubbandPyramid3DIntInitialize(QccWAVSubbandPyramid3DInt
                                        *subband_pyramid);
int QccWAVSubbandPyramid3DIntAlloc(QccWAVSubbandPyramid3DInt *subband_pyramid);
void QccWAVSubbandPyramid3DIntFree(QccWAVSubbandPyramid3DInt *subband_pyramid);
int QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsDyadic(int num_levels);
int QccWAVSubbandPyramid3DIntNumLevelsToNumSubbandsPacket(int
                                                          temporal_num_levels,
                                                          int
                                                          spatial_num_levels);
int QccWAVSubbandPyramid3DIntNumSubbandsToNumLevelsDyadic(int num_subbands);
int QccWAVSubbandPyramid3DIntCalcLevelFromSubbandDyadic(int subband,
                                                        int num_levels);
int QccWAVSubbandPyramid3DIntCalcLevelFromSubbandPacket(int subband,
                                                        int
                                                        temporal_num_levels,
                                                        int spatial_num_levels,
                                                        int *temporal_level,
                                                        int *spatial_level);
int QccWAVSubbandPyramid3DIntSubbandSize(const QccWAVSubbandPyramid3DInt
                                         *subband_pyramid,
                                         int subband,
                                         int *subband_num_frames,
                                         int *subband_num_rows,
                                         int *subband_num_cols);
int QccWAVSubbandPyramid3DIntSubbandOffsets(const QccWAVSubbandPyramid3DInt
                                            *subband_pyramid,
                                            int subband,
                                            int *frame_offset,
                                            int *row_offset,
                                            int *col_offset);
int QccWAVSubbandPyramid3DIntPrint(const QccWAVSubbandPyramid3DInt
                                   *subband_pyramid);
int QccWAVSubbandPyramid3DIntRead(QccWAVSubbandPyramid3DInt *subband_pyramid);
int QccWAVSubbandPyramid3DIntWrite(const QccWAVSubbandPyramid3DInt
                                   *subband_pyramid);
int QccWAVSubbandPyramid3DIntCopy(QccWAVSubbandPyramid3DInt *subband_pyramid1,
                                  const QccWAVSubbandPyramid3DInt
                                  *subband_pyramid2);
int QccWAVSubbandPyramid3DIntZeroSubband(QccWAVSubbandPyramid3DInt
                                         *subband_pyramid,
                                         int subband);
int QccWAVSubbandPyramid3DIntSubtractMean(QccWAVSubbandPyramid3DInt
                                          *subband_pyramid,
                                          int *mean);
int QccWAVSubbandPyramid3DIntAddMean(QccWAVSubbandPyramid3DInt
                                     *subband_pyramid,
                                     int mean);
int QccWAVSubbandPyramid3DIntDWT(QccWAVSubbandPyramid3DInt *subband_pyramid,
                                 int transform_type,
                                 int temporal_num_levels,
                                 int spatial_num_levels,
                                 const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramid3DIntInverseDWT(QccWAVSubbandPyramid3DInt
                                        *subband_pyramid,
                                        const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramid3DIntShapeAdaptiveDWT(QccWAVSubbandPyramid3DInt
                                              *subband_pyramid,
                                              QccWAVSubbandPyramid3DInt *mask,
                                              int transform_type,
                                              int temporal_num_levels,
                                              int spatial_num_levels,
                                              const QccWAVWavelet *wavelet);
int QccWAVSubbandPyramid3DIntInverseShapeAdaptiveDWT(QccWAVSubbandPyramid3DInt
                                                     *subband_pyramid,
                                                     QccWAVSubbandPyramid3DInt
                                                     *mask,
                                                     const QccWAVWavelet
                                                     *wavelet);
int QccWAVSubbandPyramid3DIntPacketToDyadic(QccWAVSubbandPyramid3DInt
                                            *subband_pyramid,
                                            int num_levels);
int QccWAVSubbandPyramid3DIntDyadicToPacket(QccWAVSubbandPyramid3DInt
                                            *subband_pyramid,
                                            int temporal_num_levels,
                                            int spatial_num_levels);


/*  sfq.c  */
#define QCCWAVSFQ_MEANQUANTIZER_STEPSIZE 1.0
#define QCCWAVSFQ_BASEBANDQUANTIZER_START_STEPSIZE 7.6
#define QCCWAVSFQ_BASEBANDQUANTIZER_END_STEPSIZE 32.0
#define QCCWAVSFQ_BASEBANDQUANTIZER_INCREMENT 0.1
#define QCCWAVSFQ_HIGHPASSQUANTIZER_START_STEPSIZE 7.6
#define QCCWAVSFQ_HIGHPASSQUANTIZER_END_STEPSIZE 32.0
#define QCCWAVSFQ_HIGHPASSQUANTIZER_INCREMENT 0.1
#define QCCWAVSFQ_PREDICTOR 3
#define QCCWAVSFQ_ZEROTREE_STARTSUBBAND 1
int QccWAVsfqWaveletAnalysis(const QccIMGImageComponent *input_image,
                             QccWAVSubbandPyramid *subband_pyramid,
                             QccWAVZerotree *zerotree,
                             const QccSQScalarQuantizer *mean_quantizer,
                             const QccWAVWavelet *wavelet,
                             const QccWAVPerceptualWeights *perceptual_weights,
                             double lambda);
int QccWAVsfqBasebandEncode(const QccWAVSubbandPyramid *subband_pyramid,
                            QccWAVZerotree *zerotree,
                            QccSQScalarQuantizer *baseband_quantizer,
                            QccChannel *channels,
                            double lambda,
                            QccIMGImageComponent *reconstructed_baseband);
int QccWAVsfqHighpassEncode(const QccWAVSubbandPyramid *subband_pyramid,
                            QccWAVZerotree *zerotree,
                            QccSQScalarQuantizer *highpass_quantizer,
                            QccChannel *channels,
                            double lambda,
                            int *high_tree_thresholds,
                            int *low_tree_thresholds,
                            QccIMGImageComponent *reconstructed_baseband);
int QccWAVsfqDecode(QccBitBuffer *input_buffer,
                    QccIMGImageComponent *output_image,
                    QccWAVZerotree *zerotree,
                    const QccSQScalarQuantizer *baseband_quantizer,
                    const QccSQScalarQuantizer *highpass_quantizer,
                    int *high_tree_thresholds,
                    int *low_tree_thresholds,
                    QccChannel *channels,
                    const QccWAVWavelet *wavelet,
                    const QccWAVPerceptualWeights *perceptual_weights);
int QccWAVsfqAssembleBitstreamHeader(const QccWAVZerotree *zerotree,
                                     const QccSQScalarQuantizer
                                     *mean_quantizer,
                                     const QccSQScalarQuantizer
                                     *baseband_quantizer,
                                     const QccSQScalarQuantizer
                                     *highpass_quantizer,
                                     const int *high_tree_thresholds,
                                     const int *low_tree_thresholds,
                                     QccBitBuffer *output_buffer,
                                     int *num_output_bits);
int QccWAVsfqDisassembleBitstreamHeader(QccWAVZerotree *zerotree,
                                        QccSQScalarQuantizer *mean_quantizer,
                                        QccSQScalarQuantizer 
                                        *baseband_quantizer,
                                        QccSQScalarQuantizer
                                        *highpass_quantizer,
                                        int *no_tree_prediction,
                                        QccBitBuffer *input_buffer);
int QccWAVsfqAssembleBitstream(const QccWAVZerotree *zerotree,
                               const QccSQScalarQuantizer *mean_quantizer,
                               const QccSQScalarQuantizer *baseband_quantizer,
                               const QccSQScalarQuantizer *highpass_quantizer,
                               const QccChannel *channels,
                               const int *high_tree_thresholds,
                               const int *low_tree_thresholds,
                               QccBitBuffer *output_buffer,
                               double *rate_header,
                               double *rate_zerotree,
                               double *rate_channels);
int QccWAVsfqDisassembleBitstreamZerotree(int num_subbands,
                                          int *high_tree_thresholds,
                                          int *low_tree_thresholds,
                                          int *zerotree_symbols,
                                          QccBitBuffer *input_buffer);
int QccWAVsfqDisassembleBitstreamChannel(QccENTArithmeticModel *model,
                                         QccWAVZerotree *zerotree, 
                                         QccChannel *channels, 
                                         int subband,
                                         QccBitBuffer *input_buffer);



/*  sr.c  */
int QccWAVsrEncode(const QccIMGImageComponent *image,
                   QccBitBuffer *buffer,
                   const QccSQScalarQuantizer *quantizer,
                   int num_levels,
                   const QccWAVWavelet *wavelet,
                   const QccWAVPerceptualWeights *perceptual_weights);
int QccWAVsrDecodeHeader(QccBitBuffer *buffer, 
                         int *num_levels, 
                         int *num_rows, int *num_cols,
                         QccSQScalarQuantizer *quantizer,
                         double *image_mean);
int QccWAVsrDecode(QccBitBuffer *buffer,
                   QccIMGImageComponent *image,
                   const QccSQScalarQuantizer *quantizer,
                   int num_levels,
                   const QccWAVWavelet *wavelet,
                   const QccWAVPerceptualWeights *perceptual_weights,
                   double image_mean);


/*  wdr.c  */
typedef struct
{
  int row;
  int col;
  unsigned int index;
} QccWAVwdrCoefficientBlock;
int QccWAVwdrEncodeHeader(QccBitBuffer *buffer,
                          int num_levels,
                          int num_rows,
                          int num_cols,
                          double image_mean,
                          int max_coefficient_bits);
int QccWAVwdrEncode(const QccIMGImageComponent *image,
                    const QccIMGImageComponent *mask,
                    QccBitBuffer *buffer,
                    int num_levels,
                    const QccWAVWavelet *wavelet,
                    const QccWAVPerceptualWeights *perceptual_weights,
                    int target_bit_cnt);
int QccWAVwdrDecodeHeader(QccBitBuffer *buffer,
                          int *num_levels,
                          int *num_rows,
                          int *num_cols,
                          double *image_mean,
                          int *max_coefficient_bits);
int QccWAVwdrDecode(QccBitBuffer *buffer,
                    QccIMGImageComponent *image,
                    const QccIMGImageComponent *mask,
                    int num_levels,
                    const QccWAVWavelet *wavelet,
                    const QccWAVPerceptualWeights *perceptual_weights,
                    double image_mean,
                    int max_coefficient_bits,
                    int target_bit_cnt);


/*  wdr3d.c  */
typedef struct
{
  int frame;
  int row;
  int col;
  unsigned int index;
} QccWAVwdr3DCoefficientBlock;
int QccWAVwdr3DEncodeHeader(QccBitBuffer *output_buffer,
                            int transform_type,
                            int temporal_num_levels,
                            int spatial_num_levels,
                            int num_frames,
                            int num_rows,
                            int num_cols,
                            double image_mean,
                            int max_coefficient_bits);
int QccWAVwdr3DEncode(QccIMGImageCube *image_cube,
                      QccIMGImageCube *mask,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      const QccWAVWavelet *wavelet,
                      QccBitBuffer *output_buffer,
                      int target_bit_cnt);
int QccWAVwdr3DDecodeHeader(QccBitBuffer *input_buffer,
                            int *transform_type,
                            int *temporal_num_levels,
                            int *spatial_num_levels,
                            int *num_frames,
                            int *num_rows,
                            int *num_cols,
                            double *image_mean,
                            int *max_coefficient_bits);
int QccWAVwdr3DDecode(QccBitBuffer *input_buffer,
                      QccIMGImageCube *image_cube,
                      QccIMGImageCube *mask,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      const QccWAVWavelet *wavelet,
                      double image_mean,
                      int max_coefficient_bits,
                      int target_bit_cnt);


/*  tarp.c  */
int QccWAVTarpEncodeHeader(QccBitBuffer *output_buffer, 
                           double alpha,
                           int num_levels, 
                           int num_rows, int num_cols,
                           double image_mean,
                           int max_coefficient_bits);
int QccWAVTarpEncode(const QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     double alpha,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     QccBitBuffer *output_buffer);
int QccWAVTarpDecodeHeader(QccBitBuffer *input_buffer, 
                           double *alpha,
                           int *num_levels, 
                           int *num_rows, int *num_cols,
                           double *image_mean,
                           int *max_coefficient_bits);
int QccWAVTarpDecode(QccBitBuffer *input_buffer,
                     QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     double alpha,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     double image_mean,
                     int max_coefficient_bits,
                     int target_bit_cnt);


/*  tarp3d.c  */
int QccWAVTarp3DEncode(const QccIMGImageCube *image,
                       const QccIMGImageCube *mask,
                       QccBitBuffer *buffer,
                       int transform_type,
                       int temporal_num_levels,
                       int spatial_num_levels,
                       double alpha,
                       const QccWAVWavelet *wavelet,
                       int target_bit_cnt);
int QccWAVTarp3DDecodeHeader(QccBitBuffer *buffer, 
                             int *transform_type,
                             int *temporal_num_levels, 
                             int *spatial_num_levels, 
                             int *num_frames,
                             int *num_rows,
                             int *num_cols,
                             double *image_mean,
                             int *max_coefficient_bits,
                             double *alpha);
int QccWAVTarp3DDecode(QccBitBuffer *buffer,
                       QccIMGImageCube *image,
                       const QccIMGImageCube *mask,
                       int transform_type,
                       int temporal_num_levels,
                       int spatial_num_levels,
                       double alpha,
                       const QccWAVWavelet *wavelet,
                       double image_mean,
                       int max_coefficient_bits,
                       int target_bit_cnt);


/*  tce.c  */
int QccWAVtceEncodeHeader(QccBitBuffer *output_buffer, 
                          int num_levels,
                          int num_rows,
                          int num_cols,
                          double image_mean,
                          double stepsize,
                          int max_coefficient_bits);
int QccWAVtceEncode(const QccIMGImageComponent *image,
                    int num_levels,
                    int target_bit_cnt,
                    double stepsize,
                    const QccWAVWavelet *wavelet,
                    QccBitBuffer *output_buffer);
int QccWAVtceDecodeHeader(QccBitBuffer *input_buffer, 
                          int *num_levels, 
                          int *num_rows,
                          int *num_cols,
                          double *image_mean,
                          double *stepsize,
                          int *max_coefficient_bits);
int QccWAVtceDecode(QccBitBuffer *input_buffer,
                    QccIMGImageComponent *image,
                    int num_levels,
                    const QccWAVWavelet *wavelet,
                    double image_mean,
                    double stepsize,
                    int max_coefficient_bits,
                    int target_bit_cnt);


/*  tce3d.c  */
int QccWAVtce3DEncodeHeader(QccBitBuffer *buffer, 
                            int transform_type,
                            int temporal_num_levels,
                            int spatial_num_levels,
                            int num_frames,
                            int num_rows,
                            int num_cols,
                            double image_mean,
                            int *max_coefficient_bits,
                            double alpha);
int QccWAVtce3DEncode(const QccIMGImageCube *image,
                      QccBitBuffer *buffer,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      double alpha,
                      const QccWAVWavelet *wavelet,
                      int target_bit_cnt);
int QccWAVtce3DDecodeHeader(QccBitBuffer *buffer, 
                            int *transform_type,
                            int *temporal_num_levels, 
                            int *spatial_num_levels, 
                            int *num_frames,
                            int *num_rows,
                            int *num_cols,
                            double *image_mean,
                            int *max_coefficient_bits,
                            double *alpha);
int QccWAVtce3DDecode(QccBitBuffer *buffer,
                      QccIMGImageCube *image,
                      int transform_type,
                      int temporal_num_levels,
                      int spatial_num_levels,
                      double alpha,
                      const QccWAVWavelet *wavelet,
                      double image_mean,
                      int max_coefficient_bits,
                      int target_bit_cnt);


/*  tce3d_lossless  */
int QccWAVtce3DLosslessEncode(const QccIMGImageCube *image,
                              QccBitBuffer *buffer,
                              int transform_type,
                              int temporal_num_levels,
                              int spatial_num_levels,
                              double alpha,
                              const QccWAVWavelet *wavelet);
int QccWAVtce3DLosslessDecodeHeader(QccBitBuffer *buffer, 
                                    int *transform_type,
                                    int *temporal_num_levels, 
                                    int *spatial_num_levels, 
                                    int *num_frames,
                                    int *num_rows,
                                    int *num_cols,
                                    int *image_mean,
                                    int *max_coefficient_bits,
                                    double *alpha);
int QccWAVtce3DLosslessDecode(QccBitBuffer *buffer,
                              QccIMGImageCube *image,                     
                              int transform_type,
                              int temporal_num_levels,
                              int spatial_num_levels,
                              double alpha,
                              const QccWAVWavelet *wavelet,
                              int image_mean,
                              int max_coefficient_bits,
                              int target_bit_cnt);


/*  klttce3d.c  */
int QccWAVklttce3DEncode(const QccIMGImageCube *image,
                         QccBitBuffer *buffer,
                         int num_levels,
                         double alpha,
                         const QccWAVWavelet *wavelet,
                         int target_bit_cnt);
int QccWAVklttce3DDecodeHeader(QccBitBuffer *buffer, 
                               int *num_levels, 
                               int *num_frames,
                               int *num_rows,
                               int *num_cols,
                               int *max_coefficient_bits,
                               double *alpha);
int QccWAVklttce3DDecode(QccBitBuffer *buffer,
                         QccIMGImageCube *image,
                         int num_levels,
                         double alpha,
                         const QccWAVWavelet *wavelet,
                         int max_coefficient_bits,
                         int target_bit_cnt);


/*  klttce3d_lossless.c  */
int QccWAVklttce3DLosslessEncode(const QccIMGImageCube *image,
                                 QccBitBuffer *buffer,
                                 int num_levels,
                                 double alpha,
                                 const QccWAVWavelet *wavelet);
int QccWAVklttce3DLosslessDecodeHeader(QccBitBuffer *buffer, 
                                       int *num_levels, 
                                       int *num_frames,
                                       int *num_rows,
                                       int *num_cols,
                                       int *max_coefficient_bits,
                                       double *alpha);
int QccWAVklttce3DLosslessDecode(QccBitBuffer *buffer,
                                 QccIMGImageCube *image,
                                 int num_levels,
                                 double alpha,
                                 const QccWAVWavelet *wavelet,
                                 int max_coefficient_bits,
                                 int target_bit_cnt);


/*  dcttce.c  */
int QccWAVdcttceEncodeHeader(QccBitBuffer *output_buffer, 
                             int num_levels, 
                             int num_rows,
                             int num_cols,
                             double image_mean,
                             double stepsize,
                             int max_coefficient_bits,
                             int overlap_sample,
                             double smooth_factor);
int QccWAVdcttceEncode(const QccIMGImageComponent *image,
                       int num_levels,
                       int target_bit_cnt,
                       double stepsize,
                       int overlap_sample,
                       double  smooth_factor,
                       QccBitBuffer *output_buffer);
int QccWAVdcttceDecodeHeader(QccBitBuffer *input_buffer, 
                             int *num_levels, 
                             int *num_rows,
                             int *num_cols,
                             double *image_mean,
                             double *stepsize,
                             int *max_coefficient_bits,
                             int *overlap_sample,
                             double  *smooth_factor);
int QccWAVdcttceDecode(QccBitBuffer *input_buffer,
                       QccIMGImageComponent *image,
                       int num_levels,
                       double image_mean,
                       double stepsize,
                       int max_coefficient_bits,
                       int target_bit_cnt,
                       int overlap_sample,
                       double smooth_factor);


/*  bisk.c  */
int QccWAVbiskEncodeHeader(QccBitBuffer *output_buffer, 
                           int num_levels, 
                           int num_rows,
                           int num_cols,
                           double image_mean,
                           int max_coefficient_bits);
int QccWAVbiskEncode(const QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     QccBitBuffer *output_buffer,
                     FILE *rate_distortion_file);
int QccWAVbiskEncode2(QccWAVSubbandPyramid *image_subband_pyramid,
                      QccWAVSubbandPyramid *mask_subband_pyramid,
                      double image_mean,
                      int target_bit_cnt,
                      QccBitBuffer *output_buffer,
                      FILE *rate_distortion_file);
int QccWAVbiskDecodeHeader(QccBitBuffer *input_buffer, 
                           int *num_levels, 
                           int *num_rows,
                           int *num_cols,
                           double *image_mean,
                           int *max_coefficient_bits);
int QccWAVbiskDecode(QccBitBuffer *input_buffer,
                     QccIMGImageComponent *image,
                     const QccIMGImageComponent *mask,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     double image_mean,
                     int max_coefficient_bits,
                     int target_bit_cnt);
int QccWAVbiskDecode2(QccBitBuffer *input_buffer,
                      QccWAVSubbandPyramid *image_subband_pyramid,
                      QccWAVSubbandPyramid *mask_subband_pyramid,
                      int max_coefficient_bits,
                      int target_bit_cnt);


/*  bisk3d.c  */
int QccWAVbisk3DEncodeHeader(QccBitBuffer *output_buffer,
                             int transform_type,
                             int temporal_num_levels,
                             int spatial_num_levels,
                             int num_frames,
                             int num_rows,
                             int num_cols,
                             double image_mean,
                             int max_coefficient_bits);
int QccWAVbisk3DEncode(const QccIMGImageCube *image_cube,
                       const QccIMGImageCube *mask,
                       int transform_type,
                       int temporal_num_levels,
                       int spatial_num_levels,
                       const QccWAVWavelet *wavelet,
                       QccBitBuffer *output_buffer,
                       int target_bit_cnt);
int QccWAVbisk3DEncode2(QccWAVSubbandPyramid3D *image_subband_pyramid,
                        QccWAVSubbandPyramid3D *mask_subband_pyramid,
                        double image_mean,
                        QccBitBuffer *output_buffer,
                        int target_bit_cnt);
int QccWAVbisk3DDecodeHeader(QccBitBuffer *input_buffer,
                             int *transform_type,
                             int *temporal_num_levels,
                             int *spatial_num_levels,
                             int *num_frames,
                             int *num_rows,
                             int *num_cols,
                             double *image_mean,
                             int *max_coefficient_bits);
int QccWAVbisk3DDecode(QccBitBuffer *input_buffer,
                       QccIMGImageCube *image_cube,
                       const QccIMGImageCube *mask,
                       int transform_type,
                       int temporal_num_levels,
                       int spatial_num_levels,
                       const QccWAVWavelet *wavelet,
                       double image_mean,
                       int max_coefficient_bits,
                       int target_bit_cnt);
int QccWAVbisk3DDecode2(QccBitBuffer *input_buffer,
                        QccWAVSubbandPyramid3D *image_subband_pyramid,
                        QccWAVSubbandPyramid3D *mask_subband_pyramid,
                        int max_coefficient_bits,
                        int target_bit_cnt);


#endif /* LIBQCCPACKWAV_H */
