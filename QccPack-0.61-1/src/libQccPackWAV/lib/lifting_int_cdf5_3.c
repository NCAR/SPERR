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


static void QccWAVCDF53AnalysisIntSymmetricEvenEven(QccVectorInt signal,
                                                    int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  signal[signal_length - 1] -= signal[signal_length - 2];
  
  signal[0] += QccWAVLiftingIntRound(signal[1], 1, 0);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 53, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 181, 7);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 75, 6);

}


static void QccWAVCDF53AnalysisIntSymmetricEvenOdd(QccVectorInt signal,
                                                   int signal_length)
{
  int index;

  signal[0] -= signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2], 1, 0);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 53, 6);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 181, 7);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 75, 6);

}


static void QccWAVCDF53AnalysisIntSymmetricOddEven(QccVectorInt signal,
                                                   int signal_length)
{
  int index;

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  
  signal[0] += QccWAVLiftingIntRound(signal[1], 1, 0);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2], 1, 0);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 53, 6);
  signal[signal_length - 1] +=
      QccWAVLiftingIntRound(signal[signal_length - 2], 53, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 181, 7);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 75, 6);
  signal[signal_length - 1] -=
      QccWAVLiftingIntRound(signal[signal_length - 2], 75, 6);

}


static void QccWAVCDF53AnalysisIntSymmetricOddOdd(QccVectorInt signal,
                                                  int signal_length)
{
  int index;

  signal[0] -= signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  signal[signal_length - 1] -= signal[signal_length - 2];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] = signal[index + 1] - signal[index];
  signal[signal_length - 1] =
    signal[signal_length - 2] - signal[signal_length - 1];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 53, 6);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 181, 7);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2], 181, 7);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 75, 6);

}


static void QccWAVCDF53AnalysisIntPeriodicEvenEven(QccVectorInt signal,
                                                   int signal_length)
{
  int index;

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2] + signal[0], 1, 0);
  
  signal[0] +=
    QccWAVLiftingIntRound(signal[1] + signal[signal_length - 1], 1, 1);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 53, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 181, 7);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 75, 6);

}


static void QccWAVCDF53AnalysisIntPeriodicEvenOdd(QccVectorInt signal,
                                                  int signal_length)
{
  int index;

  signal[0] -=
    QccWAVLiftingIntRound(signal[signal_length - 1] + signal[1], 1, 0);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  
  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[0] + signal[signal_length - 2], 1, 1);

  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 53, 6);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 181, 7);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 75, 6);

}


int QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3(QccVectorInt signal,
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
              QccWAVCDF53AnalysisIntSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53AnalysisIntPeriodicEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53AnalysisIntSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
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
              QccWAVCDF53AnalysisIntSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53AnalysisIntPeriodicEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53AnalysisIntSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingAnalysisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }
  
  return(0);
}


static void QccWAVCDF53SynthesisIntSymmetricEvenEven(QccVectorInt signal,
                                                     int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 75, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 181, 7);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 53, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  signal[0] -= QccWAVLiftingIntRound(signal[1], 1, 0);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  signal[signal_length - 1] += signal[signal_length - 2];
  
}


static void QccWAVCDF53SynthesisIntSymmetricEvenOdd(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 75, 6);

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 181, 7);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 53, 6);

  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2], 1, 0);
  
  signal[0] += signal[1];
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  
}


static void QccWAVCDF53SynthesisIntSymmetricOddEven(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 75, 6);
  signal[signal_length - 1] +=
      QccWAVLiftingIntRound(signal[signal_length - 2], 75, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 181, 7);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 53, 6);
  signal[signal_length - 1] -=
      QccWAVLiftingIntRound(signal[signal_length - 2], 53, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  signal[0] -= QccWAVLiftingIntRound(signal[1], 1, 0);
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[signal_length - 2], 1, 0);
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  
}


static void QccWAVCDF53SynthesisIntSymmetricOddOdd(QccVectorInt signal,
                                                   int signal_length)
{
  int index;
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 75, 6);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 181, 7);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2], 181, 7);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 53, 6);

  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] = signal[index + 1] - signal[index];
  signal[signal_length - 1] =
    signal[signal_length - 2] - signal[signal_length - 1];

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);

  signal[0] += signal[1];
  for (index = 2; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  signal[signal_length - 1] += signal[signal_length - 2];

}


static void QccWAVCDF53SynthesisIntPeriodicEvenEven(QccVectorInt signal,
                                                    int signal_length)
{
  int index;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 75, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 181, 7);

  for (index = 0; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1], 53, 6);

  for (index = 1; index < signal_length; index += 2)
    signal[index] = signal[index - 1] - signal[index];

  signal[0] -=
    QccWAVLiftingIntRound(signal[1] + signal[signal_length - 1], 1, 1);
  for (index = 2; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  signal[signal_length - 1] +=
    QccWAVLiftingIntRound(signal[signal_length - 2] + signal[0], 1, 0);
  
}


static void QccWAVCDF53SynthesisIntPeriodicEvenOdd(QccVectorInt signal,
                                                   int signal_length)
{
  int index;

  for (index = 1; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1], 75, 6);

  for (index = 0; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index + 1], 181, 7);

  for (index = 1; index < signal_length; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index - 1], 53, 6);

  for (index = 0; index < signal_length; index += 2)
    signal[index] = signal[index + 1] - signal[index];

  for (index = 1; index < signal_length - 1; index += 2)
    signal[index] -=
      QccWAVLiftingIntRound(signal[index + 1] + signal[index - 1], 1, 1);
  signal[signal_length - 1] -=
    QccWAVLiftingIntRound(signal[0] + signal[signal_length - 2], 1, 1);

  signal[0] +=
    QccWAVLiftingIntRound(signal[signal_length - 1] + signal[1], 1, 0);
  for (index = 2; index < signal_length; index += 2)
    signal[index] +=
      QccWAVLiftingIntRound(signal[index - 1] + signal[index + 1], 1, 0);
  
}


int QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3(QccVectorInt signal,
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
              QccWAVCDF53SynthesisIntSymmetricEvenEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53SynthesisIntPeriodicEvenEven(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53SynthesisIntSymmetricOddEven(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
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
              QccWAVCDF53SynthesisIntSymmetricEvenOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccWAVCDF53SynthesisIntPeriodicEvenOdd(signal, signal_length);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
          
        case 1:
          switch (boundary)
            {
            case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
              QccWAVCDF53SynthesisIntSymmetricOddOdd(signal, signal_length);
              break;
              
            case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3): Signal length must be even for periodic extension");
              return(1);
              break;
              
            default:
              QccErrorAddMessage("(QccWAVLiftingSynthesisIntCohenDaubechiesFeauveau5_3): Undefined boundary method");
              return(1);
            }
          break;
        }
      break;
    }

  return(0);
}
