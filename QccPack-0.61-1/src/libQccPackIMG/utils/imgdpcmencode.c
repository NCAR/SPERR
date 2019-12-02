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


#include "imgdpcmencode.h"

#define USG_STRING "[-p %d:predictor] %s:imgfile quantizers... %*s:channels..."

int PredictorCode = 2;

QccIMGImage Image;

QccSQScalarQuantizer Quantizers[3];

QccChannel Channels[3];

int NumFiles;
QccString *Filenames;
int NumComponents;

int ImageType;
int NumRows[3];
int NumCols[3];

double OutOfBoundValues[3] =
{
  128.0, 0.0, 0.0
};


int main(int argc, char *argv[])
{
  int current_file;

  QccInit(argc, argv);

  QccIMGImageInitialize(&Image);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &PredictorCode,
                         Image.filename,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();
      
  if (QccIMGImageRead(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  ImageType = Image.image_type;
  
  if (ImageType == QCCIMGTYPE_PBM)
    {
      QccErrorAddMessage("%s: PBM image type not supported",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageGetSizeYUV(&Image,
                            &(NumRows[0]), &(NumCols[0]),
                            &(NumRows[1]), &(NumCols[1]),
                            &(NumRows[2]), &(NumCols[2])))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }

  if (!QccIMGImageColor(&Image))
    {
      if (NumFiles != 2)
        {
          QccErrorAddMessage("%s: Must have only one quantizer and one channel for grayscale input image",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      if (NumFiles != 6)
        {
          QccErrorAddMessage("%s: Must have three quantizers and three channels for color input image",
                             argv[0]);
          QccErrorExit();
        }
    }
  NumComponents = NumFiles/2;

  for (current_file = 0; current_file < NumComponents; current_file++)
    {
      QccSQScalarQuantizerInitialize(&Quantizers[current_file]);
      QccStringCopy(Quantizers[current_file].filename,
             Filenames[current_file]);
      if (QccSQScalarQuantizerRead(&(Quantizers[current_file])))
        {
          QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                             argv[0]);
          QccErrorExit();
        }

      QccChannelInitialize(&Channels[current_file]);
      QccStringCopy(Channels[current_file].filename,
             Filenames[current_file + NumComponents]);
      Channels[current_file].channel_length = 
        NumRows[current_file] * NumCols[current_file];
      Channels[current_file].access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
      if (Quantizers[current_file].type == QCCSQSCALARQUANTIZER_GENERAL)
        Channels[current_file].alphabet_size = 
          Quantizers[current_file].num_levels;
      else
        switch (current_file)
          {
          case 0:
            Channels[current_file].alphabet_size = 
              2*ceil(QccMathMax(fabs(Image.Y.min_val),
                                fabs(Image.Y.max_val)) /
                     Quantizers[current_file].stepsize) + 1;
            break;
          case 1:
            Channels[current_file].alphabet_size = 
              2*ceil(QccMathMax(fabs(Image.U.min_val),
                                fabs(Image.U.max_val)) /
                     Quantizers[current_file].stepsize) + 1;
            break;
          case 2: 
            Channels[current_file].alphabet_size = 
              2*ceil(QccMathMax(fabs(Image.V.min_val),
                                fabs(Image.V.max_val)) /
                     Quantizers[current_file].stepsize) + 1;
            break;
          }
      if (QccChannelAlloc(&Channels[current_file]))
        {
          QccErrorAddMessage("%s: Error calling QccChannelAlloc()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  if (QccIMGImageDPCMEncode(&Image,
                            Quantizers,
                            OutOfBoundValues,
                            PredictorCode,
                            NULL,
                            Channels))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDPCMEncode()",
                         argv[0]);
      QccErrorExit();
    }

  for (current_file = 0; current_file < NumComponents; current_file++)
    if (QccChannelWriteWholefile(&(Channels[current_file])))
      {
        QccErrorAddMessage("%s: Error calling QccChannelWriteWholefile()",
                           argv[0]);
        QccErrorExit();
      }

  QccIMGImageFree(&Image);
  for (current_file = 0; current_file < NumComponents; current_file++)
    {
      QccSQScalarQuantizerFree(&Quantizers[current_file]);
      QccChannelFree(&Channels[current_file]);
    }

  QccExit;
}
