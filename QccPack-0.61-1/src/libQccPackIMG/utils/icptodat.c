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


#include "icptodat.h"

#define USG_STRING "[-ts %: %d:tilesize] [-tw %: %d:tilewidth] [-th %: %d:tileheight] %s:icpfile %s:datfile"


int tilesize_specified = 0;
int tilewidth_specified = 0;
int tileheight_specified = 0;

int TileSize;
int TileWidth;
int TileHeight;

QccIMGImageComponent InputImage;
QccDataset Datfile;



int main(int argc, char *argv[])
{
  int num_tile_cols, num_tile_rows;
  
  QccInit(argc, argv);
  
  QccIMGImageComponentInitialize(&InputImage);
  QccDatasetInitialize(&Datfile);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &tilesize_specified,
                         &TileSize,
                         &tilewidth_specified,
                         &TileWidth,
                         &tileheight_specified,
                         &TileHeight,
                         InputImage.filename,
                         Datfile.filename))
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
  
  if (QccIMGImageComponentRead(&InputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  num_tile_cols = InputImage.num_cols / TileWidth;
  num_tile_rows = InputImage.num_rows / TileHeight;
  
  if ((InputImage.num_cols != num_tile_cols*TileWidth) ||
      (InputImage.num_rows != num_tile_rows*TileHeight))
    {
      QccErrorAddMessage("%s: Warning: image size %dx%d is not a multiple of the tile size %dx%d -- tiles will be extracted from upper-left of image",
                         argv[0], InputImage.num_cols, InputImage.num_rows, 
                         TileWidth, TileHeight);
      QccErrorPrintMessages();
    }
  
  Datfile.num_vectors = num_tile_cols * num_tile_rows;
  Datfile.vector_dimension = TileWidth*TileHeight;
  Datfile.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  Datfile.min_val = InputImage.min_val;
  Datfile.max_val = InputImage.max_val;
  
  if (QccDatasetStartWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccIMGImageComponentToDataset(&InputImage,
                                    &Datfile, TileWidth, TileHeight))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageComponentToDataset()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccDatasetWriteBlock(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetWriteBlock()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccDatasetEndWrite(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetEndWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccIMGImageComponentFree(&InputImage);
  QccDatasetFree(&Datfile);

  QccExit;
}
