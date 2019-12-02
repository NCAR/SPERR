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


#include "datdist.h"

#define USG_STRING "[-vo %:] [-mse %:] [-psnr %:] %s:datfile1 %s:datfile2"

#define ALL 0
#define MSE 1
#define PSNR 3

int value_only_flag = 0;
int mse_specified = 0;
int psnr_specified = 0;

int DistortionMethod = ALL;

QccDataset Dataset1;
QccDataset Dataset2;



int main(int argc, char *argv[])
{
  double value = 0;
  double mse = 0;
  double psnr = 0;
  int block_size;
  int num_blocks;
  int current_block;
  double block_mse;

  QccInit(argc, argv);

  QccDatasetInitialize(&Dataset1);
  QccDatasetInitialize(&Dataset2);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &value_only_flag,
                         &mse_specified,
                         &psnr_specified,
                         Dataset1.filename,
                         Dataset2.filename))
    QccErrorExit();

  if (mse_specified + psnr_specified > 1)
    {
      QccErrorAddMessage("%s: Only one of -mse and -psnr can be given\n",
                         argv[0]);
      QccErrorExit();
    }

  if (mse_specified)
    DistortionMethod = MSE;
  else if (psnr_specified)
    DistortionMethod = PSNR;
  else
    DistortionMethod = ALL;

  if (value_only_flag &&
      (!mse_specified &&  !psnr_specified))
    {
      QccErrorAddMessage("%s: One of -mse and -psnr must be used with -vo\n",
                         argv[0]);
      QccErrorExit();
    }

  Dataset1.access_block_size = 1;
  if (QccDatasetStartRead(&Dataset1))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  Dataset2.access_block_size = 1;
  if (QccDatasetStartRead(&Dataset2))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (Dataset1.num_vectors != Dataset2.num_vectors)
    {
      QccErrorAddMessage("%s: %s and %s do not have same number of vectors",
                         argv[0],
                         Dataset1.filename, Dataset2.filename);
      QccErrorExit();
    }
  if (Dataset1.vector_dimension != Dataset2.vector_dimension)
    {
      QccErrorAddMessage("%s: %s and %s do not have same vector dimension",
                         argv[0], Dataset1.filename, Dataset2.filename);
      QccErrorExit();
    }

  block_size = QccDatasetGetBlockSize(&Dataset1);
  num_blocks = Dataset1.num_vectors / block_size;

  for (current_block = 0; current_block < num_blocks; current_block++)
    {
      if (QccDatasetReadBlock(&Dataset1))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccDatasetReadBlock(&Dataset2))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadBlock()",
                             argv[0]);
          QccErrorExit();
        }

      if ((block_mse = 
           QccDatasetMSE(&Dataset1, &Dataset2)) < 0)
        {
          QccErrorAddMessage("%s: Error calling QccDatasetMSE()",
                             argv[0]);
          QccErrorExit();
        }

      mse += block_mse;
    }

  mse /= (double)num_blocks;

  if (QccDatasetEndRead(&Dataset1))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccDatasetEndRead(&Dataset2))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (mse != 0.0)
    psnr = 10.0*log10((Dataset1.max_val - Dataset1.min_val)*
                      (Dataset1.max_val - Dataset1.min_val)/mse);

  if (!value_only_flag)
    {
      printf("The distortion between files\n   %s\n         and\n   %s\nis:     ",
             Dataset1.filename, Dataset2.filename);
      
      switch (DistortionMethod)
        {
        case MSE:
          printf("%f (MSE)\n", mse);
          break;
        case PSNR:
          printf("%f dB (PSNR)\n", psnr);
          break;
        case ALL:
        default:
          printf("\n%16.6f\t(MSE)\n%16.6f dB\t(PSNR)\n",
                 mse, psnr);
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
        case PSNR:
          value = psnr;
          break;
        }
      printf("%f\n", value);
    }

  QccDatasetFree(&Dataset1);
  QccDatasetFree(&Dataset2);

  QccExit;
}
