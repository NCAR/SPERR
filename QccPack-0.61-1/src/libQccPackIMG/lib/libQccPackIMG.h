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


#ifndef LIBQCCPACKIMG_H
#define LIBQCCPACKIMG_H


/*  image_component.c  */
#define QCCIMGIMAGECOMPONENT_MAGICNUM "ICP"
typedef QccMatrix QccIMGImageArray;

typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_rows;
  int num_cols;
  double min_val;
  double max_val;
  QccIMGImageArray image;
} QccIMGImageComponent;

int QccIMGImageComponentInitialize(QccIMGImageComponent *image_component);
int QccIMGImageComponentAlloc(QccIMGImageComponent *image_component);
void QccIMGImageComponentFree(QccIMGImageComponent *image_component);
int QccIMGImageComponentPrint(const QccIMGImageComponent *image_component);
int QccIMGImageComponentSetMin(QccIMGImageComponent *image_component);
int QccIMGImageComponentSetMax(QccIMGImageComponent *image_component);
int QccIMGImageComponentSetMaxMin(QccIMGImageComponent *image_component);
int QccIMGImageComponentResize(QccIMGImageComponent *image_component,
                               int num_rows,
                               int num_cols);
int QccIMGImageComponentRead(QccIMGImageComponent *image_component);
int QccIMGImageComponentWrite(const QccIMGImageComponent *image_component);
double QccIMGImageComponentClipPixel(double pixel_value);
int QccIMGImageComponentClip(QccIMGImageComponent *image_component);
int QccIMGImageComponentNormalize(QccIMGImageComponent *image_component);
int QccIMGImageComponentAbsoluteValue(QccIMGImageComponent *image_component);
double QccIMGImageComponentMean(const QccIMGImageComponent *image_component);
double QccIMGImageComponentShapeAdaptiveMean(const QccIMGImageComponent
                                             *image_component,
                                             const QccIMGImageComponent
                                             *alpha_mask);
double QccIMGImageComponentVariance(const
                                    QccIMGImageComponent *image_component);
double QccIMGImageComponentShapeAdaptiveVariance(const QccIMGImageComponent
                                                 *image_component,
                                                 const QccIMGImageComponent
                                                 *alpha_mask);
int QccIMGImageComponentSubtractMean(QccIMGImageComponent 
                                     *image_component,
                                     double *mean,
                                     const QccSQScalarQuantizer *quantizer);
int QccIMGImageComponentAddMean(QccIMGImageComponent *image_component,
                                double mean);
double QccIMGImageComponentMSE(const QccIMGImageComponent *image_component1,
                               const QccIMGImageComponent *image_component2);
double QccIMGImageComponentShapeAdaptiveMSE(const QccIMGImageComponent
                                            *image_component1,
                                            const QccIMGImageComponent
                                            *image_component2,
                                            const QccIMGImageComponent
                                            *alpha_mask);
double QccIMGImageComponentMAE(const QccIMGImageComponent *image_component1,
                               const QccIMGImageComponent *image_component2);
double QccIMGImageComponentShapeAdaptiveMAE(const QccIMGImageComponent
                                            *image_component1,
                                            const QccIMGImageComponent
                                            *image_component2,
                                            const QccIMGImageComponent
                                            *alpha_mask);
int QccIMGImageComponentExtractBlock(const 
                                     QccIMGImageComponent *image_component,
                                     int image_row, int image_col,
                                     QccMatrix block,
                                     int block_num_rows, int block_num_cols);
int QccIMGImageComponentInsertBlock(QccIMGImageComponent *image_component,
                                    int image_row, int image_col,
                                    const QccMatrix block,
                                    int block_num_rows, int block_num_cols);
int QccIMGImageComponentCopy(QccIMGImageComponent *image_component1,
                             const QccIMGImageComponent *image_component2);
int QccIMGImageComponentAdd(const QccIMGImageComponent *image_component1,
                            const QccIMGImageComponent *image_component2,
                            QccIMGImageComponent *image_component3);
int QccIMGImageComponentSubtract(const QccIMGImageComponent *image_component1,
                                 const QccIMGImageComponent *image_component2,
                                 QccIMGImageComponent *image_component3);
int QccIMGImageComponentExtractSubpixel(const QccIMGImageComponent
                                        *image_component,
                                        double x,
                                        double y,
                                        double *subpixel);
int QccIMGImageComponentInterpolateBilinear(const QccIMGImageComponent
                                            *image_component1,
                                            QccIMGImageComponent
                                            *image_component2);
