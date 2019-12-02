/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "imgdist.h"

#define USG_STRING "[-vo %:] [-mse %:] [-snr %:] [-psnr %:] [-mae %:] [-cie %:] [-m %: %s:mask] %s:imgfile1 %s:imgfile2"

#define ALL 0
#define MSE 1
#define SNR 2
#define PSNR 3
#define MAE 4
#define CIE 5

int value_only_flag = 0;
int mse_specified = 0;
int snr_specified = 0;
int psnr_specified = 0;
int mae_specified = 0;
int cie_specified = 0;

int DistortionMethod = ALL;

int ColorImages = 0;

QccIMGImage Image1;
QccIMGImage Image2;
int MaxVal1 = 255;
int NumCols1, NumRows1;
int NumCols2, NumRows2;

QccIMGImage Mask;
int MaskSpecified = 0;
int MaskNumRows, MaskNumCols;
int ColorMask;


int main(int argc, char *argv[])
{
  double value = 0;
  double mse = -1.0;
  double snr = -1.0;
  double psnr = -1.0;
  double mae = -1.0;
  double signal_power;
  QccIMGImageComponent *image_component1 = NULL;
  QccIMGImageComponent *image_component2 = NULL;
  QccIMGImageComponent *mask = NULL;
  int component, num_components;
  QccString component_name;
  int num_rows1_Y, num_cols1_Y;
  int num_rows1_U, num_cols1_U;
  int num_rows1_V, num_cols1_V;
  int num_rows2_Y, num_cols2_Y;
  int num_rows2_U, num_cols2_U;
  int num_rows2_V, num_cols2_V;
  int mask_num_rows_Y, mask_num_cols_Y;
  int mask_num_rows_U, mask_num_cols_U;
  int mask_num_rows_V, mask_num_cols_V;
  double cie_snr;

  QccInit(argc, argv);

  QccIMGImageInitialize(&Image1);
  QccIMGImageInitialize(&Image2);
  QccIMGImageInitialize(&Mask);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &value_only_flag,
                         &mse_specified,
                         &snr_specified,
                         &psnr_specified,
                         &mae_specified,
                         &cie_specified,
                         &MaskSpecified,
                         Mask.filename,
                         Image1.filename, Image2.filename))
    QccErrorExit();

  if (mse_specified + snr_specified + psnr_specified +
      mae_specified + cie_specified > 1)
    {
      QccErrorAddMessage("%s: Only one of -mse, -snr, -psnr, -mae, and -cie can be specified",
                         argv[0]);
      QccErrorExit();
    }

  if (mse_specified)
    DistortionMethod = MSE;
  else if (snr_specified)
    DistortionMethod = SNR;
  else if (psnr_specified)
    DistortionMethod = PSNR;
  else if (mae_specified)
    DistortionMethod = MAE;
  else if (cie_specified)
    DistortionMethod = CIE;
  else
    DistortionMethod = ALL;

  if (value_only_flag &&
      (!mse_specified && !snr_specified && !psnr_specified &&
       !mae_specified && !cie_specified))
    {
      QccErrorAddMessage("%s: One of -mse, -snr, -psnr, -mae, and -cie must be used with -vo",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageRead(&Image1))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageRead(&Image2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageGetSizeYUV(&Image1,
                            &num_rows1_Y, &num_cols1_Y,
                            &num_rows1_U, &num_cols1_U,
                            &num_rows1_V, &num_cols1_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageGetSizeYUV(&Image2,
                            &num_rows2_Y, &num_cols2_Y,
                            &num_rows2_U, &num_cols2_U,
                            &num_rows2_V, &num_cols2_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }

  if (MaskSpecified)
    {
      if (QccIMGImageRead(&Mask))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageGetSizeYUV(&Mask,
                                &mask_num_rows_Y, &mask_num_cols_Y,
                                &mask_num_rows_U, &mask_num_cols_U,
                                &mask_num_rows_V, &mask_num_cols_V))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                             argv[0]);
          QccErrorExit();
        }
      ColorMask = QccIMGImageColor(&Mask);
    }
  
  ColorImages = QccIMGImageColor(&Image1);
  if (ColorImages != QccIMGImageColor(&Image2))
    {
      QccErrorAddMessage("%s: Both images must be color or both must be grayscale",
                         argv[0]);
      QccErrorExit();
    }

  if (cie_specified && !ColorImages)
    {
      QccErrorAddMessage("%s: -cie valid only for color images",
                         argv[0]);
      QccErrorExit();
    }

  num_components = ColorImages ? 3 : 1;

  if (!value_only_flag)
    printf("The distortion between files\n   %s\n         and\n   %s\nis:\n",
           Image1.filename, Image2.filename);
  
  if (((DistortionMethod == CIE) || (DistortionMethod == ALL)) &&
      (ColorImages))
    {
      if (MaskSpecified)
        {
          QccErrorAddMessage("%s: CIE distortion unavailable when mask is specified",
                             argv[0]);
          QccErrorExit();
        }

      cie_snr = QccIMGImageColorSNR(&Image1, &Image2);
      
      if (!value_only_flag)
        {
          printf(" SNR based on 1964 CIE color-metric formula:\n");
          printf("%16.6f dB\n", cie_snr);
        }
      else
        printf("%f\n", cie_snr);

      if (cie_specified)
        QccExit;
    }

  for (component = 0; component < num_components; component++)
    {
      switch (component)
        {
        case 0:
          image_component1 = &(Image1.Y);
          image_component2 = &(Image2.Y);
          QccStringCopy(component_name, "Y component");
          NumRows1 = num_rows1_Y;
          NumCols1 = num_cols1_Y;
          NumRows2 = num_rows2_Y;
          NumCols2 = num_cols2_Y;
          if (MaskSpecified)
            {
              MaskNumRows = mask_num_rows_Y;
              MaskNumCols = mask_num_cols_Y;
              mask = &(Mask.Y);
            }
          break;
        case 1:
          image_component1 = &(Image1.U);
          image_component2 = &(Image2.U);
          QccStringCopy(component_name, "U component");
          NumRows1 = num_rows1_U;
          NumCols1 = num_cols1_U;
          NumRows2 = num_rows2_U;
          NumCols2 = num_cols2_U;
          if (MaskSpecified)
            {
              MaskNumRows = (ColorMask ? mask_num_rows_U : mask_num_rows_Y);
              MaskNumCols = (ColorMask ? mask_num_cols_U : mask_num_cols_Y);
              mask = (ColorMask ? &(Mask.U) : &(Mask.Y));
            }
          break;
        case 2:
          image_component1 = &(Image1.V);
          image_component2 = &(Image2.V);
          QccStringCopy(component_name, "V component");
          NumRows1 = num_rows1_V;
          NumCols1 = num_cols1_V;
          NumRows2 = num_rows2_V;
          NumCols2 = num_cols2_V;
          if (MaskSpecified)
            {
              MaskNumRows = (ColorMask ? mask_num_rows_V : mask_num_rows_Y);
              MaskNumCols = (ColorMask ? mask_num_cols_V : mask_num_cols_Y);
              mask = (ColorMask ? &(Mask.V) : &(Mask.Y));
            }
          break;
        }          

      if ((NumCols1 != NumCols2) || (NumRows1 != NumRows2))
        {
          QccErrorAddMessage("%s: Size of %s (%d x %d) does not equal that of %s (%d x %d) for image component %d",
                             argv[0],
                             Image1.filename, NumCols1, NumRows1,
                             Image2.filename, NumCols2, NumRows2,
                             component);
          QccErrorExit();
        }

      if (!MaskSpecified)
        {
          mse = QccIMGImageComponentMSE(image_component1, image_component2);
          mae = QccIMGImageComponentMAE(image_component1, image_component2);
          signal_power = QccIMGImageComponentVariance(image_component1);
        }
      else
        {
          if ((MaskNumRows != NumRows1) || (MaskNumCols != NumCols1))
            {
              QccErrorAddMessage("%s: Mask must be same size as images",
                                 argv[0]);
              QccErrorExit();
            }

          mse = QccIMGImageComponentShapeAdaptiveMSE(image_component1,
                                                     image_component2,
                                                     mask);

          mae = QccIMGImageComponentShapeAdaptiveMAE(image_component1,
                                                     image_component2,
                                                     mask);
          signal_power =
            QccIMGImageComponentShapeAdaptiveVariance(image_component1,
                                                      mask);
        }
      
      if (mse != 0.0)
        {
          psnr = 10.0*log10((double)MaxVal1*MaxVal1/mse);
          snr = 10.0*log10(signal_power/mse);
        }
      
      if (!value_only_flag)
        {
          if (num_components != 1)
            printf(" %s:\n", component_name);
          switch (DistortionMethod)
            {
            case MSE:
              printf("%16.6f (MSE)\n", mse);
              break;
            case SNR:
              printf("%16.6f dB (SNR)\n", snr);
              break;
            case PSNR:
              printf("%16.6f dB (PSNR)\n", psnr);
              break;
            case MAE:
              printf("%16.6f (MAE)\n", mae);
              break;
            case ALL:
            default:
              printf("%16.6f\t(MSE)\n%16.6f dB\t(SNR)\n%16.6f dB\t(PSNR)\n%16.6f\t(MAE)\n",
                     mse, snr, psnr, mae);
              break;
            }
        }
      else
        {
          switch (DistortionMethod)
            {
            case MSE:
              value = mse;
              break;
            case SNR:
              value = snr;
              break;
            case PSNR:
              value = psnr;
              break;
            case MAE:
              value = mae;
              break;
            }
          printf("%f\n", value);
        }
    }

  QccIMGImageFree(&Image1);
  QccIMGImageFree(&Image2);
  QccIMGImageFree(&Mask);

  QccExit;
}
