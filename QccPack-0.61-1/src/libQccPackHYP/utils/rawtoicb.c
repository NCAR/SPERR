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


#include "rawtoicb.h"


#define USG_STRING "[-s %:] [-bpv %d:bits_per_voxel] [-bsq %:] [-bil %:] [-bip %:] [-bend %:] [-lend %:] %d:num_rows %d:num_cols %d:num_frames %s:rawfile %s:icbfile"

QccString RawFile;

int NumRows;
int NumCols;
int NumFrames;

QccIMGImageCube ImageCube;

int Signed = 0;

int BitsPerVoxel = 16;

int Format;
int FormatBSQSpecified = 0;
int FormatBILSpecified = 0;
int FormatBIPSpecified = 0;

int Endian;
int BigEndianSpecified = 0;
int LittleEndianSpecified = 0;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccIMGImageCubeInitialize(&ImageCube);

  if (QccParseParameters(argc, argv,
			 USG_STRING,
                         &Signed,
                         &BitsPerVoxel,
                         &FormatBSQSpecified,
                         &FormatBILSpecified,
                         &FormatBIPSpecified,
                         &BigEndianSpecified,
                         &LittleEndianSpecified,
                         &NumRows,
                         &NumCols,
                         &NumFrames,
                         RawFile,
                         ImageCube.filename))
    QccErrorExit();
  
  if ((BitsPerVoxel < 1) ||
      (BitsPerVoxel > 32))
    {
      QccErrorAddMessage("%s: bpv must be between 1 and 32",
                         argv[0]);
      QccErrorExit();
    }
      
  if (FormatBSQSpecified + FormatBILSpecified + FormatBIPSpecified > 1)
    {
      QccErrorAddMessage("%s: Only one of -bsq, -bil, or -bip can be specified",
                         argv[0]);
      QccErrorExit();
    }

  if (FormatBSQSpecified)
    Format = QCCHYP_RAWFORMAT_BSQ;
  else
    if (FormatBILSpecified)
      Format = QCCHYP_RAWFORMAT_BIL;
    else
      if (FormatBIPSpecified)
        Format = QCCHYP_RAWFORMAT_BIP;
      else
        Format = QCCHYP_RAWFORMAT_BSQ;

  if (BigEndianSpecified + LittleEndianSpecified > 1)
    {
      QccErrorAddMessage("%s: Only one of -be or -le can be specified",
                         argv[0]);
      QccErrorExit();
    }

  if (BigEndianSpecified)
    Endian = QCCHYP_RAWENDIAN_BIG;
  else
    if (LittleEndianSpecified)
      Endian = QCCHYP_RAWENDIAN_LITTLE;
    else
      Endian = QCCHYP_RAWENDIAN_BIG;

  ImageCube.num_rows = NumRows;
  ImageCube.num_cols = NumCols;
  ImageCube.num_frames = NumFrames;
  if (QccIMGImageCubeAlloc(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeAlloc()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccHYPRawRead3D(RawFile, &ImageCube,
                      BitsPerVoxel, Signed, Format, Endian))
    {
      QccErrorAddMessage("%s: Error calling QccHYPRawRead3D()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageCubeSetMaxMin(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeSetMaxMin()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageCubeWrite(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&ImageCube);

  QccExit;
}
