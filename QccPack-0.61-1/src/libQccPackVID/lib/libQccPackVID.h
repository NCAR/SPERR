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


#ifndef LIBQCCPACKVID_H
#define LIBQCCPACKVID_H


#define QCCVID_INTRAFRAME 0
#define QCCVID_INTERFRAME 1


/*  motion_vectors.c  */
typedef struct
{
  QccENTHuffmanTable encode_table;
  QccENTHuffmanTable decode_table;
} QccVIDMotionVectorsTable;
void QccVIDMotionVectorsTableInitialize(QccVIDMotionVectorsTable *table);
int QccVIDMotionVectorsTableCreate(QccVIDMotionVectorsTable *table);
void QccVIDMotionVectorsTableFree(QccVIDMotionVectorsTable *table);
int QccVIDMotionVectorsEncode(const QccIMGImageComponent
                              *motion_vector_horizontal,
                              const QccIMGImageComponent
                              *motion_vector_vertical,
                              const QccVIDMotionVectorsTable *mvd_table,
                              int subpixel_accuracy,
                              QccBitBuffer *output_buffer);
int QccVIDMotionVectorsDecode(QccIMGImageComponent
                              *motion_vector_horizontal,
                              QccIMGImageComponent
                              *motion_vector_vertical,
                              const QccVIDMotionVectorsTable *table,
                              int subpixel_accuracy,
                              QccBitBuffer *input_buffer);
int QccVIDMotionVectorsReadFile(QccIMGImageComponent
                                *motion_vectors_horizontal,
                                QccIMGImageComponent
                                *motion_vectors_vertical,
                                const QccString filename,
                                int frame_num);
int QccVIDMotionVectorsWriteFile(const QccIMGImageComponent
                                 *motion_vectors_horizontal,
                                 const QccIMGImageComponent
                                 *motion_vectors_vertical,
                                 const QccString filename,
                                 int frame_num);


/*  motion_estimation.c  */
#define QCCVID_ME_FULLPIXEL 0
#define QCCVID_ME_HALFPIXEL 1
#define QCCVID_ME_QUARTERPIXEL 2
#define QCCVID_ME_EIGHTHPIXEL 3
double QccVIDMotionEstimationMAE(QccMatrix current_block,
                                 QccMatrix reference_block,
                                 QccMatrix weights,
                                 int block_size);
int QccVIDMotionEstimationExtractBlock(const QccIMGImageComponent *image,
                                       double row,
                                       double col,
                                       QccMatrix block,
                                       int block_size,
                                       int subpixel_accuracy);
int QccVIDMotionEstimationInsertBlock(QccIMGImageComponent *image,
                                      double row,
                                      double col,
                                      const QccMatrix block,
                                      int block_size,
                                      int subpixel_accuracy);
int QccVIDMotionEstimationFullSearch(const QccIMGImageComponent *current_frame,
                                     const QccIMGImageComponent
                                     *reference_frame,
                                     QccIMGImageComponent
                                     *motion_vectors_horizontal,
                                     QccIMGImageComponent
                                     *motion_vectors_vertical,
                                     int block_size,
                                     int window_size,
                                     int subpixel_accuracy);
int QccVIDMotionEstimationCalcReferenceFrameSize(int num_rows,
                                                 int num_cols,
                                                 int *reference_num_rows,
                                                 int *reference_num_cols,
                                                 int subpixel_accuracy);
int QccVIDMotionEstimationCreateReferenceFrame(const QccIMGImageComponent
                                               *current_frame,
                                               QccIMGImageComponent
                                               *reference_frame,
                                               int subpixel_accuracy,
                                               const QccFilter *filter1,
                                               const QccFilter *filter2,
                                               const QccFilter *filter3);
int QccVIDMotionEstimationCreateCompensatedFrame(QccIMGImageComponent
                                                 *motion_compensated_frame,
                                                 const QccIMGImageComponent
                                                 *reference_frame,
                                                 const QccIMGImageComponent
                                                 *motion_vectors_horizontal,
                                                 const QccIMGImageComponent
                                                 *motion_vectors_vertical,
                                                 int block_size,
                                                 int subpixel_accuracy);


/*  mesh_motion_estimation.c  */
int QccVIDMeshMotionEstimationWarpMesh(const QccRegularMesh *reference_mesh,
                                       QccRegularMesh *current_mesh,
                                       const QccIMGImageComponent
                                       *motion_vectors_horizontal,
                                       const QccIMGImageComponent
                                       *motion_vectors_vertical);
