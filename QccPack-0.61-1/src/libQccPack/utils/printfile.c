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


#include "printfile.h"

#define USG_STRING "[-sq %: %s:codebook_coder] %s:file"


QccString Filename;
QccString MagicNumber;

QccDataset Datfile;
QccSQScalarQuantizer ScalarQuantizer;
QccVQCodebook Codebook;
QccVQMultiStageCodebook MultiStageCodebook;
QccChannel Channel;
QccAVQSideInfo SideInfo;
QccIMGImageComponent ImageComponent;
QccIMGImageCube ImageCube;
QccWAVFilterBank FilterBank;
QccWAVVectorFilterBank VectorFilterBank;
QccWAVLiftingScheme LiftingScheme;
QccWAVZerotree Zerotree;
QccWAVSubbandPyramid SubbandPyramid;
QccENTHuffmanTable HuffmanTable;
QccHYPklt KLT;

int codebook_coder_specified;

int main(int argc, char *argv[])
{

  QccInit(argc, argv);
  
  QccDatasetInitialize(&Datfile);
  QccSQScalarQuantizerInitialize(&ScalarQuantizer);
  QccVQCodebookInitialize(&Codebook);
  QccVQMultiStageCodebookInitialize(&MultiStageCodebook);
  QccChannelInitialize(&Channel);
  QccAVQSideInfoInitialize(&SideInfo);
  QccIMGImageComponentInitialize(&ImageComponent);
  QccWAVFilterBankInitialize(&FilterBank);
  QccWAVVectorFilterBankInitialize(&VectorFilterBank);
  QccWAVLiftingSchemeInitialize(&LiftingScheme);
  QccWAVZerotreeInitialize(&Zerotree);
  QccWAVSubbandPyramidInitialize(&SubbandPyramid);
  QccENTHuffmanTableInitialize(&HuffmanTable);
  QccHYPkltInitialize(&KLT);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &codebook_coder_specified,
                         SideInfo.codebook_coder.filename,
                         Filename))
    QccErrorExit();
  
  if (QccFileGetMagicNumber(Filename, MagicNumber))
    {
      QccErrorAddMessage("%s: Error calling QccFileGetMagicNumber()",
                         argv[0]);
      QccErrorExit();
    }

  if (!strcmp(MagicNumber, QCCDATASET_MAGICNUM))
    {
      QccStringCopy(Datfile.filename, Filename);
      if (QccDatasetReadWholefile(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadWholefile()",
                             argv[0]);
          QccErrorExit();
        }
      QccDatasetPrint(&Datfile);
    }
  else if (!strcmp(MagicNumber, QCCSQSCALARQUANTIZER_MAGICNUM))
    {
      QccStringCopy(ScalarQuantizer.filename, Filename);
      if (QccSQScalarQuantizerRead(&ScalarQuantizer))
        {
          QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccSQScalarQuantizerPrint(&ScalarQuantizer);
    }
  else if (!strcmp(MagicNumber, QCCVQCODEBOOK_MAGICNUM))
    {
      QccStringCopy(Codebook.filename, Filename);
      if (QccVQCodebookRead(&Codebook))
        {
          QccErrorAddMessage("%s: Error calling QccVQCodebookRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccVQCodebookPrint(&Codebook);
    }
  else if (!strcmp(MagicNumber, QCCVQMULTISTAGECODEBOOK_MAGICNUM))
    {
      QccStringCopy(MultiStageCodebook.filename, Filename);
      if (QccVQMultiStageCodebookRead(&MultiStageCodebook))
        {
          QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccVQMultiStageCodebookPrint(&MultiStageCodebook);
    }
  else if (!strcmp(MagicNumber, QCCCHANNEL_MAGICNUM))
    {
      QccStringCopy(Channel.filename, Filename);
      if (QccChannelReadWholefile(&Channel))
        {
          QccErrorAddMessage("%s: Error calling QccChannelReadWholefile()",
                             argv[0]);
          QccErrorExit();
        }
      QccChannelPrint(&Channel);
    }
  else if (!strcmp(MagicNumber, QCCAVQSIDEINFO_MAGICNUM))
    {
      QccStringCopy(SideInfo.filename, Filename);
      if (!codebook_coder_specified)
        {
          QccErrorAddMessage("%s: Must specify a codebook coder for SID format files",
                             argv[0]);
          QccErrorExit();
        }
      if (QccSQScalarQuantizerRead(&(SideInfo.codebook_coder)))
        {
          QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccAVQSideInfoStartRead(&SideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQSideInfoStartRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccAVQSideInfoPrint(&SideInfo);
      if (QccAVQSideInfoEndRead(&SideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQSideInfoEndRead()",
                             argv[0]);
          QccErrorExit();
        }
    }
  else if (!strcmp(MagicNumber, QCCIMGIMAGECOMPONENT_MAGICNUM))
    {
      QccStringCopy(ImageComponent.filename, Filename);
      if (QccIMGImageComponentRead(&ImageComponent))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccIMGImageComponentPrint(&ImageComponent);
    }
  else if (!strcmp(MagicNumber, QCCIMGIMAGECUBE_MAGICNUM))
    {
      QccStringCopy(ImageCube.filename, Filename);
      if (QccIMGImageCubeRead(&ImageCube))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccIMGImageCubePrint(&ImageCube);
    }
  else if (!strcmp(MagicNumber, QCCWAVFILTERBANK_MAGICNUM))
    {
      QccStringCopy(FilterBank.filename, Filename);
      if (QccWAVFilterBankRead(&FilterBank))
        {
          QccErrorAddMessage("%s: Error calling QccWAVFilterBankRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccWAVFilterBankPrint(&FilterBank);
    }
  else if (!strcmp(MagicNumber, QCCWAVVECTORFILTERBANK_MAGICNUM))
    {
      QccStringCopy(VectorFilterBank.filename, Filename);
      if (QccWAVVectorFilterBankRead(&VectorFilterBank))
        {
          QccErrorAddMessage("%s: Error calling QccWAVVectorFilterBankRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccWAVVectorFilterBankPrint(&VectorFilterBank);
    }
  else if (!strcmp(MagicNumber, QCCWAVLIFTINGSCHEME_MAGICNUM))
    {
      QccStringCopy(LiftingScheme.filename, Filename);
      if (QccWAVLiftingSchemeRead(&LiftingScheme))
        {
          QccErrorAddMessage("%s: Error calling QccWAVLiftingSchemeRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccWAVLiftingSchemePrint(&LiftingScheme);
    }
  else if (!strcmp(MagicNumber, QCCWAVZEROTREE_MAGICNUM))
    {
      QccStringCopy(Zerotree.filename, Filename);
      if (QccWAVZerotreeRead(&Zerotree))
        {
          QccErrorAddMessage("%s: Error calling QccWAVZerotreeRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccWAVZerotreePrint(&Zerotree);
    }
  else if (!strcmp(MagicNumber, QCCWAVSUBBANDPYRAMID_MAGICNUM))
    {
      QccStringCopy(SubbandPyramid.filename, Filename);
      if (QccWAVSubbandPyramidRead(&SubbandPyramid))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccWAVSubbandPyramidPrint(&SubbandPyramid);
    }
  else if (!strcmp(MagicNumber, QCCWAVSUBBANDPYRAMIDINT_MAGICNUM))
    {
      QccStringCopy(SubbandPyramid.filename, Filename);
      if (QccWAVSubbandPyramidRead(&SubbandPyramid))
        {
          QccErrorAddMessage("%s: Error calling QccWAVSubbandPyramidRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccWAVSubbandPyramidPrint(&SubbandPyramid);
    }
  else if (!strcmp(MagicNumber, QCCENTHUFFMANTABLE_MAGICNUM))
    {
      QccStringCopy(HuffmanTable.filename, Filename);
      HuffmanTable.table_type = QCCENTHUFFMAN_DECODETABLE;
      if (QccENTHuffmanTableRead(&HuffmanTable))
        {
          QccErrorAddMessage("%s: Error calling QccENTHuffmanTableRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccENTHuffmanTablePrint(&HuffmanTable);
    }
  else if (!strcmp(MagicNumber, QCCHYPKLT_MAGICNUM))
    {
      QccStringCopy(KLT.filename, Filename);
      if (QccHYPkltRead(&KLT))
        {
          QccErrorAddMessage("%s: Error calling QccHYPkltRead()",
                             argv[0]);
          QccErrorExit();
        }
      QccHYPkltPrint(&KLT);
    }
  else
    {
      QccErrorAddMessage("%s: Unrecognized magic number, %s",
                         argv[0], MagicNumber);
      QccErrorExit();
    }
  
  QccDatasetFree(&Datfile);
  QccSQScalarQuantizerFree(&ScalarQuantizer);
  QccVQCodebookFree(&Codebook);
  QccVQMultiStageCodebookFreeCodebooks(&MultiStageCodebook);
  QccChannelFree(&Channel);
  QccIMGImageComponentFree(&ImageComponent);
  QccWAVFilterBankFree(&FilterBank);
  QccWAVVectorFilterBankFree(&VectorFilterBank);
  QccWAVZerotreeFree(&Zerotree);
  QccWAVSubbandPyramidFree(&SubbandPyramid);
  QccENTHuffmanTableFree(&HuffmanTable);
  QccHYPkltFree(&KLT);

  QccExit;
}
