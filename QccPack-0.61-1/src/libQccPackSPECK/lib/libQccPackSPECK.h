/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 */


/*
 * ----------------------------------------------------------------------------
 * 
 * Public License for the SPECK Algorithm
 * Version 1.1, March 8, 2004
 * 
 * ----------------------------------------------------------------------------
 * 
 * The Set-Partitioning Embedded Block (SPECK) algorithm is protected by US
 * Patent #6,671,413 (issued December 30, 2003) and by patents pending.
 * An implementation of the SPECK algorithm is included herein with the
 * gracious permission of Dr. William A. Pearlman, exclusive holder of patent
 * rights. Dr. Pearlman has granted the following license governing the terms
 * and conditions for use, copying, distribution, and modification of the
 * SPECK algorithm implementation contained herein (hereafter referred to as
 * "the SPECK source code").
 * 
 * 0. Use of the SPECK source code, including any executable-program or
 *    linkable-library form resulting from its compilation, is restricted to
 *    solely academic or non-commercial research activities.
 * 
 * 1. Any other use, including, but not limited to, use in the development
 *    of a commercial product, use in a commercial application, or commercial
 *    distribution, is prohibited by this license. Such acts require a separate
 *    license directly from Dr. Pearlman.
 * 
 * 2. For academic and non-commercial purposes, this license does not restrict
 *    use; copying, distribution, and modification are permitted under the
 *    terms of the GNU General Public License as published by the Free Software
 *    Foundation, with the further restriction that the terms of the present
 *    license shall also apply to all subsequent copies, distributions,
 *    or modifications of the SPECK source code.
 * 
 * NO WARRANTY
 * 
 * 3. Dr. Pearlman dislaims all warranties, expressed or implied, including
 *    without limitation any warranty whatsoever as to the fitness for a
 *    particular use or the merchantability of the SPECK source code.
 * 
 * 4. In no event shall Dr. Pearlman be liable for any loss of profits, loss
 *    of business, loss of use or loss of data, nor for indirect, special,
 *    incidental or consequential damages of any kind related to use of the
 *    SPECK source code.
 * 
 * 
 * END OF TERMS AND CONDITIONS
 * 
 * 
 * Persons desiring to license the SPECK algorithm for commercial purposes or
 * for uses otherwise prohibited by this license may wish to contact
 * Dr. Pearlman regarding the possibility of negotiating such licenses:
 * 
 *   Dr. William A. Pearlman
 *   Dept. of Electrical, Computer, and Systems Engineering
 *   Rensselaer Polytechnic Institute
 *   Troy, NY 12180-3590
 *   U.S.A.
 *   email: pearlman@ecse.rpi.edu
 *   tel.: (518) 276-6082
 *   fax: (518) 276-6261
 *  
 * ----------------------------------------------------------------------------
 */


#ifndef LIBQCCPACKSPECK_H
#define LIBQCCPACKSPECK_H


/*  speck_common.c  */
#define QCCSPECK_LICENSE "\n******************************************************************************\n  The Set-Partitioning Embedded Block (SPECK) algorithm is protected by US\n  Patent #6,671,413 (issued December 30, 2003) and by patents pending.\n  An implementation of the SPECK algorithm is included herein (utility\n  programs speckencode and speckdecode, and speck.c in the QccPack library)\n  with the permission of Dr. William A. Pearlman, exclusive holder of patent\n  rights. Dr. Pearlman has graciously granted a license with certain\n  restrictions governing the terms and conditions for use, copying,\n  distribution, and modification of the SPECK algorithm implementation\n  contained herein. Specifically, only use in academic and non-commercial\n  research is permitted, while all commercial use is prohibited. Refer to\n  the file LICENSE-SPECK for more details.\n******************************************************************************"

#define QCCSPECK3D_LICENSE "\n******************************************************************************\n  Add license statement here.\n******************************************************************************"

#define QCCSPECK_MAJORVERSION 0
#define QCCSPECK_MINORVERSION 61
#define QCCSPECK_COPYRIGHT "Copyright (C) 1997-2016 James E. Fowler"
#define QCCSPECK_DATE "01-apr-2016"

void QccSPECKHeader(void);
void QccSPECK3DHeader(void);


/*  speck.c  */
int QccSPECKEncodeHeader(QccBitBuffer *output_buffer, 
                         int num_levels, 
                         int num_rows, int num_cols,
                         double image_mean,
                         int max_coefficient_bits);

int QccSPECKEncode(const QccIMGImageComponent *image,
                   const QccIMGImageComponent *mask,
                   int num_levels,
                   int target_bit_cnt,
                   const QccWAVWavelet *wavelet,
                   QccBitBuffer *output_buffer);

int QccSPECKDecodeHeader(QccBitBuffer *input_buffer, 
                         int *num_levels, 
                         int *num_rows, int *num_cols,
                         double *image_mean,
                         int *max_coefficient_bits);

int QccSPECKDecode(QccBitBuffer *input_buffer,
                   QccIMGImageComponent *image,
                   const QccIMGImageComponent *mask,
                   int num_levels,
                   const QccWAVWavelet *wavelet,
                   double image_mean,
                   int max_coefficient_bits,
                   int target_bit_cnt);


/*  speck3d.c  */
int QccSPECK3DEncodeHeader(QccBitBuffer *output_buffer,
                           int transform_type,
                           int temporal_num_levels,
                           int spatial_num_levels,
                           int num_frames,
                           int num_rows,
                           int num_cols,
                           double image_mean,
                           int max_coefficient_bits);

int QccSPECK3DEncode(QccIMGImageCube *image_cube,
                     QccIMGImageCube *mask,
                     int transform_type,
                     int temporal_num_levels,
                     int spatial_num_levels,
                     const QccWAVWavelet *wavelet,
                     QccBitBuffer *output_buffer,
                     int target_bit_cnt);

int QccSPECK3DDecodeHeader(QccBitBuffer *input_buffer,
                           int *transform_type,
                           int *temporal_num_levels,
                           int *spatial_num_levels,
                           int *num_frames,
                           int *num_rows,
                           int *num_cols,
                           double *image_mean,
                           int *max_coefficient_bits);

int QccSPECK3DDecode(QccBitBuffer *input_buffer,
                     QccIMGImageCube *image_cube,
                     QccIMGImageCube *mask,
                     int transform_type,
                     int temporal_num_levels,
                     int spatial_num_levels,
                     const QccWAVWavelet *wavelet,
                     double image_mean,
                     int max_coefficient_bits,
                     int target_bit_cnt);


#endif /* LIBQCCPACKSPECK_H */
