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


#ifndef LIBQCCPACKHYP_H
#define LIBQCCPACKHYP_H


/*  rawfile.c  */
#define QCCHYP_RAWFORMAT_BSQ 0
#define QCCHYP_RAWFORMAT_BIL 1
#define QCCHYP_RAWFORMAT_BIP 2
#define QCCHYP_RAWENDIAN_BIG 0
#define QCCHYP_RAWENDIAN_LITTLE 1
int QccHYPRawRead2D(QccString filename,
                    QccIMGImageComponent *image_component,
                    int bpp,
                    int signed_data,
                    int endian);
int QccHYPRawWrite2D(QccString filename,
                     const QccIMGImageComponent *image_component,
                     int bpp,
                     int endian);
int QccHYPRawDist2D(QccString filename1,
                    QccString filename2,
                    double *mse,
                    double *mae,
                    double *snr,
                    int num_rows,
                    int num_cols,
                    int bpp,
                    int signed_data,
                    int endian);
int QccHYPRawRead3D(QccString filename,
                    QccIMGImageCube *image_cube,
                    int bpv,
                    int signed_data,
                    int format,
                    int endian);
int QccHYPRawWrite3D(QccString filename,
                     const QccIMGImageCube *image_cube,
                     int bpv,
                     int format,
                     int endian);
int QccHYPRawDist3D(QccString filename1,
                    QccString filename2,
                    double *mse,
                    double *mae,
                    double *snr,
                    int num_frames,
                    int num_rows,
                    int num_cols,
                    int bpv,
                    int signed_data,
                    int format,
                    int endian);


/*  sam.c  */
double QccHYPImageCubeMeanSAM(const QccIMGImageCube *image_cube1,
                              const QccIMGImageCube *image_cube2);


/*  hyperspectral.c  */
int QccHYPImageCubeToColor(const QccIMGImageCube *image_cube,
                           QccIMGImage *image,
                           int red_band,
                           int green_band,
                           int blue_band);


/*  klt.c  */
#define QCCHYPKLT_MAGICNUM "KLT"
typedef struct
{
  QccString filename;
  QccString magic_num;
  int major_version;
  int minor_version;
  int num_bands;
  QccVector mean;
  QccMatrix matrix;
} QccHYPklt;
int QccHYPkltInitialize(QccHYPklt *klt);
int QccHYPkltAlloc(QccHYPklt *klt);
void QccHYPkltFree(QccHYPklt *klt);
int QccHYPkltPrint(const QccHYPklt *klt);
int QccHYPkltRead(QccHYPklt *klt);
int QccHYPkltWrite(const QccHYPklt *klt);
int QccHYPkltTrain(const QccIMGImageCube *image,
                   QccHYPklt *klt);
int QccHYPkltTransform(QccIMGImageCube *image,
                       const QccHYPklt *klt,
                       int num_pcs);
int QccHYPkltInverseTransform(QccIMGImageCube *image,
                              const QccHYPklt *klt);


/*  rklt.c  */
typedef struct
{
  int num_bands;
  QccVectorInt mean;
  QccMatrix matrix;
  QccMatrix P;
  QccMatrix L;
  QccMatrix U;
  QccMatrix S;
  int factored;
} QccHYPrklt;
int QccHYPrkltInitialize(QccHYPrklt *rklt);
int QccHYPrkltAlloc(QccHYPrklt *rklt);
void QccHYPrkltFree(QccHYPrklt *rklt);
int QccHYPrkltTrain(const QccVolumeInt image,
                    int num_bands,
                    int num_rows,
                    int num_cols,
                    QccHYPrklt *rklt);
int QccHYPrkltFactorization(QccHYPrklt *rklt);
int QccHYPrkltTransform(QccVolumeInt image,
                        int num_bands,
                        int num_rows,
                        int num_cols,
                        const QccHYPrklt *rklt);
int QccHYPrkltInverseTransform(QccVolumeInt image,
                               int num_bands,
                               int num_rows,
                               int num_cols,
                               const QccHYPrklt *rklt);


#endif /* LIBQCCPACKHYP_H */