int QccVIDMeshMotionEstimationSearch(const QccIMGImageComponent
                                     *current_frame,
                                     QccRegularMesh *current_mesh,
                                     const QccIMGImageComponent
                                     *reference_frame,
                                     const QccRegularMesh
                                     *reference_mesh,
                                     QccIMGImageComponent
                                     *motion_vectors_horizontal,
                                     QccIMGImageComponent
                                     *motion_vectors_vertical,
                                     int block_size,
                                     int window_size,
                                     int subpixel_accuracy,
                                     int constrained_boundary,
                                     int exponential_kernel);
int QccVIDMeshMotionEstimationCreateCompensatedFrame(QccIMGImageComponent
                                                     *motion_compensated_frame,
                                                     const QccRegularMesh
                                                     *current_mesh,
                                                     const QccIMGImageComponent
                                                     *reference_frame,
                                                     const QccRegularMesh
                                                     *reference_mesh,
                                                     int subpixel_accuracy);


/*  spatialblock.c  */
int QccVIDSpatialBlockEncode(QccIMGImageSequence *image_sequence,
                             const QccFilter *filter1,
                             const QccFilter *filter2,
                             const QccFilter *filter3,
                             int subpixel_accuracy,
                             QccBitBuffer *output_buffer,
                             int blocksize,
                             int num_levels,
                             int target_bit_cnt,
                             const QccWAVWavelet *wavelet,
                             const QccString mv_filename,
                             int read_motion_vectors,
                             int quiet);
int QccVIDSpatialBlockDecodeHeader(QccBitBuffer *input_buffer,
                                   int *num_rows,
                                   int *num_cols,
                                   int *start_frame_num,
                                   int *end_frame_num,
                                   int *blocksize,
                                   int *num_levels,
                                   int *target_bit_cnt);
int QccVIDSpatialBlockDecode(QccIMGImageSequence *image_sequence,
                             const QccFilter *filter1,
                             const QccFilter *filter2,
                             const QccFilter *filter3,
                             int subpixel_accuracy,
                             QccBitBuffer *input_buffer,
                             int target_bit_cnt,
                             int blocksize,
                             int num_levels,
                             const QccWAVWavelet *wavelet,
                             const QccString mv_filename,
                             int quiet);


/*  rdwtblock.c  */
int QccVIDRDWTBlockEncode(QccIMGImageSequence *image_sequence,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int subpixel_accuracy,
                          QccBitBuffer *output_buffer,
                          int blocksize,
                          int num_levels,
                          int target_bit_cnt,
                          const QccWAVWavelet *wavelet,
                          const QccString mv_filename,
                          int read_motion_vectors,
                          int quiet);
int QccVIDRDWTBlockDecodeHeader(QccBitBuffer *input_buffer,
                                int *num_rows,
                                int *num_cols,
                                int *start_frame_num,
                                int *end_frame_num,
                                int *blocksize,
                                int *num_levels,
                                int *target_bit_cnt);
int QccVIDRDWTBlockDecode(QccIMGImageSequence *image_sequence,
                          const QccFilter *filter1,
                          const QccFilter *filter2,
                          const QccFilter *filter3,
                          int subpixel_accuracy,
                          QccBitBuffer *input_buffer,
                          int target_bit_cnt,
                          int blocksize,
                          int num_levels,
                          const QccWAVWavelet *wavelet,
                          const QccString mv_filename,
                          int quiet);


/*  rwmh.c  */
int QccVIDRWMHEncode(QccIMGImageSequence *image_sequence,
                     const QccFilter *filter1,
                     const QccFilter *filter2,
                     const QccFilter *filter3,
                     int subpixel_accuracy,
                     int blocksize,
                     QccBitBuffer *output_buffer,
                     int num_levels,
                     int target_bit_cnt,
                     const QccWAVWavelet *wavelet,
                     const QccString mv_filename,
                     int read_motion_vectors,
                     int quiet);
int QccVIDRWMHDecodeHeader(QccBitBuffer *input_buffer,
                           int *num_rows,
                           int *num_cols,
                           int *start_frame_num,
                           int *end_frame_num,
                           int *num_levels,
                           int *blocksize,
                           int *target_bit_cnt);
int QccVIDRWMHDecode(QccIMGImageSequence *image_sequence,
                     const QccFilter *filter1,
                     const QccFilter *filter2,
                     const QccFilter *filter3,
                     int subpixel_accuracy,
                     int blocksize,
                     QccBitBuffer *input_buffer,
                     int target_bit_cnt,
                     int num_levels,
                     const QccWAVWavelet *wavelet,
                     const QccString mv_filename,
                     int quiet);


#endif /* LIBQCCPACKVID_H */
