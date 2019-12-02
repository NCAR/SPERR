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


static void QccWAVCDF53AnalysisSymmetricEvenEven(QccVector signal,
                                                 int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] -= signal[signal_length - 2];
  
  signal[0] = M_SQRT2 *
    (signal[0] + signal[1] / 2);
  for (index = 2; index < signal_length; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisSymmetricEvenOdd(QccVector signal,
                                                int signal_length)
{
  int index;

  signal[0] -= signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  signal[signal_length - 1] = M_SQRT2 *
    (signal[signal_length - 1] + signal[signal_length - 2] / 2);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisSymmetricOddEven(QccVector signal,
                                                int signal_length)
{
  int index;

  for (index = 1; index < signal_length; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  
  signal[0] = M_SQRT2 *
    (signal[0] + signal[1] / 2);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  signal[signal_length - 1] = M_SQRT2 *
    (signal[signal_length - 1] + signal[signal_length - 2] / 2);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisSymmetricOddOdd(QccVector signal,
                                               int signal_length)
{
  int index;

  signal[0] -= signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] -= signal[signal_length - 2];

  for (index = 1; index < signal_length; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisPeriodicEvenEven(QccVector signal,
                                                int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] -=
    (signal[signal_length - 2] + signal[0]) / 2;
  
  signal[0] = M_SQRT2 *
    (signal[0] + (signal[1] + signal[signal_length - 1]) / 4);
  for (index = 2; index < signal_length; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisPeriodicEvenOdd(QccVector signal,
                                               int signal_length)
{
  int index;

  signal[0] -=
    (signal[signal_length - 1] + signal[1]) / 2;
  for (index = 2; index < signal_length; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  signal[signal_length - 1] = M_SQRT2 *
    (signal[signal_length - 1] + (signal[0] + signal[signal_length - 2]) / 4);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisBoundaryEvenEven(QccVector signal,
                                                int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] -=
    (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 2;
  
  signal[0] = M_SQRT2 *
    (signal[0] + (3 * signal[1] - signal[3]) / 4);
  for (index = 2; index < signal_length; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisBoundaryEvenOdd(QccVector signal,
                                               int signal_length)
{
  int index;

  signal[0] -=
    (3 * signal[1] - signal[3]) / 2;
  for (index = 2; index < signal_length; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  signal[signal_length - 1] = M_SQRT2 *
    (signal[signal_length - 1] +
     (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 4);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisBoundaryOddEven(QccVector signal,
                                               int signal_length)
{
  int index;

  for (index = 1; index < signal_length; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  
  signal[0] = M_SQRT2 *
    (signal[0] + (3 * signal[1] - signal[3]) / 4);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  signal[signal_length - 1] = M_SQRT2 *
    (signal[signal_length - 1] +
     (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 4);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


static void QccWAVCDF53AnalysisBoundaryOddOdd(QccVector signal,
                                              int signal_length)
{
  int index;

  signal[0] -= (3 * signal[1] - signal[3]) / 2;
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -= (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] -= 
    (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 2;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] = M_SQRT2 *
      (signal[index] + (signal[index + 1] + signal[index - 1]) / 4);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= -M_SQRT2;
}


int QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3(QccVector signal,
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
              QccWAVCDF53AnalysisSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53AnalysisPeriodicEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53AnalysisBoundaryEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53AnalysisSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53AnalysisBoundaryOddEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3): Undefined boundary method");
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
              QccWAVCDF53AnalysisSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53AnalysisPeriodicEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53AnalysisBoundaryEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53AnalysisSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53AnalysisBoundaryOddOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }
  
  return(0);
}


static void QccWAVCDF53SynthesisSymmetricEvenEven(QccVector signal,
                                                  int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;
  
  signal[0] =
    signal[0] / M_SQRT2 - signal[1] / 2;
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      signal[index] / M_SQRT2 -
      (signal[index + 1] + signal[index - 1]) / 4;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] += signal[signal_length - 2];
}


static void QccWAVCDF53SynthesisSymmetricEvenOdd(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] =
      signal[index] / M_SQRT2 - (signal[index + 1] + signal[index - 1]) / 4;
  signal[signal_length - 1] = 
    signal[signal_length - 1] / M_SQRT2 - signal[signal_length - 2] / 2;
  
  signal[0] += signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  
}


static void QccWAVCDF53SynthesisSymmetricOddEven(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;
  
  signal[0] = 
    signal[0] / M_SQRT2 - signal[1] / 2;
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = 
      signal[index] / M_SQRT2 -
      (signal[index + 1] + signal[index - 1]) / 4;
  signal[signal_length - 1] = 
    signal[signal_length - 1] / M_SQRT2 - signal[signal_length - 2] / 2;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  
}


static void QccWAVCDF53SynthesisSymmetricOddOdd(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;

  for (index = 1; index < signal_length; index += 2)
    signal[index] = 
      signal[index] / M_SQRT2 - (signal[index + 1] + signal[index - 1]) / 4;

  signal[0] += signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] += signal[signal_length - 2];
  
}


static void QccWAVCDF53SynthesisPeriodicEvenEven(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;
  
  signal[0] =
    signal[0] / M_SQRT2 - (signal[1] + signal[signal_length - 1]) / 4;
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      signal[index] / M_SQRT2 -
      (signal[index + 1] + signal[index - 1]) / 4;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] +=
    (signal[signal_length - 2] + signal[0]) / 2;
}


static void QccWAVCDF53SynthesisPeriodicEvenOdd(QccVector signal,
                                                int signal_length)
{
  int index;

  for (index = 0; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] = signal[index] / M_SQRT2 -
      (signal[index + 1] + signal[index - 1]) / 4;
  signal[signal_length - 1] = signal[signal_length - 1] / M_SQRT2 -
    (signal[0] + signal[signal_length - 2]) / 4;
  
  signal[0] +=
    (signal[signal_length - 1] + signal[1]) / 2;
  for (index = 2; index < signal_length; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
}


static void QccWAVCDF53SynthesisBoundaryEvenEven(QccVector signal,
                                                 int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;
  
  signal[0] =
    signal[0] / M_SQRT2 - (3 * signal[1] - signal[3]) / 4;
  for (index = 2; index < signal_length; index += 2)
    signal[index] =
      signal[index] / M_SQRT2 -
      (signal[index + 1] + signal[index - 1]) / 4;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] +=
    (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 2;
}


static void QccWAVCDF53SynthesisBoundaryEvenOdd(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] = 
      signal[index] / M_SQRT2 - (signal[index + 1] + signal[index - 1]) / 4;
  signal[signal_length - 1] = 
    signal[signal_length - 1] / M_SQRT2 -
     (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 4;
  
  signal[0] +=
    (3 * signal[1] - signal[3]) / 2;
  for (index = 2; index < signal_length; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
}


static void QccWAVCDF53SynthesisBoundaryOddEven(QccVector signal,
                                                int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;
  
  signal[0] =
    signal[0] / M_SQRT2 - (3 * signal[1] - signal[3]) / 4;
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] = 
      signal[index] / M_SQRT2 -
      (signal[index + 1] + signal[index - 1]) / 4;
  signal[signal_length - 1] = 
    signal[signal_length - 1] / M_SQRT2 -
    (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 4;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  
}


static void QccWAVCDF53SynthesisBoundaryOddOdd(QccVector signal,
                                               int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= -M_SQRT2;

  for (index = 1; index < signal_length; index += 2)
    signal[index] = 
      signal[index] / M_SQRT2 - (signal[index + 1] + signal[index - 1]) / 4;
  
  signal[0] += (3 * signal[1] - signal[3]) / 2;
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] += (signal[index - 1] + signal[index + 1]) / 2;
  signal[signal_length - 1] += 
    (3 * signal[signal_length - 2] - signal[signal_length - 4]) / 2;
  
}


int QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3(QccVector signal,
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
              QccWAVCDF53SynthesisSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53SynthesisPeriodicEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53SynthesisBoundaryEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53SynthesisSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53SynthesisBoundaryOddEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3): Undefined boundary method");
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
              QccWAVCDF53SynthesisSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53SynthesisPeriodicEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53SynthesisBoundaryEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53SynthesisSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
              QccWAVCDF53SynthesisBoundaryOddOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }

  return(0);
}

