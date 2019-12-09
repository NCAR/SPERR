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


#ifndef LIBQCCPACK_H
#define LIBQCCPACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>
#include <glob.h>
#include <fcntl.h>
#ifdef QCC_USE_PTHREADS
#include <pthread.h>
#endif
#ifdef QCC_USE_MTRACE
#include <mcheck.h>
#endif
#ifdef QCC_USE_GSL
#define HAVE_INLINE
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_fft_complex.h>
#endif

#define TRUE 1
#define FALSE 0

#ifndef MAXINT
#define MAXINT 2147483647
#endif
#ifndef MAXDOUBLE
#define MAXDOUBLE 1.7976931348623157e+308
#endif
#ifndef MAXFLOAT
#define MAXFLOAT 3.40282347e+38F
#endif


/*  parse.c  */
int QccParseParametersVA(int argc, char *argv[], const char *format,
                         va_list ap);
int QccParseParameters(int argc, char *argv[], const char *format, ...);


/*  string.c  */
#define QCCSTRINGLEN 1186
typedef char QccString[QCCSTRINGLEN + 1];
void QccStringMakeNull(QccString qccstring);
int QccStringNull(const QccString qccstring);
void QccConvertToQccString(QccString qccstring, const char *str);
void QccStringCopy(QccString qccstring1, const QccString qccstring2);
void QccStringSprintf(QccString qccstring, const char *format, ...);


/*  init.c  */
#define QCCEXIT_NOERROR 0
#define QCCEXIT_ERROR 1
void QccExtractProgramName(const char *argv0);
int QccGetProgramName(QccString program_name);
void QccInit(int argc, char *argv[]);
#define QccExit exit(QCCEXIT_NOERROR)
#define QccFree(p) (p != NULL) ? free(p):(void)0, (p) = NULL


/*  env.c  */
int QccGetEnv(const char *environment_variable,
              QccString returned_value);
int QccSetEnv(const char *environment_variable,
              const char *value);


/*  version.c  */
void QccVersionInit();
void QccSetUserHeader(const QccString user_header);
void QccGetQccPackVersion(int *major, int *minor, QccString date);
int QccCompareQccPackVersions(int major1, int minor1,
                              int major2, int minor2);
void QccPrintQccPackVersion(FILE *outfile);


/*  time.c  */
void QccTimeInit();
int QccTimeTic();
int QccTimeToc(double *time);


/*  error.c  */
#define QCCERROR_OUTPUTLINELENGTH 80
#define QCCERROR_OUTPUTTABLENGTH 8
typedef struct
{
  QccString current_message;
  QccString *error_messages;
  int num_messages;
  int errcode;
} QccErrorStruct;
void QccErrorInit(void);
void QccErrorAddMessageVA(const char *format, va_list ap);
void QccErrorAddMessage(const char *format, ...);
void QccErrorWarningVA(const char *format, va_list ap);
void QccErrorWarning(const char *format, ...);
void QccErrorPrintMessages(void);
void QccErrorClearMessages(void);
void QccErrorExit(void);


/*  binary_values.c  */
#define QCC_INT_SIZE 4
int QccBinaryCharToInt(const unsigned char *ch, unsigned int *val);
int QccBinaryIntToChar(unsigned int val, unsigned char *ch);
int QccBinaryCharToFloat(const unsigned char *ch, float *val);
int QccBinaryFloatToChar(float val, unsigned char *ch);


/*  file.c  */
#define QCCMAKESTRING(s) QCCMAKESTRING2(s)
#define QCCMAKESTRING2(s) #s
#define QCCFILE_WHOLEFILE -1
void QccFileInit(void);
int QccFileExists(const QccString filename);
int QccFileGetExtension(const QccString filename, QccString extension);
FILE *QccFileOpen(const QccString filename, const QccString mode);
FILE *QccFileDescriptorOpen(int file_descriptor, const QccString mode);
int QccFileClose(FILE *fileptr);
int QccFileSeekable(FILE *fileptr);
int QccFileFlush(FILE *fileptr);
int QccFileRemove(const QccString filename);
int QccFileGetSize(const QccString filename,
                   FILE *fileptr,
                   long *filesize);
int QccFileGetModTime(const QccString filename,
                      FILE *fileptr,
                      long *time);
int QccFileGetRealPath(const QccString filename,
                       QccString path);
