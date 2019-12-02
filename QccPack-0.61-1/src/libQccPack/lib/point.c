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


void QccPointPrint(const QccPoint *point)
{
  if (point == NULL)
    return;

  printf("(%f, %f)",
         point->x, point->y);
}

void QccPointCopy(QccPoint *point1,
                  const QccPoint *point2)
{
  if (point1 == NULL)
    return;
  if (point2 == NULL)
    return;

  point1->x = point2->x;
  point1->y = point2->y;
}


void QccPointAffineTransform(const QccPoint *point1,
                             QccPoint *point2,
                             QccMatrix affine_transform)
{
  if (point1 == NULL)
    return;
  if (point2 == NULL)
    return;
  if (affine_transform == NULL)
    return;

  point2->x =
    affine_transform[0][0] * point1->x +
    affine_transform[0][1] * point1->y +
    affine_transform[0][2];
  point2->y =
    affine_transform[1][0] * point1->x +
    affine_transform[1][1] * point1->y +
    affine_transform[1][2];
}