int QccIMGImageComponentInterpolateFilter(const QccIMGImageComponent
                                          *image_component1,
                                          QccIMGImageComponent
                                          *image_component2,
                                          const QccFilter *filter);


/*  image.c  */
#define QCCIMGTYPE_UNKNOWN 0
#define QCCIMGTYPE_PBM     1
#define QCCIMGTYPE_PGM     2
#define QCCIMGTYPE_PPM     3
#define QCCIMGTYPE_ICP     4
typedef struct
{
  int image_type;
  QccString filename;
  QccIMGImageComponent Y;
  QccIMGImageComponent U;
  QccIMGImageComponent V;
} QccIMGImage;
int QccIMGImageInitialize(QccIMGImage *image);
int QccIMGImageGetSize(const QccIMGImage *image,
                       int *num_rows, int *num_cols);
int QccIMGImageGetSizeYUV(const QccIMGImage *image,
                          int *num_rows_Y, int *num_cols_Y,
                          int *num_rows_U, int *num_cols_U,
                          int *num_rows_V, int *num_cols_V);
int QccIMGImageSetSize(QccIMGImage *image,
                       int num_rows, int num_cols);
int QccIMGImageSetSizeYUV(QccIMGImage *image,
                          int num_rows_Y, int num_cols_Y,
                          int num_rows_U, int num_cols_U,
                          int num_rows_V, int num_cols_V);
int QccIMGImageAlloc(QccIMGImage *image);
void QccIMGImageFree(QccIMGImage *image);
int QccIMGImageSetMaxMin(QccIMGImage *image);
int QccIMGImageColor(const QccIMGImage *image);
int QccIMGImageDetermineType(QccIMGImage *image);
int QccIMGImageRead(QccIMGImage *image);
int QccIMGImageWrite(QccIMGImage *image);
int QccIMGImageCopy(QccIMGImage *image1, const QccIMGImage *image2);
double QccIMGImageColorSNR(const QccIMGImage *image1,
                           const QccIMGImage *image2);


/*  image_pnm.c  */
#define QCCIMG_PNM_RAW 0
#define QCCIMG_PNM_ASCII 1
void QccIMGImagePNMGetType(const QccString magic_number,
                           int *returned_image_type,
                           int *returned_raw);
int QccIMGImagePNMRead(QccIMGImage *image);
int QccIMGImagePNMWrite(const QccIMGImage *image);


/*  image_color_conversion.c  */
void QccIMGImageRGBtoYUV(double *Y, double *U, double *V,
                         int R, int G, int B);
void QccIMGImageYUVtoRGB(int *R, int *G, int *B,
                         double Y, double U, double V);
void QccIMGImageRGBtoYCbCr(int *Y, int *Cb, int *Cr,
                           int R, int G, int B);
void QccIMGImageYCbCrtoRGB(int *R, int *G, int *B,
                           int Y, int Cb, int Cr);
void QccIMGImageRGBtoXYZ(double *X, double *Y, double *Z,
                         int R, int G, int B);
void QccIMGImageXYZtoRGB(int *R, int *G, int *B,
                         double X, double Y, double Z);
void QccIMGImageRGBtoUCS(double *U, double *V, double *W,
                         int R, int G, int B);
void QccIMGImageUCStoRGB(int *R, int *G, int *B,
                         double U, double V, double W);
void QccIMGImageRGBtoModifiedUCS(double *U, double *V, double *W,
                                 int R, int G, int B);
void QccIMGImageModifiedUCStoRGB(int *R, int *G, int *B,
                                 double U, double V, double W);
double QccIMGImageModifiedUCSColorMetric(int R1, int G1, int B1,
                                         int R2, int G2, int B2);
void QccIMGImageRGBtoHSV(double *H, double *S, double *V,
                         int R, int G, int B);
void QccIMGImageHSVtoRGB(int *R, int *G, int *B,
                         double H, double S, double V);


/*  image_cube.c  */
#define QCCIMGIMAGECUBE_MAGICNUM "ICB"
typedef QccVolume QccIMGImageVolume;
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_frames;
  int num_rows;
  int num_cols;
  double min_val;
  double max_val;
  QccIMGImageVolume volume;
} QccIMGImageCube;
int QccIMGImageCubeInitialize(QccIMGImageCube *image_cube);
int QccIMGImageCubeAlloc(QccIMGImageCube *image_cube);
void QccIMGImageCubeFree(QccIMGImageCube *image_cube);
int QccIMGImageCubePrint(const QccIMGImageCube *image_cube);
int QccIMGImageCubeSetMin(QccIMGImageCube *image_cube);
int QccIMGImageCubeSetMax(QccIMGImageCube *image_cube);
int QccIMGImageCubeSetMaxMin(QccIMGImageCube *image_cube);
int QccIMGImageCubeResize(QccIMGImageCube *image_cube,
                          int num_frames,
                          int num_rows,
                          int num_cols);