int QccFileGetCurrentPosition(FILE *infile, long *current_position);
int QccFileRewind(FILE *infile);
int QccFileReadChar(FILE *infile, char *val);
int QccFileWriteChar(FILE *outfile, char val);
int QccFileReadInt(FILE *infile, int *val);
int QccFileWriteInt(FILE *outfile, int val);
int QccFileReadDouble(FILE *infile, double *val);
int QccFileWriteDouble(FILE *outfile, double val);
int QccFileReadString(FILE *infile, QccString s);
int QccFileWriteString(FILE *outfile, const QccString s);
int QccFileReadLine(FILE *infile, QccString s);
int QccFileSkipWhiteSpace(FILE *infile, int skip_comments_flag);
int QccFileReadMagicNumber(FILE *infile, QccString magic_num,
                           int *major_version_number,
                           int *minor_version_number);
int QccFileWriteMagicNumber(FILE *outfile, const QccString magic_num);
int QccFileWriteMagicNumberVersion(FILE *outfile,
                                   const QccString magic_num,
                                   int major_version_number,
                                   int minor_version_number);
int QccFileGetMagicNumber(const QccString filename, QccString magic_num);
int QccFilePrintFileInfo(const QccString filename,
                         const QccString magic_num,
                         int major_version,
                         int minor_version);


/*  file_path.c  */
int QccFilePathSearch(const QccString pathlist,
                      const QccString filename,
                      QccString found_pathname);
FILE *QccFilePathSearchOpenRead(QccString filename,
                                const char *environment_path_variable,
                                const char *default_path_list);


/*  math.c  */
#define QccMathMax(x, y) (((x) >= (y)) ? (x) : (y))
#define QccMathMin(x, y) (((x) <= (y)) ? (x) : (y))
#define QccMathPercent(x, y) ((y) ? ((double)(x)/(y))*100.0 : (double)0)
#define QccMathModulus(x, y) ((y)?((x)-(y)*((int)floor((double)(x)/(y)))):(x))
#define QccMathLog2(x) ((log(x) / M_LN2))
#define QccMathMedian(a, b, c) (((a) > (b)) ? (((b) > (c)) ? (b) : (((a) > (c)) ? (c) : (a))) : (((a) > (c)) ? (a) : (((b) > (c)) ? (c) : (b))))
#define QCCMATHEPS 3e-16
double QccMathRand(void);
double QccMathRandNormal(void);
typedef double (*QccMathProbabilityDensity)(double value,
                                            double variance,
                                            double mean);
double QccMathGaussianDensity(double x, double variance, double mean);
double QccMathLaplacianDensity(double x, double variance, double mean);


/*  alpha.c  */
#define QCCALPHA_TRANSPARENT 0.0
#define QCCALPHA_OPAQUE 1.0
#define QccAlphaOpaque(alpha) ((alpha) >= QCCALPHA_OPAQUE)
#define QccAlphaTransparent(alpha) ((alpha) <= QCCALPHA_TRANSPARENT)
#define QccAlphaTranslucent(alpha) (((alpha) < QCCALPHA_OPAQUE) && ((alpha) > QCCALPHA_TRANSPARENT))


/*  vector.c  */
#define QCCVECTOR_SORTASCENDING 1
#define QCCVECTOR_SORTDESCENDING 2
#define QCCVECTOR_EVEN 0
#define QCCVECTOR_ODD 1
typedef double *QccVector;
QccVector QccVectorAlloc(int vector_dimension);
void QccVectorFree(QccVector vector);
int QccVectorZero(QccVector vector,
                  int vector_dimension);
QccVector QccVectorResize(QccVector vector,
                          int vector_dimension,
                          int new_vector_dimension);
double QccVectorMean(const QccVector vector,
                     int vector_dimension);
double QccVectorVariance(const QccVector vector,
                         int vector_dimension);
int QccVectorAdd(QccVector vector1,
                 const QccVector vector2,
                 int vector_dimension);
int QccVectorSubtract(QccVector vector1,
                      const QccVector vector2,
                      int vector_dimension);
int QccVectorScalarMult(QccVector vector,
                        double s,
                        int vector_dimension);
int QccVectorCopy(QccVector vector1,
                  const QccVector vector2,
                  int vector_dimension);
double QccVectorNorm(const QccVector vector,
                     int vector_dimension);
void QccVectorNormalize(QccVector vector,
                        int vector_dimension);
double QccVectorDotProduct(const QccVector vector1,
                           const QccVector vector2,
                           int vector_dimension);
double QccVectorAngle(const QccVector vector1,
                      const QccVector vector2,
                      int vector_dimension,
                      int degrees);
