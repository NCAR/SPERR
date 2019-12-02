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


#include "dattoimg.h"

#define USG_STRING "[-tw %: %d:tilewidth] [-th %: %d:tileheight] %d:imagewidth %d:imageheight datfiles... %*s:imgfile"


int tilewidth_specified = 0;
int tileheight_specified = 0;

int TileWidth;
int TileHeight;
int ImageWidth;
int ImageHeight;

QccDataset Datfiles[3];

QccIMGImage Image;

int NumFiles;
QccString *Filenames;


int main(int argc, char *argv[])
{
  int num_tile_cols, num_tile_rows;
  int leftover_rows, leftover_cols;
  int row, col;
  int vector_dimension;
  int current_file;
  QccIMGImageComponent *image_component = NULL;

  QccInit(argc, argv);

  QccIMGImageInitialize(&Image);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &tilewidth_specified,
                         &TileWidth,
                         &tileheight_specified,
                         &TileHeight,
                         &ImageWidth, &ImageHeight,
                         &NumFiles,
                         &Filenames))
    QccErrorExit();
      
  QccStringCopy(Image.filename, Filenames[NumFiles - 1]);
  if (QccIMGImageDetermineType(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageDetermineType()",
                         argv[0]);
      QccErrorExit();
    }
  if ((QccIMGImageColor(&Image) && (NumFiles != 4)) ||
      (!QccIMGImageColor(&Image) && (NumFiles != 2)))
    {
      QccErrorAddMessage("%s: Invalid number of files specified",
                         argv[0]);
      QccErrorExit();
    }

  for (current_file = 0; current_file < NumFiles - 1; current_file++)
    {
      QccDatasetInitialize(&Datfiles[current_file]);
      QccStringCopy(Datfiles[current_file].filename,
             Filenames[current_file]);
      
      if (QccDatasetReadWholefile(&Datfiles[current_file]))
        {
          QccErrorAddMessage("%s: Error calling QccDatasetReadWholefile()",
                             argv[0]);
          QccErrorExit();
        }
    }

  vector_dimension = Datfiles[0].vector_dimension;

  if (!tilewidth_specified && !tileheight_specified)
    {
      TileWidth = TileHeight = (int)sqrt((double)vector_dimension);
      if (vector_dimension != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Vector dimension (%d) of %s is not a perfect square\n",
                             argv[0], vector_dimension, Datfiles[0].filename);
          QccErrorExit();
        }
    }
  else if (tilewidth_specified && !tileheight_specified)
    {
      TileHeight = vector_dimension / TileWidth;
      if (vector_dimension != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Vector dimension (%d) of %s is not a multiple of specified tile width %d\n",
                             argv[0], vector_dimension, Datfiles[0].filename, TileWidth);
          QccErrorExit();
        }
    }
  else if (!tilewidth_specified && tileheight_specified)
    {
      TileWidth = vector_dimension / TileHeight;
      if (vector_dimension != TileWidth*TileHeight)
        {
          QccErrorAddMessage("%s: Vector dimension (%d) of %s is not a multiple of specified tile height %d\n",
                             argv[0], vector_dimension, Datfiles[0].filename, TileHeight);
          QccErrorExit();
        }
    }
  else 
    if (TileWidth * TileHeight != Datfiles[0].vector_dimension)
      {
        QccErrorAddMessage("%s: Tilesize %d x %d is incompatible with vector dimension (%d) of %s\n",
                           argv[0], TileWidth, TileHeight,                
                           Datfiles[0].vector_dimension, Datfiles[0].filename);
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
      QccErrorAddMessage("%s: Warning: Image size %dx%d is not a multiple of the tile size %dx%d --- %d and %d columns and rows will be duplicated at right and bottom of the image, respectively",
                         argv[0], ImageWidth, ImageHeight, TileWidth, TileHeight,
                         leftover_cols, leftover_rows);
      QccErrorPrintMessages();
    }
  
  QccIMGImageSetSize(&Image, ImageHeight, ImageWidth);

  if (QccIMGImageAlloc(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageAlloc()",
                         argv[0]);
      QccErrorExit();
    }
      
  for (current_file = 0; current_file < NumFiles - 1; current_file++)
    {
      if (QccDatasetGetBlockSize(&Datfiles[current_file]) < 
          num_tile_cols*num_tile_rows)
        {
          QccErrorAddMessage("%s: Dataset %s does not contain enough data to create image of size %d x %d\n", 
                             argv[0], Datfiles[current_file].filename, ImageWidth, ImageHeight);
          QccErrorExit();
        }
      
      switch (current_file)
        {
        case 0:
          image_component = &(Image.Y);
          break;
        case 1:
          image_component = &(Image.U);
          break;
        case 2:
          image_component = &(Image.V);
          break;
        }

      if (QccIMGDatasetToImageComponent(image_component,
                                        &Datfiles[current_file], 
                                        TileHeight, TileWidth))
        {
          QccErrorAddMessage("%s: Error calling QccIMGDatasetToImageComponent()",
                             argv[0]);
          QccErrorExit();
        }
      
      if (leftover_cols)
        for (col = num_tile_cols*TileWidth; col < ImageWidth; col++)
          for (row = 0; row < ImageHeight; row++)
            image_component->image[row][col] = 
              image_component->image[row][col - 1];
      if (leftover_rows)
        for (row = num_tile_rows*TileHeight; row < ImageHeight; row++)
          for (col = 0; col < ImageWidth; col++)
            image_component->image[row][col] = 
              image_component->image[row - 1][col];
    }

  if (QccIMGImageWrite(&Image))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccIMGImageFree(&Image);

  for (current_file = 0; current_file < NumFiles - 1; current_file++)
    QccDatasetFree(&Datfiles[current_file]);

  QccExit;
}
