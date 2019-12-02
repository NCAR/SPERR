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


#include "imgdpcmdecode.h"

#define USG_STRING "[-p %d:predictor] %d:imagewidth %d:imageheight quantizers... channels... %*s:imgfile"

int PredictorCode = 2;

QccSQScalarQuantizer Quantizers[3];

QccChannel Channels[3];

QccIMGImage Image;

int NumFiles;
QccString *Filenames;
int NumComponents;

int NumRows;
int NumCols;

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
                         &NumCols, &NumRows,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();
      
  if (NumFiles == 3)
    Image.image_type = QCCIMGTYPE_PGM;
  else
    if (NumFiles == 7)
      Image.image_type = QCCIMGTYPE_PPM;
    else
      {
        QccErrorAddMessage("%s: Invalid number of files specified",
                           argv[0]);
        QccErrorExit();
      }
  NumComponents = (NumFiles - 1)/2;

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
      if (QccChannelReadWholefile(&(Channels[current_file])))
        {
          QccErrorAddMessage("%s: Error calling QccChannelReadWholefile()",
                             argv[0]);
          QccErrorExit();
        }
    }

  QccStringCopy(Image.filename,
         Filenames[NumFiles - 1]);
  QccIMGImageSetSize(&Image, NumRows, NumCols);
  if (QccIMGImageAlloc(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageDPCMDecode(Channels,
                            Quantizers,
                            OutOfBoundValues,
                            PredictorCode,
                            &Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDPCMDecode()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageWrite(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
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