double QccVectorSquareDistance(const QccVector vector1,
                               const QccVector vector2, 
                               int vector_dimension);
double QccVectorSumComponents(const QccVector vector,
                              int vector_dimension);
double QccVectorMaxValue(const QccVector vector,
                         int vector_dimension,
                         int *winner);
double QccVectorMinValue(const QccVector vector,
                         int vector_dimension,
                         int *winner);
int QccVectorPrint(const QccVector vector,
                   int vector_dimension);
int QccVectorSortComponents(const QccVector vector,
                            QccVector sorted_vector,
                            int vector_dimension,
                            int sort_direction,
                            int *auxiliary_list);
int QccVectorGetSymbolProbs(const int *symbol_list,
                            int symbol_list_length,
                            QccVector probs, 
                            int num_symbols);
int QccVectorMoveComponentToFront(QccVector vector,
                                  int vector_dimension,
                                  int index);
int QccVectorSubsample(const QccVector input_signal,
                       int input_length,
                       QccVector output_signal,
                       int output_length,
                       int sampling_flag);
int QccVectorUpsample(const QccVector input_signal,
                      int input_length,
                      QccVector output_signal,
                      int output_length,
                      int sampling_flag);
int QccVectorDCT(const QccVector input_signal,
                 QccVector output_signal,
                 int signal_length);
int QccVectorInverseDCT(const QccVector input_signal,
                        QccVector output_signal,
                        int signal_length);


/*  vector_int.c  */
#define QCCVECTORINT_SORTASCENDING 1
#define QCCVECTORINT_SORTDESCENDING 2
#define QCCVECTORINT_EVEN 0
#define QCCVECTORINT_ODD 1
typedef int *QccVectorInt;
QccVectorInt QccVectorIntAlloc(int vector_dimension);
void QccVectorIntFree(QccVectorInt vector);
int QccVectorIntZero(QccVectorInt vector,
                     int vector_dimension);
QccVectorInt QccVectorIntResize(QccVectorInt vector,
                                int vector_dimension,
                                int new_vector_dimension);
double QccVectorIntMean(const QccVectorInt vector,
                        int vector_dimension);
double QccVectorIntVariance(const QccVectorInt vector,
                            int vector_dimension);
int QccVectorIntAdd(QccVectorInt vector1,
                    const QccVectorInt vector2,
                    int vector_dimension);
int QccVectorIntSubtract(QccVectorInt vector1,
                         const QccVectorInt vector2,
                         int vector_dimension);
int QccVectorIntScalarMult(QccVectorInt vector,
                           int s,
                           int vector_dimension);
int QccVectorIntCopy(QccVectorInt vector1,
                     const QccVectorInt vector2,
                     int vector_dimension);
double QccVectorIntNorm(const QccVectorInt vector,
                        int vector_dimension);
int QccVectorIntDotProduct(const QccVectorInt vector1,
                           const QccVectorInt vector2,
                           int vector_dimension);
int QccVectorIntSquareDistance(const QccVectorInt vector1,
                               const QccVectorInt vector2, 
                               int vector_dimension);
int QccVectorIntSumComponents(const QccVectorInt vector,
                              int vector_dimension);
int QccVectorIntMaxValue(const QccVectorInt vector,
                         int vector_dimension,
                         int *winner);
int QccVectorIntMinValue(const QccVectorInt vector,
                         int vector_dimension,
                         int *winner);
int QccVectorIntPrint(const QccVectorInt vector,
                      int vector_dimension);
int QccVectorIntSortComponents(const QccVectorInt vector,
                               QccVectorInt sorted_vector,
                               int vector_dimension,
                               int sort_direction,
                               int *auxiliary_list);
int QccVectorIntMoveComponentToFront(QccVectorInt vector,
                                     int vector_dimension,
                                     int index);
int QccVectorIntSubsample(const QccVectorInt input_signal,
                          int input_length,
                          QccVectorInt output_signal,
                          int output_length,
                          int sampling_flag);
int QccVectorIntUpsample(const QccVectorInt input_signal,
                         int input_length,
                         QccVectorInt output_signal,
                         int output_length,
                         int sampling_flag);


/*  matrix.c  */
typedef QccVector *QccMatrix;
QccMatrix QccMatrixAlloc(int num_rows,
                         int num_cols);
void QccMatrixFree(QccMatrix matrix,
                   int num_rows);
int QccMatrixZero(QccMatrix matrix,
                  int num_rows,
                  int num_cols);
