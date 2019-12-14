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


#define ALPHA     -1.58615986717275
#define BETA      -0.05297864003258
#define GAMMA      0.88293362717904
#define DELTA      0.44350482244527
#define EPSILON    1.14960430535816

#define SAM

static void QccWAVCDF97AnalysisSymmetricEvenEven(QccVector signal,
                                                 int signal_length)
{
#ifdef SAM
fprintf( stderr, "signal_len = %d, using Even Even\n", signal_length );
#endif

  int index;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    2 * ALPHA * signal[signal_length - 2];
  
  signal[0] +=
    2 * BETA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    2 * GAMMA * signal[signal_length - 2];
  
  signal[0] =
    EPSILON * (signal[0] +
               2 * DELTA * signal[1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisSymmetricEvenOdd(QccVector signal,
                                                int signal_length)
{
#ifdef SAM
fprintf( stderr, "signal_len = %d, using Even Odd\n", signal_length );
#endif

  int index;

  signal[0] +=
    2 * ALPHA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] +=
    2 * BETA * signal[signal_length - 2];
  
  signal[0] +=
    2 * GAMMA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  signal[signal_length - 1] =
    EPSILON * (signal[signal_length - 1] +
               2 * DELTA * signal[signal_length - 2]);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisSymmetricOddEven(QccVector signal,
                                                int signal_length)
{
#ifdef SAM
fprintf( stderr, "signal_len = %d, using Odd Even\n", signal_length );
#endif

  int index;

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] +=
    2 * BETA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] +=
    2 * BETA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] =
    EPSILON * (signal[0] +
               2 * DELTA * signal[1]);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  signal[signal_length - 1] =
    EPSILON * (signal[signal_length - 1] +
               2 * DELTA * signal[signal_length - 2]);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisSymmetricOddOdd(QccVector signal,
                                               int signal_length)
{
#ifdef SAM
fprintf( stderr, "signal_len = %d, using Odd Odd\n", signal_length );
#endif

  int index;

  signal[0] +=
    2 * ALPHA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    2 * ALPHA * signal[signal_length - 2];
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  signal[0] +=
    2 * GAMMA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    2 * GAMMA * signal[signal_length - 2];
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisPeriodicEvenEven(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    ALPHA * (signal[signal_length - 2] + signal[0]);
  
  signal[0] +=
    BETA * (signal[1] + signal[signal_length - 1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    GAMMA * (signal[signal_length - 2] + signal[0]);
  
  signal[0] =
    EPSILON * (signal[0] +
               DELTA * (signal[1] + signal[signal_length - 1]));
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisPeriodicEvenOdd(QccVector signal,
                                               int signal_length)
{
  int index;
  
  signal[0] +=
    ALPHA * (signal[signal_length - 1] + signal[1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] +=
    BETA * (signal[0] + signal[signal_length - 2]);
  
  signal[0] +=
    GAMMA * (signal[signal_length - 1] + signal[1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  signal[signal_length - 1] =
    EPSILON * (signal[signal_length - 1] +
               DELTA * (signal[0] + signal[signal_length - 2]));
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisBoundaryEvenEven(QccVector signal,
                                                int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    ALPHA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  signal[0] +=
    BETA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    GAMMA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  signal[0] =
    EPSILON * (signal[0] +
               DELTA * (3*signal[1] - signal[3]));
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisBoundaryEvenOdd(QccVector signal,
                                               int signal_length)
{
  int index;

  signal[0] +=
    ALPHA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] +=
    BETA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  signal[0] +=
    GAMMA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  signal[signal_length - 1] =
    EPSILON * (signal[signal_length - 1] +
               DELTA * (3*signal[signal_length - 2] -
                        signal[signal_length - 4]));
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisBoundaryOddEven(QccVector signal,
                                               int signal_length)
{
  int index;

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] +=
    BETA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] +=
    BETA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  signal[0] =
    EPSILON * (signal[0] +
               DELTA * (3*signal[1] - signal[3]));
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  signal[signal_length - 1] =
    EPSILON * (signal[signal_length - 1] +
               DELTA * (3*signal[signal_length - 2] -
                        signal[signal_length - 4]));
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


static void QccWAVCDF97AnalysisBoundaryOddOdd(QccVector signal,
                                              int signal_length)
{
  int index;

  signal[0] +=
    ALPHA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] +=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    ALPHA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  signal[0] +=
    GAMMA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] +=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] +=
    GAMMA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] =
      EPSILON * (signal[index] + 
                 DELTA * (signal[index + 1] + signal[index - 1]));
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= (-EPSILON);
}


int QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7(QccVector signal,
                                                    int signal_length,
                                                    int phase,
                                                    int boundary)
{
  if (signal == NULL)
    return(0);
  
  if (!signal_length)
    return(0);
  
  if (signal_length == 1)
    {
      if (phase == QCCWAVWAVELET_PHASE_EVEN)
        signal[0] *= M_SQRT2;
      else
        signal[0] /= M_SQRT2;
      return(0);
    }
  
  if ((boundary == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET) &&
      ((signal_length == 2) || (signal_length == 3)))
    boundary = QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION;
  
  switch (phase)
    {
    case QCCWAVWAVELET_PHASE_EVEN:
      switch (signal_length % 2)
        {
        case 0:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97AnalysisPeriodicEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97AnalysisBoundaryEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97AnalysisBoundaryOddEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;

    case QCCWAVWAVELET_PHASE_ODD:
      switch (signal_length % 2)
        {
        case 0:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97AnalysisPeriodicEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97AnalysisBoundaryEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97AnalysisBoundaryOddOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }
  
  return(0);
}


static void QccWAVCDF97SynthesisSymmetricEvenEven(QccVector signal,
                                                  int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);
  
  signal[0] =
    signal[0]/EPSILON - 2 * DELTA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      signal[index]/EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    2 * GAMMA * signal[signal_length - 2];
  
  signal[0] -=
    2 * BETA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    2 * ALPHA * signal[signal_length - 2];
}


static void QccWAVCDF97SynthesisSymmetricEvenOdd(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] =
      signal[index] / EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] =
    signal[signal_length - 1] / EPSILON -
    2 * DELTA * signal[signal_length - 2];

  signal[0] -=
    2 * GAMMA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] -=
    2 * BETA * signal[signal_length - 2];

  signal[0] -=
    2 * ALPHA * signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
}


static void QccWAVCDF97SynthesisSymmetricOddEven(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] *= (-EPSILON);

  signal[0] =
    signal[0] / EPSILON - 2 * DELTA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] =
      signal[index] / EPSILON -
      DELTA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] =
    signal[signal_length - 1] / EPSILON -
    2 * DELTA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);

  signal[0] -=
    2 * BETA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] -=
    2 * BETA * signal[signal_length - 2];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
}


