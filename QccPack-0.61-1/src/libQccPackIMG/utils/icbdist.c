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


#include "icbdist.h"

#define USG_STRING "[-vo %:] [-mse %:] [-snr %:] [-mae %:] [-m %: %s:mask] %s:icbfile1 %s:icbfile2"

#define ALL 0
#define MSE 1
#define SNR 2
#define MAE 3

int value_only_flag = 0;
int mse_specified = 0;
int snr_specified = 0;
int mae_specified = 0;

int DistortionMethod = ALL;

QccIMGImageCube Image1;
QccIMGImageCube Image2;

QccIMGImageCube Mask;
int MaskSpecified = 0;


int main(int argc, char *argv[])
{
  double value = 0;
  double mse = -1.0;
  double snr = -1.0;
  double mae = -1.0;
  double signal_power;

  QccInit(argc, argv);

  QccIMGImageCubeInitialize(&Image1);
  QccIMGImageCubeInitialize(&Image2);
  QccIMGImageCubeInitialize(&Mask);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &value_only_flag,
                         &mse_specified,
                         &snr_specified,
                         &mae_specified,
                         &MaskSpecified,
                         Mask.filename,
                         Image1.filename, Image2.filename))
    QccErrorExit();

  if (mse_specified + snr_specified + mae_specified > 1)
    {
      QccErrorAddMessage("%s: Only one of -mse, -snr, or -mae can be specified",
                         argv[0]);
      QccErrorExit();
    }

  if (mse_specified)
    DistortionMethod = MSE;
  else if (snr_specified)
    DistortionMethod = SNR;
  else if (mae_specified)
    DistortionMethod = MAE;
  else
    DistortionMethod = ALL;

  if (value_only_flag &&
      (!mse_specified && !snr_specified && !mae_specified))
    {
      QccErrorAddMessage("%s: One of -mse, -snr, and -mae must be used with -vo",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageCubeRead(&Image1))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageCubeRead(&Image2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (MaskSpecified)
    {
      if (QccIMGImageCubeRead(&Mask))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                             argv[0]);
          QccErrorExit();
        }
      
      mse = QccIMGImageCubeShapeAdaptiveMSE(&Image1, &Image2, &Mask);
      mae = QccIMGImageCubeShapeAdaptiveMAE(&Image1, &Image2, &Mask);
      
      if ((DistortionMethod == ALL) ||
          (DistortionMethod == SNR))
        {
          signal_power =
            QccIMGImageCubeShapeAdaptiveVariance(&Image1, &Mask);
          if (mse != 0.0)
            snr = 10.0*log10(signal_power/mse);
        }
    }
  else
    {
      mse = QccIMGImageCubeMSE(&Image1, &Image2);
      mae = QccIMGImageCubeMAE(&Image1, &Image2);

      if ((DistortionMethod == ALL) ||
          (DistortionMethod == SNR))
        {
          signal_power = QccIMGImageCubeVariance(&Image1);
          if (mse != 0.0)
            snr = 10.0*log10(signal_power/mse);
        }
    }
      
  if (!value_only_flag)
    printf("The distortion between files\n   %s\n         and\n   %s\nis:\n",
           Image1.filename, Image2.filename);

  
  if (!value_only_flag)
    {
      switch (DistortionMethod)
        {
        case MSE:
          printf("%16.6f (MSE)\n", mse);
          break;
        case SNR:
          printf("%16.6f dB (SNR)\n", snr);
          break;
        case MAE:
          printf("%16.6f (MAE)\n", mae);
          break;
        case ALL:
        default:
          printf("%16.6f\t(MSE)\n%16.6f dB\t(SNR)\n%16.6f\t(MAE)\n",
                 mse, snr, mae);
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
        case MAE:
          value = mae;
          break;
        }
      printf("%f\n", value);
    }

  QccIMGImageCubeFree(&Image1);
  QccIMGImageCubeFree(&Image2);
  QccIMGImageCubeFree(&Mask);

  QccExit;
}