QccMatrix QccMatrixResize(QccMatrix matrix,
                          int num_rows,
                          int num_cols,
                          int new_num_rows,
                          int new_num_cols);
int QccMatrixCopy(QccMatrix matrix1,
                  const QccMatrix matrix2,
                  int num_rows,
                  int num_cols);
double QccMatrixMaxValue(const QccMatrix matrix,
                         int num_rows,
                         int num_cols);
double QccMatrixMinValue(const QccMatrix matrix,
                         int num_rows,
                         int num_cols);
int QccMatrixPrint(const QccMatrix matrix,
                   int num_rows,
                   int num_cols);
int QccMatrixRowExchange(QccMatrix matrix,
                         int num_cols,
                         int row1,
                         int row2);
int QccMatrixColExchange(QccMatrix matrix,   
                         int num_rows,
                         int col1,
                         int col2);
int QccMatrixIdentity(QccMatrix matrix, int num_rows, int num_cols);
int QccMatrixTranspose(const QccMatrix matrix1,
                       QccMatrix matrix2,
                       int num_rows,
                       int num_cols);
int QccMatrixAdd(QccMatrix matrix1,
                 const QccMatrix matrix2,
                 int num_rows,
                 int num_cols);
int QccMatrixSubtract(QccMatrix matrix1,
                      const QccMatrix matrix2,
                      int num_rows,
                      int num_cols);
double QccMatrixMean(const QccMatrix matrix,
                     int num_rows,
                     int num_cols);
double QccMatrixVariance(const QccMatrix matrix,
                         int num_rows,
                         int num_cols);
double QccMatrixMaxSignalPower(const QccMatrix matrix,
                               int num_rows,
                               int num_cols);
int QccMatrixVectorMultiply(const QccMatrix matrix,
                            const QccVector vector1,
                            QccVector vector2,
                            int num_rows,
                            int num_cols);
int QccMatrixMultiply(const QccMatrix matrix1,
                      int num_rows1,
                      int num_cols1,
                      const QccMatrix matrix2,
                      int num_rows2,
                      int num_cols2,
                      QccMatrix matrix3);
int QccMatrixDCT(const QccMatrix input_block,
                 QccMatrix output_block,
                 int num_rows,
                 int num_cols);
int QccMatrixInverseDCT(const QccMatrix input_block,
                        QccMatrix output_block,
                        int num_rows,
                        int num_cols);
int QccMatrixAddNoiseToRegion(QccMatrix matrix,
                              int num_rows,
                              int num_cols,
                              int start_row,
                              int start_col,
                              int region_num_rows,
                              int region_num_cols,
                              double noise_variance);
int QccMatrixInverse(const QccMatrix matrix,
                     QccMatrix matrix_inverse,
                     int num_rows,
                     int num_cols);
int QccMatrixSVD(const QccMatrix A,
                 int num_rows,
                 int num_cols,
                 QccMatrix U,
                 QccVector S,
                 QccMatrix V);
int QccMatrixOrthogonalize(const QccMatrix matrix1,
                           int num_rows,
                           int num_cols,
                           QccMatrix matrix2,
                           int *num_cols2);
int QccMatrixNullspace(const QccMatrix matrix1,
                       int num_rows,
                       int num_cols,
                       QccMatrix matrix2,
                       int *num_cols2);


/*  matrix_int.c  */
typedef QccVectorInt *QccMatrixInt;
QccMatrixInt QccMatrixIntAlloc(int num_rows,
                               int num_cols);
void QccMatrixIntFree(QccMatrixInt matrix,
                      int num_rows);
int QccMatrixIntZero(QccMatrixInt matrix,
                     int num_rows,
                     int num_cols);
QccMatrixInt QccMatrixIntResize(QccMatrixInt matrix,
                                int num_rows,
                                int num_cols,
                                int new_num_rows,
                                int new_num_cols);
int QccMatrixIntCopy(QccMatrixInt matrix1,
                     const QccMatrixInt matrix2,
                     int num_rows,
                     int num_cols);
int QccMatrixIntMaxValue(const QccMatrixInt matrix,
                         int num_rows,
                         int num_cols);
int QccMatrixIntMinValue(const QccMatrixInt matrix,
                         int num_rows,
                         int num_cols);
int QccMatrixIntPrint(const QccMatrixInt matrix,
                      int num_rows,
                      int num_cols);
int QccMatrixIntTranspose(const QccMatrixInt matrix1,
                          QccMatrixInt matrix2,
                          int num_rows,
                          int num_cols);
