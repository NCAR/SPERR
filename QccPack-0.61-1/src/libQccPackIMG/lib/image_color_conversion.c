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


void QccIMGImageRGBtoYUV(double *Y, double *U, double *V,
                         int R, int G, int B)
{
  if (Y != NULL)
    *Y =  0.299 * R + 0.587 * G + 0.114 * B;
  if (U != NULL)
    *U = -0.147 * R - 0.289 * G + 0.436 * B;
  if (V != NULL)
    *V =  0.615 * R - 0.515 * G - 0.100 * B;
}


void QccIMGImageYUVtoRGB(int *R, int *G, int *B,
                         double Y, double U, double V)
{
  if (R != NULL)
    *R = QccIMGImageComponentClipPixel((int)rint(Y + 1.140 * V));
  if (G != NULL)
    *G = QccIMGImageComponentClipPixel((int)rint(Y - 0.395 * U - 0.581 * V));
  if (B != NULL)
    *B = QccIMGImageComponentClipPixel((int)rint(Y + 2.032 * U));
}


void QccIMGImageRGBtoYCbCr(int *Y, int *Cb, int *Cr,
                           int R, int G, int B)
{
  if (Y != NULL)
    *Y =  (int)rint(0.257 * R + 0.504 * G + 0.098 * B) + 16;
  if (Cb != NULL)
    *Cb = (int)rint(-0.148 * R - 0.291 * G + 0.439 * B) + 128;
  if (Cr != NULL)
    *Cr =  (int)rint(0.439 * R - 0.368 * G - 0.071 * B) + 128;
}


void QccIMGImageYCbCrtoRGB(int *R, int *G, int *B,
                           int Y, int Cb, int Cr)
{
  Y -= 16;
  Cb -= 128;
  Cr -= 128;

  if (R != NULL)
    *R =
      QccIMGImageComponentClipPixel((int)rint(1.164*Y - 0.002*Cb + 1.596*Cr));
  if (G != NULL)
    *G =
      QccIMGImageComponentClipPixel((int)rint(1.164*Y - 0.391*Cb - 0.814*Cr));
  if (B != NULL)
    *B =
      QccIMGImageComponentClipPixel((int)rint(1.164*Y + 2.018*Cb - 0.001*Cr));
}


void QccIMGImageRGBtoXYZ(double *X, double *Y, double *Z,
                         int R, int G, int B)
{
  if (X != NULL)
    *X = (0.490 * R + 0.310 * G + 0.200 * B) / 255;
  if (Y != NULL)
    *Y = (0.177 * R + 0.813 * G + 0.011 * B) / 255;
  if (Z != NULL)
    *Z = (0.010 * G + 0.990 * B) / 255;
}


void QccIMGImageXYZtoRGB(int *R, int *G, int *B,
                         double X, double Y, double Z)
{
  double red, green, blue;

  red   =  2.3644 * X - 0.8958 * Y - 0.4677 * Z;
  green = -0.5148 * X + 1.4252 * Y + 0.0882 * Z;
  blue  =  0.0052 * X - 0.0144 * Y + 1.0092 * Z;

  if (R != NULL)
    *R = QccIMGImageComponentClipPixel((int)rint(red * 255));
  if (G != NULL)
    *G = QccIMGImageComponentClipPixel((int)rint(green * 255));
  if (B != NULL)
    *B = QccIMGImageComponentClipPixel((int)rint(blue * 255));
}


void QccIMGImageRGBtoUCS(double *U, double *V, double *W,
                         int R, int G, int B)
{
  double X, Y, Z;

  QccIMGImageRGBtoXYZ(&X, &Y, &Z, R, G, B);
  *U = 2*X / 3;
  *V = Y;
  *W = -0.5*X + 1.5*Y + 0.5*Z; 
}


void QccIMGImageUCStoRGB(int *R, int *G, int *B,
                         double U, double V, double W)
{
  double X, Y, Z;

  X = 1.5*U;
  Y = V;
  Z = 2*(W + 0.5*X - 1.5*Y);

  QccIMGImageXYZtoRGB(R, G, B, X, Y, Z);
}


void QccIMGImageRGBtoModifiedUCS(double *U, double *V, double *W,
                                 int R, int G, int B)
{
  double U1, V1, W1;      /*  tristimulus values  */
  double u1, v1;          /*  chromaticities      */
  double C;
  double U_star, V_star, W_star;

  QccIMGImageRGBtoUCS(&U1, &V1, &W1, R, G, B);

  if (V1 < 0.01)
    {
      /*  black  */
      W_star = U_star = V_star = 0.0;
      goto Return;
    }

  C = U1 + V1 + W1;
  if (C == 0)
    u1 = v1 = 0.0;
  else
    {
      u1 = U1/C;
      v1 = V1/C;
    }

  W_star = 25*cbrt(100*V1) - 17;
  U_star = 13*W_star*(u1 - 4.0/19);
  V_star = 13*W_star*(v1 - 6.0/19);

 Return:
  if (U != NULL)
    *U = U_star;
  if (V != NULL)
    *V = V_star;
  if (W != NULL)
    *W = W_star;
}


void QccIMGImageModifiedUCStoRGB(int *R, int *G, int *B,
                                 double U, double V, double W)
{
  double u1, v1;           /*  chromaticities      */
  double U1, V1, W1;       /*  tristimulus values  */

  V1 = pow((W + 17)/25, 3)/100;
  u1 = (U/13/W + 4.0/19);
  v1 = (V/13/W + 6.0/19);
  if (v1 == 0.0)
    U1 = W1 = 0.0;
  else
    {
      U1 = u1/v1*V1;
      W1 = (1 - v1 - u1)/v1*V1;
    }

  QccIMGImageUCStoRGB(R, G, B, U1, V1, W1);
}


double QccIMGImageModifiedUCSColorMetric(int R1, int G1, int B1,
                                         int R2, int G2, int B2)
{
  double U1, V1, W1;
  double U2, V2, W2;

  QccIMGImageRGBtoModifiedUCS(&U1, &V1, &W1, R1, G1, B1);
  QccIMGImageRGBtoModifiedUCS(&U2, &V2, &W2, R2, G2, B2);

  return((U1 - U2)*(U1 - U2) + (V1 - V2)*(V1 - V2) +
         (W1 - W2)*(W1 - W2));
}


/*
 * Hue, Saturation, Value (HSV)
 *   This space is merely polar form of Modified UCS; V is value,
 *   or more accurately, brightness
 */

void QccIMGImageRGBtoHSV(double *H, double *S, double *V,
                         int R, int G, int B)
{
  double U, W;

  /*  Here, V is chroma, not value  */
  QccIMGImageRGBtoModifiedUCS(&U, V, &W, R, G, B);

  *S = sqrt(U*U + (*V)*(*V));
  if (U == 0.0)
    *H = 0;
  else
    *H = atan2(*V, U) + M_PI;

  /*  Now, set V to value  */
  *V = W;
}


void QccIMGImageHSVtoRGB(int *R, int *G, int *B,
                         double H, double S, double V)
{
  double U, W;

  /*  Set W to value; hereafter V becomes chroma  */
  W = V;

  U = S * cos(H);
  V = S * sin(H);

  QccIMGImageModifiedUCStoRGB(R, G, B,
                              U, V, W);
}


