/*
 * 
 * QccPack: Quantization, compression, and coding libraries
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * 
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 * 
 */


#include "libQccPack.h"


void QccRegularMeshInitialize(QccRegularMesh *mesh)
{
  if (mesh == NULL)
    return;

  mesh->num_rows = 0;
  mesh->num_cols = 0;
  mesh->vertices = NULL;
}


int QccRegularMeshAlloc(QccRegularMesh *mesh)
{
  int row;

  if (mesh == NULL)
    return(0);

  if (mesh->num_rows <= 1)
    {
      QccErrorAddMessage("(QccRegularMeshAlloc): Too few rows for regular mesh");
      return(1);
    }
  if (mesh->num_cols <= 1)
    {
      QccErrorAddMessage("(QccRegularMeshAlloc): Too few columns for regular mesh");
      return(1);
    }

  if ((mesh->vertices =
       (QccPoint **)malloc(sizeof(QccPoint *) * mesh->num_rows)) == NULL)
    {
      QccErrorAddMessage("(QccRegularMesh): Error allocating memory");
      return(1);
    }

  for (row = 0; row < mesh->num_rows; row++)
    if ((mesh->vertices[row] =
         (QccPoint *)malloc(sizeof(QccPoint) * mesh->num_cols)) == NULL)
      {
        QccErrorAddMessage("(QccRegularMesh): Error allocating memory");
        return(1);
      }

  return(0);
}


void QccRegularMeshFree(QccRegularMesh *mesh)
{
  int row;

  if (mesh == NULL)
    return;

  for (row = 0; row < mesh->num_rows; row++)
    QccFree(mesh->vertices[row]);
  QccFree(mesh->vertices);
}


int QccRegularMeshGenerate(QccRegularMesh *mesh,
                           const QccPoint *range_upper,
                           const QccPoint *range_lower)
{
  double delta_x;
  double delta_y;
  int row, col;
  double current_x;
  double current_y;

  if (mesh == NULL)
    return(0);
  if (range_upper == NULL)
    return(0);
  if (range_lower == NULL)
    return(0);

  if (mesh->num_rows <= 1)
    {
      QccErrorAddMessage("(QccRegularMeshGenerate): Too few rows for regular mesh");
      return(1);
    }
  if (mesh->num_cols <= 1)
    {
      QccErrorAddMessage("(QccRegularMeshGenerate): Too few columns for regular mesh");
      return(1);
    }

  delta_x = (range_upper->x - range_lower->x) / (mesh->num_cols - 1);
  delta_y = (range_upper->y - range_lower->y) / (mesh->num_rows - 1);

  for (current_y = range_lower->y, row = 0;
       row < mesh->num_rows;
       current_y += delta_y, row++)
    for (current_x = range_lower->x, col = 0;
         col < mesh->num_cols;
         current_x += delta_x, col++)
      {
        mesh->vertices[row][col].x = current_x;
        mesh->vertices[row][col].y = current_y;
      }
  
  return(0);
}


int QccRegularMeshNumTriangles(const QccRegularMesh *mesh)
{
  if (mesh == NULL)
    return(0);

  return(2 * (mesh->num_rows - 1) * (mesh->num_cols - 1));
}


int QccRegularMeshToTriangles(const QccRegularMesh *mesh,
                              QccTriangle *triangles)
{
  int current_triangle;
  int row, col;

  if (mesh == NULL)
    return(0);
  if (triangles == NULL)
    return(0);

  current_triangle = 0;

  for (row = 0; row < (mesh->num_rows - 1); row++)
    for (col = 0; col < (mesh->num_cols - 1); col++, current_triangle++)
      {
        QccPointCopy(&triangles[current_triangle].vertices[0],
                     &mesh->vertices[row][col]);
        QccPointCopy(&triangles[current_triangle].vertices[1],
                     &mesh->vertices[row + 1][col + 1]);
        QccPointCopy(&triangles[current_triangle].vertices[2],
                     &mesh->vertices[row + 1][col]);

        current_triangle++;

        QccPointCopy(&triangles[current_triangle].vertices[0],
                     &mesh->vertices[row][col]);
        QccPointCopy(&triangles[current_triangle].vertices[1],
                     &mesh->vertices[row][col + 1]);
        QccPointCopy(&triangles[current_triangle].vertices[2],
                     &mesh->vertices[row + 1][col + 1]);
      }

  return(0);
}