int QccMatrixIntAdd(QccMatrixInt matrix1,
                    const QccMatrixInt matrix2,
                    int num_rows,
                    int num_cols);
int QccMatrixIntSubtract(QccMatrixInt matrix1,
                         const QccMatrixInt matrix2,
                         int num_rows,
                         int num_cols);
double QccMatrixIntMean(const QccMatrixInt matrix,
                        int num_rows,
                        int num_cols);
double QccMatrixIntVariance(const QccMatrixInt matrix,
                            int num_rows,
                            int num_cols);
int QccMatrixIntVectorMultiply(const QccMatrixInt matrix,
                               const QccVectorInt vector1,
                               QccVectorInt vector2,
                               int num_rows,
                               int num_cols);


/*  volume.c  */
typedef QccMatrix *QccVolume;
QccVolume QccVolumeAlloc(int num_frames,
                         int num_rows,
                         int num_cols);
void QccVolumeFree(QccVolume volume,
                   int num_frames,
                   int num_rows);
int QccVolumeZero(QccVolume volume,
                  int num_frames,
                  int num_rows,
                  int num_cols);
QccVolume QccVolumeResize(QccVolume volume,
                          int num_frames,
                          int num_rows,
                          int num_cols,
                          int new_num_frames,
                          int new_num_rows,
                          int new_num_cols);
int QccVolumeCopy(QccVolume volume1,
                  const QccVolume volume2,
                  int num_frames,
                  int num_rows,
                  int num_cols);
double QccVolumeMaxValue(const QccVolume volume,
                         int num_frames,
                         int num_rows,
                         int num_cols);
double QccVolumeMinValue(const QccVolume volume,
                         int num_frames,
                         int num_rows,
                         int num_cols);
int QccVolumePrint(const QccVolume volume,
                   int num_frames,
                   int num_rows,
                   int num_cols);
int QccVolumeAdd(QccVolume volume1,
                 const QccVolume volume2,
                 int num_frames,
                 int num_rows,
                 int num_cols);
int QccVolumeSubtract(QccVolume volume1,
                      const QccVolume volume2,
                      int num_frames,
                      int num_rows,
                      int num_cols);
double QccVolumeMean(const QccVolume volume,
                     int num_frames,
                     int num_rows,
                     int num_cols);
double QccVolumeVariance(const QccVolume volume,
                         int num_frames,
                         int num_rows,
                         int num_cols);


/*  volume_int.c  */
typedef QccMatrixInt *QccVolumeInt;
QccVolumeInt QccVolumeIntAlloc(int num_frames,
                               int num_rows,
                               int num_cols);
void QccVolumeIntFree(QccVolumeInt volume,
                      int num_frames,
                      int num_rows);
int QccVolumeIntZero(QccVolumeInt volume,
                     int num_frames,
                     int num_rows,
                     int num_cols);
QccVolumeInt QccVolumeIntResize(QccVolumeInt volume,
                                int num_frames,
                                int num_rows,
                                int num_cols,
                                int new_num_frames,
                                int new_num_rows,
                                int new_num_cols);
int QccVolumeIntCopy(QccVolumeInt volume1,
                     const QccVolumeInt volume2,
                     int num_frames,
                     int num_rows,
                     int num_cols);
int QccVolumeIntMaxValue(const QccVolumeInt volume,
                         int num_frames,
                         int num_rows,
                         int num_cols);
int QccVolumeIntMinValue(const QccVolumeInt volume,
                         int num_frames,
                         int num_rows,
                         int num_cols);
int QccVolumeIntPrint(const QccVolumeInt volume,
                      int num_frames,
                      int num_rows,
                      int num_cols);
int QccVolumeIntAdd(QccVolumeInt volume1,
                    const QccVolumeInt volume2,
                    int num_frames,
                    int num_rows,
                    int num_cols);
int QccVolumeIntSubtract(QccVolumeInt volume1,
                         const QccVolumeInt volume2,
                         int num_frames,
                         int num_rows,
                         int num_cols);
double QccVolumeIntMean(const QccVolumeInt volume,
                        int num_frames,
                        int num_rows,
                        int num_cols);
double QccVolumeIntVariance(const QccVolumeInt volume,
                            int num_frames,
                            int num_rows,
                            int num_cols);


