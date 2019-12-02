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


#include "icbtoraw.h"


#define USG_STRING "[-bpv %d:bits_per_voxel] [-bsq %:] [-bil %:] [-bip %:] [-bend %:] [-lend %:] %s:icbfile %s:rawfile"

QccString RawFile;

QccIMGImageCube ImageCube;

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
                         &BitsPerVoxel,
                         &FormatBSQSpecified,
                         &FormatBILSpecified,
                         &FormatBIPSpecified,
                         &BigEndianSpecified,
                         &LittleEndianSpecified,
                         ImageCube.filename,
                         RawFile))
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

  if (QccIMGImageCubeRead(&ImageCube))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageCubeRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccHYPRawWrite3D(RawFile, &ImageCube,
                       BitsPerVoxel, Format, Endian))
    {
      QccErrorAddMessage("%s: Error calling QccHYPRawWrite3D()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageCubeFree(&ImageCube);

  QccExit;
}