int QccIMGImageCubeRead(QccIMGImageCube *image_cube);
int QccIMGImageCubeWrite(const QccIMGImageCube *image_cube);
int QccIMGImageCubeClip(QccIMGImageCube *image_cube);
int QccIMGImageCubeNormalize(QccIMGImageCube *image_cube,
                             double upper_bound,
                             double lower_bound);
int QccIMGImageCubeAbsoluteValue(QccIMGImageCube *image_cube);
double QccIMGImageCubeMean(const QccIMGImageCube *image_cube);
double QccIMGImageCubeShapeAdaptiveMean(const QccIMGImageCube *image_cube,
                                        const QccIMGImageCube *alpha_mask);
double QccIMGImageCubeVariance(const QccIMGImageCube *image_cube);
double QccIMGImageCubeShapeAdaptiveVariance(const QccIMGImageCube *image_cube,
                                            const QccIMGImageCube *alpha_mask);
int QccIMGImageCubeSubtractMean(QccIMGImageCube *image_cube,
                                double *mean,
                                const QccSQScalarQuantizer *quantizer);
int QccIMGImageCubeAddMean(QccIMGImageCube *image_cube,
                           double mean);
double QccIMGImageCubeMSE(const QccIMGImageCube *image_cube1,
                          const QccIMGImageCube *image_cube2);
double QccIMGImageCubeShapeAdaptiveMSE(const QccIMGImageCube *image_cube1,
                                       const QccIMGImageCube *image_cube2,
                                       const QccIMGImageCube *alpha_mask);
double QccIMGImageCubeMAE(const QccIMGImageCube *image_cube1,
                          const QccIMGImageCube *image_cube2);
double QccIMGImageCubeShapeAdaptiveMAE(const QccIMGImageCube *image_cube1,
                                       const QccIMGImageCube *image_cube2,
                                       const QccIMGImageCube *alpha_mask);
int QccIMGImageCubeExtractBlock(const QccIMGImageCube *image_cube,
                                int image_frame,
                                int image_row,
                                int image_col,
                                QccVolume block,
                                int block_num_frames,
                                int block_num_rows,
                                int block_num_cols);
int QccIMGImageCubeInsertBlock(QccIMGImageCube *image_cube,
                               int image_frame,
                               int image_row,
                               int image_col,
                               const QccVolume block,
                               int block_num_frames,
                               int block_num_rows,
                               int block_num_cols);
int QccIMGImageCubeCopy(QccIMGImageCube *image_cube1,
                        const QccIMGImageCube *image_cube2);
int QccIMGImageCubeAdd(const QccIMGImageCube *image_cube1,
                       const QccIMGImageCube *image_cube2,
                       QccIMGImageCube *image_cube3);
int QccIMGImageCubeSubtract(const QccIMGImageCube *image_cube1,
                            const QccIMGImageCube *image_cube2,
                            QccIMGImageCube *image_cube3);
int QccIMGImageCubeExtractFrame(const QccIMGImageCube *image_cube,
                                int frame,
                                QccIMGImageComponent *image_component);


