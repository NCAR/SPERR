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


static void QccWAVCDF97AnalysisIntSymmetricEvenEven(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 203, 6);
  
  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 217, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 113, 6);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 1817, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 153, 9);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 3563, 11);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 11, 5);

}


static void QccWAVCDF97AnalysisIntSymmetricEvenOdd(QccVectorInt signal,
                                                   int signal_length)
{
  int index;

  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 203, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 217, 11);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 113, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 1817, 11);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 153, 9);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 3563, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 11, 5);

}


static void QccWAVCDF97AnalysisIntSymmetricOddEven(QccVectorInt signal,
                                                   int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  
  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 217, 11);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 217, 11);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 1817, 11);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 1817, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 153, 9);
  signal[signal_length - 1] +=
      QccWAVLiftingIntRound(signal[signal_length - 2], 153, 9);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 3563, 11);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 11, 5);
  signal[signal_length - 1] -=
      QccWAVLiftingIntRound(signal[signal_length - 2], 11, 5);

}


static void QccWAVCDF97AnalysisIntSymmetricOddOdd(QccVectorInt signal,
                                                  int signal_length)
{
  int index;

  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 203, 6);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 203, 6);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 113, 6);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 113, 6);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] = signal[index + 1] - signal[index];
  signal[signal_length - 1] =
    signal[signal_length - 2] - signal[signal_length - 1];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 153, 9);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 3563, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2], 3563, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 11, 5);

}


static void QccWAVCDF97AnalysisIntPeriodicEvenEven(QccVectorInt signal,
                                                   int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2] + signal[0], 203, 6);
  
  signal[0] -=
    QccWAVLiftingIntRound(signal[1] + signal[signal_length - 1], 217, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2] + signal[0], 113, 6);
  
  signal[0] +=
    QccWAVLiftingIntRound(signal[1] + signal[signal_length - 1], 1817, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 153, 9);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 3563, 11);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 11, 5);

}


static void QccWAVCDF97AnalysisIntPeriodicEvenOdd(QccVectorInt signal,
                                                  int signal_length)
{
  int index;
  
  signal[0] -=
    QccWAVLiftingIntRound(signal[signal_length - 1] + signal[1], 203, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[0] + signal[signal_length - 2], 217, 11);
  
  signal[0] +=
    QccWAVLiftingIntRound(signal[signal_length - 1] + signal[1], 113, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[0] + signal[signal_length - 2], 1817, 11);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 153, 9);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 3563, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 11, 5);

}


int QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7(QccVectorInt signal,
                                                       int signal_length,
                                                       int phase,
                                                       int boundary)
{
  if (signal == NULL)
    return(0);
  
  if (!signal_length)
    return(0);
  
  if (signal_length == 1)
    return(0);
  
  if (boundary == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET)
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Boundary wavelets not supported");
      return(1);
    }
  
  switch (phase)
    {
    case QCCWAVWAVELET_PHASE_EVEN:
      switch (signal_length % 2)
        {
        case 0:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisIntSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97AnalysisIntPeriodicEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisIntSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
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
              QccWAVCDF97AnalysisIntSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97AnalysisIntPeriodicEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97AnalysisIntSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }
  
  return(0);
}


static void QccWAVCDF97SynthesisIntSymmetricEvenEven(QccVectorInt signal,
                                                     int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 11, 5);

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 3563, 11);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 153, 9);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 1817, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 113, 6);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 217, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 203, 6);
  
}


static void QccWAVCDF97SynthesisIntSymmetricEvenOdd(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 11, 5);

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 3563, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 153, 9);

  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 1817, 11);
  
  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 113, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 217, 11);

  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 203, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);

}


static void QccWAVCDF97SynthesisIntSymmetricOddEven(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 11, 5);
  signal[signal_length - 1] +=
      QccWAVLiftingIntRound(signal[signal_length - 2], 11, 5);

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 3563, 11);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 153, 9);
  signal[signal_length - 1] -=
      QccWAVLiftingIntRound(signal[signal_length - 2], 153, 9);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 1817, 11);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 1817, 11);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 217, 11);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 217, 11);

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);

}


static void QccWAVCDF97SynthesisIntSymmetricOddOdd(QccVectorInt signal,
                                                   int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 11, 5);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 3563, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2], 3563, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 153, 9);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] = signal[index + 1] - signal[index];
  signal[signal_length - 1] =
    signal[signal_length - 2] - signal[signal_length - 1];

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);

  signal[0] -=
    QccWAVLiftingIntRound(2 * signal[1], 113, 6);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 113, 6);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  
  signal[0] +=
    QccWAVLiftingIntRound(2 * signal[1], 203, 6);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(2 * signal[signal_length - 2], 203, 6);
  
}


static void QccWAVCDF97SynthesisIntPeriodicEvenEven(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 11, 5);

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 3563, 11);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 153, 9);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  signal[0] -=
    QccWAVLiftingIntRound(signal[1] + signal[signal_length - 1], 1817, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2] + signal[0], 113, 6);
  
  signal[0] +=
    QccWAVLiftingIntRound(signal[1] + signal[signal_length - 1], 217, 11);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2] + signal[0], 203, 6);
  
}


static void QccWAVCDF97SynthesisIntPeriodicEvenOdd(QccVectorInt signal,
                                                   int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 11, 5);

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 3563, 11);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 153, 9);

  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1817, 11);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[0] + signal[signal_length - 2], 1817, 11);
  
  signal[0] -=
    QccWAVLiftingIntRound(signal[signal_length - 1] + signal[1], 113, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 113, 6);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 217, 11);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[0] + signal[signal_length - 2], 217, 11);
  
  signal[0] +=
    QccWAVLiftingIntRound(signal[signal_length - 1] + signal[1], 203, 6);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 203, 6);
  
}


int QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7(QccVectorInt signal,
                                                        int signal_length,
                                                        int phase,
                                                        int boundary)
{
  if (signal == NULL)
    return(0);
  
  if (!signal_length)
    return(0);
  
  if (signal_length == 1)
    return(0);
  
  if (boundary == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET)
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Boundary wavelets not supported");
      return(1);
    }
  
  switch (phase)
    {
    case QCCWAVWAVELET_PHASE_EVEN:
      switch (signal_length % 2)
        {
        case 0:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisIntSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97SynthesisIntPeriodicEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisIntSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
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
              QccWAVCDF97SynthesisIntSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF97SynthesisIntPeriodicEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF97SynthesisIntSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau9_7): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }

  return(0);
}
