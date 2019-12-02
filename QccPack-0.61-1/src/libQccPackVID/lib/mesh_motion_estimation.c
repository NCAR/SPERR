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


int QccVIDMeshMotionEstimationWarpMesh(const QccRegularMesh *reference_mesh,
                                       QccRegularMesh *current_mesh,
                                       const QccIMGImageComponent
                                       *motion_vectors_horizontal,
                                       const QccIMGImageComponent
                                       *motion_vectors_vertical)
{
  int row, col;

  if (reference_mesh == NULL)
    return(0);
  if (current_mesh == NULL)
    return(0);
  if (motion_vectors_horizontal == NULL)
    return(0);
  if (motion_vectors_vertical == NULL)
    return(0);

  if (reference_mesh->vertices == NULL)
    return(0);
  if (current_mesh->vertices == NULL)
    return(0);
  if (motion_vectors_horizontal->image == NULL)
    return(0);
  if (motion_vectors_vertical->image == NULL)
    return(0);

  if ((motion_vectors_horizontal->num_rows !=
       current_mesh->num_rows) ||
      (motion_vectors_horizontal->num_cols !=
       current_mesh->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationWarpMesh): Motion-vector field is inconsistent with mesh size");
      return(1);
    }
  if ((motion_vectors_vertical->num_rows !=
       current_mesh->num_rows) ||
      (motion_vectors_vertical->num_cols !=
       current_mesh->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationWarpMesh): Motion-vector field is inconsistent with mesh size");
      return(1);
    }

  if ((current_mesh->num_rows != reference_mesh->num_rows) ||
      (current_mesh->num_cols != reference_mesh->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Reference mesh and current mesh are not the same size");
      return(1);
    }
  
  for (row = 0; row < current_mesh->num_rows; row++)
    for (col = 0; col < current_mesh->num_cols; col++)
      {
        current_mesh->vertices[row][col].x =
          reference_mesh->vertices[row][col].x +
          motion_vectors_horizontal->image[row][col];
        current_mesh->vertices[row][col].y =
          reference_mesh->vertices[row][col].y +
          motion_vectors_vertical->image[row][col];
      }
  
  return(0);
}


static int QccVIDMeshMotionEstimationCreateKernel(QccMatrix search_kernel,
                                                  int block_size)
{
  int row, col;
  int offset;
  double d;

  offset = block_size / 2;

  for (row = 0; row < block_size; row++)
    for (col = 0; col < block_size; col++)
      {
        d = (row - offset) * (row - offset) + (col - offset) * (col - offset);
        search_kernel[row][col] = exp(-d / block_size / block_size);
      }      
  
  return(0);
}


static int QccVIDMeshMotionEstimationWindowSearch(QccMatrix current_block,
                                                  QccMatrix reference_block,
                                                  int block_size,
                                                  const QccIMGImageComponent
                                                  *current_frame,
                                                  QccPoint *search_point,
                                                  double window_size,
                                                  double search_step,
                                                  QccMatrix search_kernel,
                                                  int subpixel_accuracy,
                                                  double
                                                  *motion_vector_horizontal,
                                                  double
                                                  *motion_vector_vertical)
{
  double u, v;
  QccPoint current_point;
  double current_mae;
  double min_mae = MAXDOUBLE;

  for (v = -window_size; v <= window_size; v += search_step)
    for (u = -window_size; u <= window_size; u += search_step)
      {
        current_point.x = search_point->x + u;
        current_point.y = search_point->y + v;
        
        if (QccVIDMotionEstimationExtractBlock(current_frame,
                                               current_point.y,
                                               current_point.x,
                                               current_block,
                                               block_size,
                                               subpixel_accuracy))
          {
            QccErrorAddMessage("(QccVIDMeshMotionEstimationWindowSearch): Error calling QccVIDMotionEstimationExtractBlock()");
            return(1);
          }
        
        current_mae = QccVIDMotionEstimationMAE(current_block,
                                                reference_block,
                                                search_kernel,
                                                block_size);
        
        if (current_mae < min_mae)
          {
            min_mae = current_mae;
            *motion_vector_horizontal = u;
            *motion_vector_vertical = v;
          }
      }

  return(0);
}


int QccVIDMeshMotionEstimationSearch(const QccIMGImageComponent
                                     *current_frame,
                                     QccRegularMesh *current_mesh,
                                     const QccIMGImageComponent
                                     *reference_frame,
                                     const QccRegularMesh
                                     *reference_mesh,
                                     QccIMGImageComponent
                                     *motion_vectors_horizontal,
                                     QccIMGImageComponent
                                     *motion_vectors_vertical,
                                     int block_size,
                                     int window_size,
                                     int subpixel_accuracy,
                                     int constrained_boundary,
                                     int exponential_kernel)
{
  int return_value;
  double final_search_step;
  int row, col;
  QccPoint reference_point;
  double current_search_step;
  QccPoint search_point;
  QccMatrix current_block = NULL;
  QccMatrix reference_block = NULL;
  double current_window_size;
  double u = 0;
  double v = 0;
  QccMatrix search_kernel = NULL;
  int start_row, end_row;
  int start_col, end_col;

  if (current_frame == NULL)
    return(0);
  if (current_mesh == NULL)
    return(0);
  if (reference_frame == NULL)
    return(0);
  if (reference_mesh == NULL)
    return(0);
  if (motion_vectors_horizontal == NULL)
    return(0);
  if (motion_vectors_vertical == NULL)
    return(0);

  if (current_frame->image == NULL)
    return(0);
  if (current_mesh->vertices == NULL)
    return(0);
  if (reference_frame->image == NULL)
    return(0);
  if (reference_mesh->vertices == NULL)
    return(0);
  if (motion_vectors_horizontal->image == NULL)
    return(0);
  if (motion_vectors_vertical->image == NULL)
    return(0);

  if ((motion_vectors_horizontal->num_rows != reference_mesh->num_rows) ||
      (motion_vectors_horizontal->num_cols != reference_mesh->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Motion-vector field is inconsistent with mesh size");
      goto Error;
    }
  if ((motion_vectors_vertical->num_rows != reference_mesh->num_rows) ||
      (motion_vectors_vertical->num_cols != reference_mesh->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Motion-vector field is inconsistent with mesh size");
      goto Error;
    }

  if ((current_mesh->num_rows != reference_mesh->num_rows) ||
      (current_mesh->num_cols != reference_mesh->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Reference mesh and current mesh are not the same size");
      goto Error;
    }
  
  if ((reference_frame->num_rows != current_frame->num_rows) ||
      (reference_frame->num_cols != current_frame->num_cols))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Reference-frame size is inconsistent with current-frame size");
      goto Error;
    }

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      final_search_step = 1.0;
      break;
    case QCCVID_ME_HALFPIXEL:
      final_search_step = 0.5;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      final_search_step = 0.25;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      final_search_step = 0.125;
      break;
    default:
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Unrecognized motion-estimation accuracy (%d)",
                         subpixel_accuracy);
      goto Error;
    }

  if ((current_block =
       QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccMatrixAlloc()");
      goto Error;
    }
  if ((reference_block =
       QccMatrixAlloc(block_size, block_size)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccMatrixAlloc()");
      goto Error;
    }

  if (exponential_kernel)
    {
      if ((search_kernel =
           QccMatrixAlloc(block_size, block_size)) == NULL)
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccMatrixAlloc()");
          goto Error;
        }
      if (QccVIDMeshMotionEstimationCreateKernel(search_kernel,
                                                 block_size))
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccVIDMeshMotionEstimationCreateKernel()");
          goto Error;
        }
    }

  for (row = 0; row < current_mesh->num_rows; row++)
    for (col = 0; col < current_mesh->num_cols; col++)
      motion_vectors_horizontal->image[row][col] = 
        motion_vectors_vertical->image[row][col] = 0.0;

  start_row = 0;
  end_row = current_mesh->num_rows - 1;
  start_col = 0;
  end_col = current_mesh->num_cols - 1;

  if (constrained_boundary)
    {
      start_row++;
      end_row--;
      start_col++;
      end_col--;
    }

  for (row = start_row; row <= end_row; row++)
    for (col = start_col; col <= end_col; col++)
      {
        reference_point.x =
          reference_mesh->vertices[row][col].x - (block_size / 2);
        reference_point.y =
          reference_mesh->vertices[row][col].y - (block_size / 2);
        if (QccVIDMotionEstimationExtractBlock(reference_frame,
                                               reference_point.y,
                                               reference_point.x,
                                               reference_block,
                                               block_size,
                                               subpixel_accuracy))
          {
            QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccVIDMotionEstimationExtractBlock()");
            goto Error;
          }

        for (current_search_step = 1.0;
             current_search_step >= final_search_step;
             current_search_step /= 2)
          {
            search_point.x = reference_point.x + 
              motion_vectors_horizontal->image[row][col];
            search_point.y = reference_point.y + 
              motion_vectors_vertical->image[row][col];

            current_window_size =
              (current_search_step == 1.0) ?
              (double)window_size : current_search_step;

            if (QccVIDMeshMotionEstimationWindowSearch(current_block,
                                                       reference_block,
                                                       block_size,
                                                       current_frame,
                                                       &search_point,
                                                       current_window_size,
                                                       current_search_step,
                                                       search_kernel,
                                                       subpixel_accuracy,
                                                       &u,
                                                       &v))
              {
                QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccVIDMeshMotionEstimationWindowSearch()");
                goto Error;
              }
            
            motion_vectors_horizontal->image[row][col] += u;
            motion_vectors_vertical->image[row][col] += v;
          }
      }
      
  if (QccVIDMeshMotionEstimationWarpMesh(reference_mesh,
                                         current_mesh,
                                         motion_vectors_horizontal,
                                         motion_vectors_vertical))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationSearch): Error calling QccVVIDMeshMotionEstimationWarpMesh()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccMatrixFree(current_block, block_size);
  QccMatrixFree(reference_block, block_size);
  QccMatrixFree(search_kernel, block_size);
  return(return_value);
}