/*  image_sequence.c  */
typedef struct
{
  QccString filename;
  int start_frame_num;
  int end_frame_num;
  int current_frame_num;
  QccString current_filename;
  QccIMGImage current_frame;
} QccIMGImageSequence;
int QccIMGImageSequenceInitialize(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceAlloc(QccIMGImageSequence *image_sequence);
void QccIMGImageSequenceFree(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceLength(const QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceSetCurrentFilename(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceIncrementFrameNum(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceFindFrameNums(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceReadFrame(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceWriteFrame(QccIMGImageSequence *image_sequence);
int QccIMGImageSequenceStartRead(QccIMGImageSequence *image_sequence);


/*  image_filter.c  */
int QccIMGImageComponentFilterSeparable(const
                                        QccIMGImageComponent *input_image,
                                        QccIMGImageComponent *output_image,
                                        const QccFilter *horizontal_filter,
                                        const QccFilter *vertical_filter,
                                        int boundary_extension);
int QccIMGImageFilterSeparable(const QccIMGImage *input_image,
                               QccIMGImage *output_image,
                               const QccFilter *horizontal_filter,
                               const QccFilter *vertical_filter,
                               int boundary_extension);
int QccIMGImageComponentFilter2D(const QccIMGImageComponent *input_image,
                                 QccIMGImageComponent *output_image,
                                 const QccIMGImageComponent *filter);
int QccIMGImageFilter2D(const QccIMGImage *input_image,
                        QccIMGImage *output_image,
                        const QccIMGImageComponent *filter);


/*  image_dpcm.c  */
#define QCCIMGPREDICTOR_NUMPREDICTORS 6
int QccIMGImageComponentDPCMEncode(const QccIMGImageComponent *image_component,
                                   const QccSQScalarQuantizer *quantizer,
                                   double out_of_bound_value,
                                   int predictor_code,
                                   QccIMGImageComponent *difference_image,
                                   QccChannel *channel);
int QccIMGImageComponentDPCMDecode(QccChannel *channel,
                                   const QccSQScalarQuantizer *quantizer,
                                   double out_of_bound_value,
                                   int predictor_code,
                                   QccIMGImageComponent *image_component);
int QccIMGImageDPCMEncode(const QccIMGImage *image,
                          const QccSQScalarQuantizer *quantizers,
                          const QccVector out_of_bound_values,
                          int predictor_code,
                          QccIMGImage *difference_image,
                          QccChannel *channels);
int QccIMGImageDPCMDecode(QccChannel *channels,
                          const QccSQScalarQuantizer *quantizers,
                          const QccVector out_of_bound_values,
                          int predictor_code,
                          QccIMGImage *image);


/*  image_sq.c  */
int QccIMGImageComponentScalarQuantize(const
                                       QccIMGImageComponent *image_component,
                                       const QccSQScalarQuantizer *quantizer,
                                       QccVector distortion,
                                       QccChannel *channel);
int QccIMGImageComponentInverseScalarQuantize(const QccChannel *channel,
                                              const
                                              QccSQScalarQuantizer *quantizer,
                                              QccIMGImageComponent 
                                              *image_component);
int QccIMGImageScalarQuantize(const QccIMGImage *image,
                              const QccSQScalarQuantizer *quantizers,
                              QccChannel *channels);
int QccIMGImageInverseScalarQuantize(const QccChannel *channels,
                                     const QccSQScalarQuantizer *quantizers,
                                     QccIMGImage *image);


/*  image_dct.c  */
int QccIMGImageComponentDCT(const QccIMGImageComponent *input_component,
                            QccIMGImageComponent *output_component,
                            int block_num_rows,
                            int block_num_cols);
int QccIMGImageComponentInverseDCT(const QccIMGImageComponent *input_component,
                                   QccIMGImageComponent *output_component,
                                   int block_num_rows,
                                   int block_num_cols);
int QccIMGImageDCT(const QccIMGImage *input_image,
                   QccIMGImage *output_image,
                   int block_num_rows,
                   int block_num_cols);
int QccIMGImageInverseDCT(const QccIMGImage *input_image,
                          QccIMGImage *output_image,
                          int block_num_rows,
                          int block_num_cols);


/*  image_lbt.c  */
int QccIMGImageComponentLBT(const QccIMGImageComponent *image1,
                            QccIMGImageComponent *image2,
                            int overlap_sample,
                            int block_size,
                            double smooth_factor);
int QccIMGImageComponentInverseLBT(const QccIMGImageComponent *image1,
                                   QccIMGImageComponent *image2,
                                   int overlap_sample,
                                   int block_size,
                                   double smooth_factor);


/*  icpdat.c  */
int QccIMGImageComponentToDataset(const QccIMGImageComponent *image_component, 
                                  QccDataset *dataset,
                                  int tile_num_rows, 
                                  int tile_num_cols);
int QccIMGDatasetToImageComponent(QccIMGImageComponent *image_component,
                                  const QccDataset *dataset,
                                  int tile_num_rows,
                                  int tile_num_cols);


/*  seqicb.c  */
int QccIMGImageSequenceToImageCube(QccIMGImageSequence *image_sequence,
                                   QccIMGImageCube *image_cube);
int QccIMGImageCubeToImageSequence(const QccIMGImageCube *image_cube,
                                   QccIMGImageSequence *image_sequence);


/*  image_deinterlace  */
int QccIMGImageComponentDeinterlace(const
                                    QccIMGImageComponent *input_component,
                                    QccIMGImageComponent *output_component1,
                                    QccIMGImageComponent *output_component2);
int QccIMGImageDeinterlace(const QccIMGImage *input_image, 
                           QccIMGImage *output_image1,
                           QccIMGImage *output_image2);
int QccIMGImageSequenceDeinterlace(QccIMGImageSequence *input_sequence,
                                   QccIMGImageSequence *output_sequence,
                                   int supersample);


#endif /* LIBQCCPACKIMG_H */
