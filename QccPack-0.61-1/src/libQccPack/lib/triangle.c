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


void QccTrianglePrint(const QccTriangle *triangle)
{
  if (triangle == NULL)
    return;
  printf("< ");
  QccPointPrint(&triangle->vertices[0]);
  printf(" ");
  QccPointPrint(&triangle->vertices[1]);
  printf(" ");
  QccPointPrint(&triangle->vertices[2]);
  printf(" >\n");
}


int QccTriangleBoundingBox(const QccTriangle *triangle,
                           QccPoint *box_max,
                           QccPoint *box_min)
{
  int vertex;
  double box_max_x = -MAXDOUBLE;
  double box_min_x = MAXDOUBLE;
  double box_max_y = -MAXDOUBLE;
  double box_min_y = MAXDOUBLE;

  if (triangle == NULL)
    return(0);

  for (vertex = 0; vertex < 3; vertex++)
    {
      if (triangle->vertices[vertex].x > box_max_x)
        box_max_x = triangle->vertices[vertex].x;
      if (triangle->vertices[vertex].y > box_max_y)
        box_max_y = triangle->vertices[vertex].y;

      if (triangle->vertices[vertex].x < box_min_x)
        box_min_x = triangle->vertices[vertex].x;
      if (triangle->vertices[vertex].y < box_min_y)
        box_min_y = triangle->vertices[vertex].y;
    }

  if (box_max != NULL)
    {
      box_max->x = box_max_x;
      box_max->y = box_max_y;
    }
  if (box_min != NULL)
    {
      box_min->x = box_min_x;
      box_min->y = box_min_y;
    }
  
  return(0);
}


static int QccTrianglePointInsideBoundingBox(const QccTriangle *triangle,
                                             const QccPoint *point)
{
  QccPoint box_max;
  QccPoint box_min;

  QccTriangleBoundingBox(triangle, &box_max, &box_min);

  if ((point->x >= box_min.x) && (point->x <= box_max.x) &&
      (point->y >= box_min.y) && (point->y <= box_max.y))
    return(1);

  return(0);
}


static int QccTrianglePointInsideCross(const QccPoint *point1,
                                       const QccPoint *point2,
                                       const QccPoint *point3)
{
  QccPoint diff1;
  QccPoint diff2;
  double cross;

  diff1.x = point1->x - point2->x;
  diff1.y = point1->y - point2->y;

  diff2.x = point3->x - point2->x;
  diff2.y = point3->y - point2->y;

  cross = diff1.x * diff2.y - diff1.y * diff2.x;

  if (cross > 0)
    return(1);
  else
    if (cross < 0)
      return(-1);

  return(0);
}


int QccTrianglePointInside(const QccTriangle *triangle, const QccPoint *point)
{
  int cross1, cross2, cross3;

  if (triangle == NULL)
    return(0);
  if (point == NULL)
    return(0);
  
  if (!QccTrianglePointInsideBoundingBox(triangle, point))
    return(0);
  
  cross1 = QccTrianglePointInsideCross(&triangle->vertices[1],
                                       &triangle->vertices[0],
                                       point);
  cross2 = QccTrianglePointInsideCross(&triangle->vertices[2],
                                       &triangle->vertices[1],
                                       point);
  cross3 = QccTrianglePointInsideCross(&triangle->vertices[0],
                                       &triangle->vertices[2],
                                       point);
  
  if (((cross1 >= 0) && (cross2 >= 0) && (cross3 >= 0)) ||
      ((cross1 <= 0) && (cross2 <= 0) && (cross3 <= 0)))
    return(1);
  
  return(0);
}


int QccTriangleCreateAffineTransform(const QccTriangle *triangle1,
                                     const QccTriangle *triangle2,
                                     QccMatrix transform)
{
  double m[9], minv[9];
  double D;
  
  if (triangle1 == NULL)
    return(0);
  if (triangle2 == NULL)
    return(0);
  if (transform == NULL)
    return(0);
  
  m[0] = triangle1->vertices[0].x;
  m[1] = triangle1->vertices[0].y;
  m[2] = 1;
  m[3] = triangle1->vertices[1].x;
  m[4] = triangle1->vertices[1].y;
  m[5] = 1;
  m[6] = triangle1->vertices[2].x;
  m[7] = triangle1->vertices[2].y;
  m[8] = 1;
  
  D =
    m[0]*m[4]*m[8] + m[3]*m[2]*m[7] + m[6]*m[1]*m[5] -
    m[0]*m[5]*m[7] - m[3]*m[1]*m[8] - m[6]*m[2]*m[4];
  
  if (D == 0)
    {
      transform[0][0] = 1;
      transform[0][1] = 0;
      transform[0][2] = 0;
      transform[1][0] = 0;
      transform[1][1] = 1;
      transform[1][2] = 0;
    }
  else
    {
      minv[0] = (m[4]*m[8] - m[5]*m[7]) / D;
      minv[1] = (m[2]*m[7] - m[1]*m[8]) / D;
      minv[2] = (m[1]*m[5] - m[2]*m[4]) / D;
      minv[3] = (m[5]*m[6] - m[3]*m[8]) / D;
      minv[4] = (m[0]*m[8] - m[2]*m[6]) / D;
      minv[5] = (m[2]*m[3] - m[0]*m[5]) / D;
      minv[6] = (m[3]*m[7] - m[6]*m[4]) / D;
      minv[7] = (m[1]*m[6] - m[0]*m[7]) / D;
      minv[8] = (m[0]*m[4] - m[3]*m[1]) / D;
      
      transform[0][0] =
        minv[0] * triangle2->vertices[0].x +
        minv[1] * triangle2->vertices[1].x +
        minv[2] * triangle2->vertices[2].x;
      transform[0][1] =
        minv[3] * triangle2->vertices[0].x +
        minv[4] * triangle2->vertices[1].x +
        minv[5] * triangle2->vertices[2].x;
      transform[0][2] =
        minv[6] * triangle2->vertices[0].x +
        minv[7] * triangle2->vertices[1].x +
        minv[8] * triangle2->vertices[2].x;
      
      transform[1][0] =
        minv[0] * triangle2->vertices[0].y +
        minv[1] * triangle2->vertices[1].y +
        minv[2] * triangle2->vertices[2].y;
      transform[1][1] =
        minv[3] * triangle2->vertices[0].y +
        minv[4] * triangle2->vertices[1].y +
        minv[5] * triangle2->vertices[2].y;
      transform[1][2] =
        minv[6] * triangle2->vertices[0].y +
        minv[7] * triangle2->vertices[1].y +
        minv[8] * triangle2->vertices[2].y;
    }

  return(0);
}