static int QccVIDMeshMotionEstimationExtractSubpixel(const QccIMGImageComponent
                                                     *reference_frame,
                                                     const QccPoint
                                                     *reference_point,
                                                     int subpixel_accuracy,
                                                     double *subpixel_value)
{
  QccPoint reference_point2;
  double scale;

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      scale = 1.0;
      break;
    case QCCVID_ME_HALFPIXEL:
      scale = 2.0;
      break;
    case QCCVID_ME_QUARTERPIXEL:
      scale = 4.0;
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      scale = 8.0;
      break;
    default:
      QccErrorAddMessage("(QccVIDMeshMotionEstimationExtractSubpixel): Unrecognized subpixel accuracy (%d)",
                         subpixel_accuracy);
      return(1);
    }

  reference_point2.x = scale * reference_point->x;
  reference_point2.y = scale * reference_point->y;

  if (QccIMGImageComponentExtractSubpixel(reference_frame,
                                          reference_point2.x,
                                          reference_point2.y,
                                          subpixel_value))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationExtractSubpixel): Error calling QccIMGImageComponentExtractSubpixel()");
      return(1);
    }

  return(0);
}


int QccVIDMeshMotionEstimationCreateCompensatedFrame(QccIMGImageComponent
                                                     *motion_compensated_frame,
                                                     const QccRegularMesh
                                                     *current_mesh,
                                                     const QccIMGImageComponent
                                                     *reference_frame,
                                                     const QccRegularMesh
                                                     *reference_mesh,
                                                     int subpixel_accuracy)
{
  int return_value;
  int row, col;
  int num_triangles;
  int triangle;
  QccTriangle *current_triangles = NULL;
  QccTriangle *reference_triangles = NULL;
  QccMatrix *transforms = NULL;
  QccPoint current_point;
  QccPoint reference_point;
  double subpixel_value;

  if (motion_compensated_frame == NULL)
    return(0);
  if (reference_frame == NULL)
    return(0);
  if (current_mesh == NULL)
    return(0);
  if (reference_mesh == NULL)
    return(0);

  if (motion_compensated_frame->image == NULL)
    return(0);
  if (reference_frame->image == NULL)
    return(0);
  if (current_mesh->vertices == NULL)
    return(0);
  if (reference_mesh->vertices == NULL)
    return(0);

  num_triangles = QccRegularMeshNumTriangles(current_mesh);

  switch (subpixel_accuracy)
    {
    case QCCVID_ME_FULLPIXEL:
      if ((reference_frame->num_rows != motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols != motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for full-pixel motion estimation");
          goto Error;
        }
      break;
    case QCCVID_ME_HALFPIXEL:
      if ((reference_frame->num_rows !=
           2 * motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols !=
           2 * motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for half-pixel motion estimation");
          goto Error;
        }
      break;
    case QCCVID_ME_QUARTERPIXEL:
      if ((reference_frame->num_rows !=
           4 * motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols !=
           4 * motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for quarter-pixel motion estimation");
          goto Error;
        }
      break;
    case QCCVID_ME_EIGHTHPIXEL:
      if ((reference_frame->num_rows !=
           8 * motion_compensated_frame->num_rows) ||
          (reference_frame->num_cols !=
           8 * motion_compensated_frame->num_cols))
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Reference-frame size is inconsistent with current-frame size for eighth-pixel motion estimation");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Unrecognized motion-estimation accuracy (%d)",
                         subpixel_accuracy);
      goto Error;
    }

  if ((current_triangles =
       (QccTriangle *)malloc(sizeof(QccTriangle) * num_triangles)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error allocating memory");
      goto Error;
    }
  if ((reference_triangles =
       (QccTriangle *)malloc(sizeof(QccTriangle) * num_triangles)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error allocating memory");
      goto Error;
    }
  if ((transforms =
       (QccMatrix *)malloc(sizeof(QccMatrix) * num_triangles)) == NULL)
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error allocating memory");
      goto Error;
    }

  if (QccRegularMeshToTriangles(current_mesh,
                                current_triangles))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error calling QccRegularMeshToTriangles()");
      goto Error;
    }
  if (QccRegularMeshToTriangles(reference_mesh,
                                reference_triangles))
    {
      QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error calling QccRegularMeshToTriangles()");
      goto Error;
    }

  for (triangle = 0; triangle < num_triangles; triangle++)
    {
      if ((transforms[triangle] =
           QccMatrixAlloc(2, 3)) == NULL)
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error calling QccMatrixAlloc()");
          goto Error;
        }
      if (QccTriangleCreateAffineTransform(&current_triangles[triangle],
                                           &reference_triangles[triangle],
                                           transforms[triangle]))
        {
          QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error calling QccTriangleCreateAffineTransform()");
          goto Error;
        }
    }        

  for (row = 0; row < motion_compensated_frame->num_rows; row++)
    for (col = 0; col < motion_compensated_frame->num_cols; col++)
      {
        current_point.x = (double)col;
        current_point.y = (double)row;

        for (triangle = 0; triangle < num_triangles; triangle++)
          if (QccTrianglePointInside(&current_triangles[triangle],
                                     &current_point))
            break;

        if (triangle >= num_triangles)
          QccPointCopy(&reference_point,
                       &current_point);
        else
          QccPointAffineTransform(&current_point,
                                  &reference_point,
                                  transforms[triangle]);
        
        if (QccVIDMeshMotionEstimationExtractSubpixel(reference_frame,
                                                      &reference_point,
                                                      subpixel_accuracy,
                                                      &subpixel_value))
          {
            QccErrorAddMessage("(QccVIDMeshMotionEstimationCreateCompensatedFrame): Error calling QccVIDMeshMotionEstimationExtractSubpixel()");
            goto Error;
          }
        
        motion_compensated_frame->image[row][col] = subpixel_value;
      }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccFree(current_triangles);
  QccFree(reference_triangles);
  if (transforms != NULL)
    {
      for (triangle = 0; triangle < num_triangles; triangle++)
        QccMatrixFree(transforms[triangle], 2);
      QccFree(transforms);
    }
  return(return_value);
}
