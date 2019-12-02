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


#include "imgtodat.h"

#define USG_STRING "[-ts %: %d:tilesize] [-tw %: %d:tilewidth] [-th %: %d:tileheight] %s:imgfile %*s:datfiles..."


int tilesize_specified = 0;
int tilewidth_specified = 0;
int tileheight_specified = 0;

int TileSize;
int TileWidth;
int TileHeight;

QccIMGImage Image;

QccDataset Datfile;
QccString *Datfilenames;
int NumDatfiles;

int NumCols, NumRows;
int ImageType;


int main(int argc, char *argv[])
{
  int num_tile_cols, num_tile_rows;
  int current_file;
  QccIMGImageComponent *image_component = NULL;
  int num_rows_Y, num_cols_Y;
  int num_rows_U, num_cols_U;
  int num_rows_V, num_cols_V;
  
  QccInit(argc, argv);
  
  QccIMGImageInitialize(&Image);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &tilesize_specified,
                         &TileSize,
                         &tilewidth_specified,
                         &TileWidth,
                         &tileheight_specified,
                         &TileHeight,
                         Image.filename,
                         &NumDatfiles,
                         &Datfilenames))
    QccErrorExit();
  
  if ((tilesize_specified && (tilewidth_specified || tileheight_specified)) ||
      (tilewidth_specified && !tileheight_specified) ||
      (!tilewidth_specified && tileheight_specified) ||
      (!tilesize_specified && !tilewidth_specified && !tileheight_specified))
    {
      QccErrorAddMessage("%s: -ts alone or both -tw and -th must be specified\n",
                         argv[0]);
      QccErrorExit();
    }
  
  if (tilesize_specified)
    {
      TileWidth = TileHeight = (int)sqrt((double)TileSize);
      if (TileSize != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Error tilesize %d is not a perfect square\n",
                             argv[0], TileSize);
          QccErrorExit();
        }
    }
  else
    TileSize = TileWidth*TileHeight;
  
  if (TileSize <= 0)
    {
      QccErrorAddMessage("%s: tilesize %d is invalid",
                         argv[0], TileSize);
      QccErrorExit();
    }
  
  if (QccIMGImageRead(&Image))
    {
      QccErrorAddMessage("%s: Error reading image %s",
                         argv[0], Image.filename);
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
                            &num_rows_Y, &num_cols_Y,
                            &num_rows_U, &num_cols_U,
                            &num_rows_V, &num_cols_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (!QccIMGImageColor(&Image))
    {
      if (NumDatfiles != 1)
        {
          QccErrorAddMessage("%s: Must have only one output datfile for grayscale input image",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      if (NumDatfiles != 3)
        {
          QccErrorAddMessage("%s: Must have three output datfiles for color input image",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  for (current_file = 0; current_file < NumDatfiles; current_file++)
    {
      
      switch (current_file)
        {
        case 0:
          image_component = &(Image.Y);
          NumRows = num_rows_Y;
          NumCols = num_cols_Y;
          break;
        case 1:
          image_component = &(Image.U);
          NumRows = num_rows_U;
          NumCols = num_cols_U;
          break;
        case 2:
          image_component = &(Image.V);
          NumRows = num_rows_V;
          NumCols = num_cols_V;
          break;
        }
      
      num_tile_cols = NumCols / TileWidth;
      num_tile_rows = NumRows / TileHeight;
      
      if ((NumCols != num_tile_cols*TileWidth) ||
          (NumRows != num_tile_rows*TileHeight))
        {
          QccErrorAddMessage("%s: Warning: image size %dx%d is not a multiple of the tile size %dx%d -- tiles will be extracted from upper-left of image\n",
                             argv[0], NumCols, NumRows, TileWidth, TileHeight);
          QccErrorPrintMessages();
        }
      
      QccDatasetInitialize(&Datfile);
      Datfile.num_vectors = num_tile_cols * num_tile_rows;
      Datfile.vector_dimension = TileWidth*TileHeight;
      Datfile.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
      
      if (QccDatasetAlloc(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetAlloc()",
                             argv[0]);
          QccErrorExit();
        }
      
      QccStringCopy(Datfile.filename,
             Datfilenames[current_file]);
      
      if (QccIMGImageComponentToDataset(image_component,
                                        &Datfile, TileHeight, TileWidth))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageComponentToDataset()",
                             argv[0]);
          QccErrorExit();
        }
      
      QccDatasetSetMaxMinValues(&Datfile);
      
      if (QccDatasetWriteWholefile(&Datfile))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetEndWrite()",
                             argv[0]);
          QccErrorExit();
        }
      
      QccDatasetFree(&Datfile);
      
    }
  
  QccIMGImageFree(&Image);

  QccExit;
}