/*  fast_dct.c  */
typedef struct
{
  int length;
  QccVector signal_workspace;
#ifdef QCC_USE_GSL
  gsl_complex *forward_weights;
  gsl_complex *inverse_weights;
  gsl_fft_complex_wavetable *wavetable;
  gsl_fft_complex_workspace *workspace;
#endif
} QccFastDCT;
int QccFastDCTInitialize(QccFastDCT *transform);
int QccFastDCTCreate(QccFastDCT *transform,
                     int length);
void QccFastDCTFree(QccFastDCT *transform);
int QccFastDCTForwardTransform1D(const QccVector input_signal,
                                 QccVector output_signal,
                                 int length,
                                 const QccFastDCT *transform);
int QccFastDCTInverseTransform1D(const QccVector input_signal,
                                 QccVector output_signal,
                                 int length,
                                 const QccFastDCT *transform);
int QccFastDCTForwardTransform2D(const QccMatrix input_block,
                                 QccMatrix output_block,
                                 int num_rows,
                                 int num_cols,
                                 const QccFastDCT *transform_horizontal,
                                 const QccFastDCT *transform_vertical);
int QccFastDCTInverseTransform2D(const QccMatrix input_block,
                                 QccMatrix output_block,
                                 int num_rows,
                                 int num_cols,
                                 const QccFastDCT *transform_horizontal,
                                 const QccFastDCT *transform_vertical);


/*  point.c  */
typedef struct
{
  double x;
  double y;
} QccPoint;
void QccPointPrint(const QccPoint *point);
void QccPointCopy(QccPoint *point1,
                  const QccPoint *point2);
void QccPointAffineTransform(const QccPoint *point1,
                             QccPoint *point2,
                             QccMatrix affine_transform);


/*  triangle.c  */
typedef struct
{
  QccPoint vertices[3];
} QccTriangle;
void QccTrianglePrint(const QccTriangle *triangle);
int QccTriangleBoundingBox(const QccTriangle *triangle,
                           QccPoint *box_max,
                           QccPoint *box_min);
int QccTrianglePointInside(const QccTriangle *triangle, const QccPoint *point);
int QccTriangleCreateAffineTransform(const QccTriangle *triangle1,
                                     const QccTriangle *triangle2,
                                     QccMatrix transform);


/*  regular_mesh.c  */
typedef struct
{
  int num_rows;
  int num_cols;
  QccPoint **vertices;
} QccRegularMesh;
void QccRegularMeshInitialize(QccRegularMesh *mesh);
int QccRegularMeshAlloc(QccRegularMesh *mesh);
void QccRegularMeshFree(QccRegularMesh *mesh);
int QccRegularMeshGenerate(QccRegularMesh *mesh,
                           const QccPoint *range_upper,
                           const QccPoint *range_lower);
int QccRegularMeshNumTriangles(const QccRegularMesh *mesh);
int QccRegularMeshToTriangles(const QccRegularMesh *mesh,
                              QccTriangle *triangles);


/*  filter.c  */
#define QCCFILTER_CAUSAL 0
#define QCCFILTER_ANTICAUSAL 1
#define QCCFILTER_SYMMETRICWHOLE 2
#define QCCFILTER_SYMMETRICHALF 3
#define QCCFILTER_SYMMETRIC_EXTENSION 0
#define QCCFILTER_PERIODIC_EXTENSION 1
#define QCCFILTER_SAMESAMPLING 0
#define QCCFILTER_SUBSAMPLEEVEN 1
#define QCCFILTER_SUBSAMPLEODD 2
#define QCCFILTER_UPSAMPLEEVEN 3
#define QCCFILTER_UPSAMPLEODD 4
typedef struct
{
  int causality;
  int length;
  QccVector coefficients;
} QccFilter;
int QccFilterInitialize(QccFilter *filter);
int QccFilterAlloc(QccFilter *filter);
void QccFilterFree(QccFilter *filter);
int QccFilterCopy(QccFilter *filter1,
                  const QccFilter *filter2);
int QccFilterReversal(const QccFilter *filter1,
                      QccFilter *filter2);
int QccFilterAlternateSignFlip(QccFilter *filter);
int QccFilterRead(FILE *infile,
                  QccFilter *filter);
int QccFilterWrite(FILE *outfile,
                   const QccFilter *filter);
int QccFilterPrint(const QccFilter *filter);
int QccFilterCalcSymmetricExtension(int index, int length, int symmetry);
int QccFilterVector(const QccVector input_signal,
                    QccVector output_signal,
                    int length,
                    const QccFilter *filter,
                    int boundary_extension);