static void QccWAVCDF97SynthesisSymmetricOddOdd(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  for (index = 1; index < signal_length; index += 2)
    signal[index] =
      signal[index] / EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);

  signal[0] -=
    2 * GAMMA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    2 * GAMMA * signal[signal_length - 2];

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);

  signal[0] -=
    2 * ALPHA * signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    2 * ALPHA * signal[signal_length - 2];
}


static void QccWAVCDF97SynthesisPeriodicEvenEven(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);
  
  signal[0] =
    signal[0]/EPSILON - DELTA * (signal[1] + signal[signal_length - 1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      signal[index]/EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    GAMMA * (signal[signal_length - 2] + signal[0]);
  
  signal[0] -=
    BETA * (signal[1] + signal[signal_length - 1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    ALPHA * (signal[signal_length - 2] + signal[0]);
}


static void QccWAVCDF97SynthesisPeriodicEvenOdd(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] =
      signal[index] / EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] =
    signal[signal_length - 1] / EPSILON -
    DELTA * (signal[0] + signal[signal_length - 2]);

  signal[0] -=
    GAMMA * (signal[signal_length - 1] + signal[1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] -=
    BETA * (signal[0] + signal[signal_length - 2]);
  
  signal[0] -=
    ALPHA * (signal[signal_length - 1] + signal[1]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
}


static void QccWAVCDF97SynthesisBoundaryEvenEven(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);
  
  signal[0] =
    signal[0]/EPSILON - DELTA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      signal[index]/EPSILON -
      DELTA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    GAMMA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  signal[0] -=
    BETA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    ALPHA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
}


static void QccWAVCDF97SynthesisBoundaryEvenOdd(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] =
      signal[index] / EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] =
    signal[signal_length - 1] / EPSILON -
    DELTA * (3*signal[signal_length - 2] -
             signal[signal_length - 4]);
  
  signal[0] -=
    GAMMA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] -=
    BETA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  signal[0] -=
    ALPHA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
}


static void QccWAVCDF97SynthesisBoundaryOddEven(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  signal[0] =
    signal[0] / EPSILON -
    DELTA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] =
      signal[index] / EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] =
    signal[signal_length - 1] / EPSILON -
    DELTA * (3*signal[signal_length - 2] -
             signal[signal_length - 4]);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);

  signal[0] -=
    BETA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);
  signal[signal_length - 1] -=
    BETA * (3*signal[signal_length - 2] - signal[signal_length - 4]);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
}


static void QccWAVCDF97SynthesisBoundaryOddOdd(QccVector signal,
                                               int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= (-EPSILON);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] =
      signal[index] / EPSILON - 
      DELTA * (signal[index + 1] + signal[index - 1]);
  
  signal[0] -=
    GAMMA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] -=
      GAMMA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    GAMMA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      BETA * (signal[index + 1] + signal[index - 1]);

  signal[0] -=
    ALPHA * (3*signal[1] - signal[3]);
  for (index = 2; index < signal_length - 1; index += 2)
    signal[index] -=
      ALPHA * (signal[index - 1] + signal[index + 1]);
  signal[signal_length - 1] -=
    ALPHA * (3*signal[signal_length - 2] - signal[signal_length - 4]);
  
  
}


int QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7(QccVector signal,
                                                     int signal_length,
                                                     int phase,
                                                     int boundary)
{
  if (signal == NULL)
    return(0);
  
  if (!signal_length)
    return(0);
  
  if (signal_length == 1)
    {
      if (phase == QCCWAVWAVELET_PHASE_EVEN)
        signal[0] /= M_SQRT2;
      else
        signal[0] *= M_SQRT2;
      return(0);
    }
  
  if ((boundary == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET) &&
      ((signal_length == 2) || (signal_length == 3)))
    boundary = QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION;
  
  switch (phase)
    {
    case QCCWAVWAVELET_PHASE_EVEN:
      switch (signal_length % 2)
        {
        case 0:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97SynthesisPeriodicEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97SynthesisBoundaryEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97SynthesisBoundaryOddEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;

    case QCCWAVWAVELET_PHASE_ODD:
      switch (signal_length % 2)
        {
        case 0:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97SynthesisPeriodicEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97SynthesisBoundaryEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF97SynthesisBoundaryOddOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }

  return(0);
}
