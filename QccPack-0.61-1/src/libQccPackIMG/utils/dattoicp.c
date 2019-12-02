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


#include "dattoicp.h"

#define USG_STRING "[-tw %: %d:tilewidth] [-th %: %d:tileheight] %d:imagewidth %d:imageheight %s:datfile %s:icpfile"


int tilewidth_specified = 0;
int tileheight_specified = 0;

int TileWidth;
int TileHeight;
int ImageWidth;
int ImageHeight;

QccIMGImageComponent OutputImage;
QccDataset Datfile;


int main(int argc, char *argv[])
{
  int num_tile_cols, num_tile_rows;
  int leftover_cols, leftover_rows;
  int vector_dimension;
  
  QccInit(argc, argv);
  
  QccIMGImageComponentInitialize(&OutputImage);
  QccDatasetInitialize(&Datfile);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &tilewidth_specified,
                         &TileWidth,
                         &tileheight_specified,
                         &TileHeight,
                         &ImageWidth, &ImageHeight,
                         Datfile.filename,
                         OutputImage.filename))
    QccErrorExit();
  
  if (QccDatasetReadWholefile(&Datfile))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  vector_dimension = Datfile.vector_dimension;
  
  if (!tilewidth_specified && !tileheight_specified)
    {
      TileWidth = TileHeight = (int)sqrt((double)vector_dimension);
      if (vector_dimension != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Vector dimension (%d) of %s is not a perfect square\n",
                             argv[0], vector_dimension, Datfile.filename);
          QccErrorExit();
        }
    }
  else if (tilewidth_specified && !tileheight_specified)
    {
      TileHeight = vector_dimension / TileWidth;
      if (vector_dimension != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Vector dimension (%d) of %s is not a multiple of specified tile width %d\n",
                             argv[0], vector_dimension, Datfile.filename, TileWidth);
          QccErrorExit();
        }
    }
  else if (!tilewidth_specified && tileheight_specified)
    {
      TileWidth = vector_dimension / TileHeight;
      if (vector_dimension != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Vector dimension (%d) of %s is not a multiple of specified tile height %d\n",
                             argv[0], vector_dimension, Datfile.filename, TileHeight);
          QccErrorExit();
        }
    }
  else 
    if (TileWidth * TileHeight != Datfile.vector_dimension)
      {
        QccErrorAddMessage("%s: Tilesize %d x %d is incompatible with vector dimension (%d) of %s\n",
                           argv[0], TileWidth, TileHeight,
                           Datfile.vector_dimension, Datfile.filename);
        QccErrorExit();
      }
  
  if (TileWidth*TileHeight <= 0)
    {
      QccErrorAddMessage("%s: Tilesize %d x %d is invalid\n",
                         argv[0], TileWidth, TileHeight);
      QccErrorExit();
    }
  
  
  num_tile_cols = ImageWidth / TileWidth;
  leftover_cols = ImageWidth % TileWidth;
  num_tile_rows = ImageHeight / TileHeight;
  leftover_rows = ImageHeight % TileHeight;
  
  if (!num_tile_cols || !num_tile_rows)
    {
      QccErrorAddMessage("%s: Image size %dx%d is too small for any tiles of size %dx%d",
                         argv[0], ImageWidth, ImageHeight, TileWidth, TileHeight);
      QccErrorExit();
    }
  
  if (leftover_cols || leftover_rows)
    {
      QccErrorAddMessage("%s: Warning: Image size %dx%d is not a multiple of the tile size %dx%d --- %d and %d columns and rows will be duplicated at the right and bottom of the image, respectively",
                         argv[0], ImageWidth, ImageHeight, TileWidth, TileHeight,
                         leftover_cols, leftover_rows);
      QccErrorPrintMessages();
    }
  
  OutputImage.num_rows = ImageHeight;
  OutputImage.num_cols = ImageWidth;
  
  if (QccIMGImageComponentAlloc(&OutputImage))
    {
      QccErrorAddMessage("%s: Error allocating memory\n",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccDatasetGetBlockSize(&Datfile) < num_tile_cols*num_tile_rows)
    {
      QccErrorAddMessage("%s: Dataset %s does not contain enough data to create image of size %d x %d\n", 
                         argv[0], Datfile.filename, ImageWidth, ImageHeight);
      QccErrorExit();
    }
  
  if (QccIMGDatasetToImageComponent(&OutputImage, &Datfile,
                                    TileWidth, TileHeight))
    {
      QccErrorAddMessage("%s: Error calling QccIMGDatasetToImageComponent()",
                         argv[0]);
      QccErrorExit();
    }
  
  OutputImage.max_val = Datfile.max_val;
  OutputImage.min_val = Datfile.min_val;
  
  if (QccIMGImageComponentWrite(&OutputImage))
    {
      QccErrorAddMessage("%s: Error calling QccIMGWritePgm()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccIMGImageComponentFree(&OutputImage);
  QccDatasetFree(&Datfile);

  QccExit;
}