int QccFilterMultiRateFilterVector(const QccVector input_signal,
                                   int input_length,
                                   QccVector output_signal,
                                   int output_length,
                                   const QccFilter *filter,
                                   int input_sampling,
                                   int output_sampling,
                                   int boundary_extension);
int QccFilterMatrixSeparable(const QccMatrix input_matrix,
                             QccMatrix output_matrix,
                             int num_rows,
                             int num_cols,
                             const QccFilter *horizontal_filter,
                             const QccFilter *vertical_filter,
                             int boundary_extension);


/*  linked_list.c  */
#define QCCLIST_SORTASCENDING 1
#define QCCLIST_SORTDESCENDING 2
typedef struct QccListNodeStruct
{
  int value_size;
  void *value;
  struct QccListNodeStruct *previous;
  struct QccListNodeStruct *next;
} QccListNode;
typedef struct
{
  QccListNode *start;
  QccListNode *end;
} QccList;
typedef void (*QccListPrintValueFunction)(const void *value);
typedef int (*QccListCompareValuesFunction)(const void *value1,
                                            const void *value2);
void QccListInitialize(QccList *list);
void QccListFreeNode(QccListNode *node);
void QccListFree(QccList *list);
QccListNode *QccListCreateNode(int value_size, const void *value);
QccListNode *QccListCopyNode(const QccListNode *node);
int QccListLength(const QccList *list);
int QccListAppendNode(QccList *list, QccListNode *node);
int QccListInsertNode(QccList *list,
                      QccListNode *insertion_point, 
                      QccListNode *node);
int QccListSortedInsertNode(QccList *list,
                            QccListNode *node,
                            int sort_direction,
                            QccListCompareValuesFunction);
int QccListRemoveNode(QccList *list, QccListNode *node);
int QccListDeleteNode(QccList *list, QccListNode *node);
int QccListMoveNode(QccList *list1, QccList *list2, QccListNode *node);
int QccListSort(QccList *list, int sort_direction,
                int (*compare_values)(const void *value1,
                                      const void *value2));
int QccListConcatenate(QccList *list1, QccList *list2);
int QccListPrint(QccList *list,
                 QccListPrintValueFunction);


/*  bit_buffer.c  */
#define QCCBITBUFFER_BLOCKSIZE 1024
#define QCCBITBUFFER_INPUT 0
#define QCCBITBUFFER_OUTPUT 1
typedef unsigned char QccBitBufferChar;
typedef struct
{
  QccString          filename;
  FILE               *fileptr;
  int                type;
  int                bit_cnt;
  int                byte_cnt;
  int                bits_to_go;
  QccBitBufferChar   buffer;
} QccBitBuffer;
int QccBitBufferInitialize(QccBitBuffer *bit_buffer);
int QccBitBufferStart(QccBitBuffer *bit_buffer);
int QccBitBufferEnd(QccBitBuffer *bit_buffer);
int QccBitBufferFlush(QccBitBuffer *bit_buffer);
int QccBitBufferCopy(QccBitBuffer *output_buffer,
                     QccBitBuffer *input_buffer,
                     int num_bits);
int QccBitBufferPutBit(QccBitBuffer *bit_buffer, int bit_value);
int QccBitBufferGetBit(QccBitBuffer *bit_buffer, int *bit_value);
int QccBitBufferPutBits(QccBitBuffer *bit_buffer, int val, int num_bits);
int QccBitBufferGetBits(QccBitBuffer *bit_buffer, int *val, int num_bits);
int QccBitBufferPutChar(QccBitBuffer *bit_buffer, unsigned char val);
int QccBitBufferGetChar(QccBitBuffer *bit_buffer, unsigned char *val);
int QccBitBufferPutInt(QccBitBuffer *bit_buffer, int val);
int QccBitBufferGetInt(QccBitBuffer *bit_buffer, int *val);
int QccBitBufferPutDouble(QccBitBuffer *bit_buffer, double val);
int QccBitBufferGetDouble(QccBitBuffer *bit_buffer, double *val);


/*  fifo.c  */
typedef struct
{
  QccBitBuffer output_buffer;
  QccBitBuffer input_buffer;
} QccFifo;
void QccFifoInitialize(QccFifo *fifo);
int QccFifoStart(QccFifo *fifo);
int QccFifoEnd(QccFifo *fifo);
int QccFifoFlush(QccFifo *fifo);
int QccFifoRestart(QccFifo *fifo);


/*  dataset.c  */
#define QCCDATASET_MAGICNUM "DAT"
#define QCCDATASET_ACCESSWHOLEFILE QCCFILE_WHOLEFILE
typedef struct
{
  QccString       filename;
  FILE            *fileptr;
  QccString       magic_num;
  int             major_version;
  int             minor_version;
  int             num_vectors;
  int             vector_dimension;
  int             access_block_size;
  int             num_blocks_accessed;
  double          min_val;
  double          max_val;
  QccMatrix       vectors;
} QccDataset;
int QccDatasetInitialize(QccDataset *dataset);
int QccDatasetAlloc(QccDataset *dataset);
void QccDatasetFree(QccDataset *dataset);
int QccDatasetGetBlockSize(const QccDataset *dataset);
int QccDatasetPrint(const QccDataset *dataset);
int QccDatasetCopy(QccDataset *dataset1, const QccDataset *dataset2);
int QccDatasetReadWholefile(QccDataset *dataset);
int QccDatasetReadHeader(QccDataset *dataset);
int QccDatasetStartRead(QccDataset *dataset);
int QccDatasetEndRead(QccDataset *dataset);
int QccDatasetReadBlock(QccDataset *dataset);
int QccDatasetReadSlidingBlock(QccDataset *dataset);
int QccDatasetWriteWholefile(QccDataset *dataset);
int QccDatasetWriteHeader(QccDataset *dataset);
int QccDatasetStartWrite(QccDataset *dataset);
int QccDatasetEndWrite(QccDataset *dataset);
int QccDatasetWriteBlock(QccDataset *dataset);
int QccDatasetSetMaxMinValues(QccDataset *dataset);
double QccDatasetMSE(const QccDataset *dataset1, const QccDataset *dataset2);
int QccDatasetMeanVector(const QccDataset *dataset, QccVector mean);
int QccDatasetCovarianceMatrix(const QccDataset *dataset,
                               QccMatrix covariance);
int QccDatasetCalcVectorPowers(const QccDataset *dataset, 
                               QccVector vector_power);


/*  channel.c  */
#define QCCCHANNEL_MAGICNUM "CHN"
#define QCCCHANNEL_ACCESSWHOLEFILE QCCFILE_WHOLEFILE
#define QCCCHANNEL_UNDEFINEDSIZE -1
#define QCCCHANNEL_NULLSYMBOL -1
typedef struct
{
  QccString    filename;
  FILE         *fileptr;
  QccString    magic_num;
  int          major_version;
  int          minor_version;
  int          channel_length;
  int          access_block_size;
  int          num_blocks_accessed;
  int          alphabet_size;
  int          *channel_symbols;
} QccChannel;
int QccChannelInitialize(QccChannel *channel);
int QccChannelAlloc(QccChannel *channel);
void QccChannelFree(QccChannel *channel);
int QccChannelGetBlockSize(const QccChannel *channel);
int QccChannelPrint(const QccChannel *channel);
int QccChannelReadWholefile(QccChannel *channel);
int QccChannelReadHeader(QccChannel *channel);
int QccChannelStartRead(QccChannel *channel);
int QccChannelEndRead(QccChannel *channel);
int QccChannelReadBlock(QccChannel *channel);
int QccChannelWriteWholefile(QccChannel *channel);
int QccChannelWriteHeader(QccChannel *channel);
int QccChannelStartWrite(QccChannel *channel);
int QccChannelEndWrite(QccChannel *channel);
int QccChannelWriteBlock(QccChannel *channel);
int QccChannelNormalize(QccChannel *channel);
int QccChannelDenormalize(QccChannel *channel);
int QccChannelGetNumNullSymbols(const QccChannel *channel);
int QccChannelRemoveNullSymbols(QccChannel *channel);
double QccChannelEntropy(const QccChannel *channel, int order);
int QccChannelAddSymbolToChannel(QccChannel *channel, int symbol);

/*  Link in other header files  */
#include "libQccPackENT.h"
#include "libQccPackECC.h"
#include "libQccPackSQ.h"
#include "libQccPackVQ.h"
#include "libQccPackAVQ.h"
#include "libQccPackIMG.h"
#include "libQccPackWAV.h"
#include "libQccPackVID.h"
#include "libQccPackHYP.h"

/*  Optional modules  */
#ifdef HAVE_SPIHT
#include "libQccPackSPIHT.h"
#endif
#ifdef HAVE_SPECK
#include "libQccPackSPECK.h"
#endif


#endif /* LIBQCCPACK_H */
