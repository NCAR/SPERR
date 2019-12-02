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


/*  sqrt(3)  */
#define A1 1.73205080756888
/*  sqrt(3)/4  */
#define A2 0.43301270189222
/*  (sqrt(3) - 2)/4  */
#define A3 -0.06698729810778
/*  (sqrt(3) + 1)/sqrt(2)  */
#define A4 1.93185165257814
/*  (sqrt(3) - 1)/sqrt(2)  */
#define A5 0.51763809020504
#define A6 (A2 + 2 * A3)


static void QccWAVD4AnalysisPeriodicEvenEven(QccVector signal,
                                             int signal_length)
{
  int index;
  double tmp;

  for (index = 1; index < signal_length; index += 2)
    signal[index] -= 
      A1 * signal[index - 1];
  
  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] += 
      A2 * signal[index + 1] + A3 * signal[index + 3];
  signal[signal_length - 2] +=
    A2 * signal[signal_length - 1] + A3 * signal[1];
  
  signal[1] =
    A5 * (signal[1] + signal[signal_length - 2]);
  for (index = 3; index < signal_length; index += 2)
    signal[index] =
      A5 * (signal[index] + signal[index - 3]);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= A4; 
  
  tmp = signal[1];
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] = signal[index + 2];
  signal[signal_length - 1] = tmp;
}


static void QccWAVD4AnalysisPeriodicEvenOdd(QccVector signal,
                                            int signal_length)
{

}


static void QccWAVD4AnalysisBoundaryEvenEven(QccVector signal,
                                             int signal_length)
{
  int index;
  double tmp;

  for (index = 1; index < signal_length; index += 2)
    signal[index] -= 
      A1 * signal[index - 1];
  
  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] += 
      A2 * signal[index + 1] + A3 * signal[index + 3];
  signal[signal_length - 2] +=
    A6 * signal[signal_length - 1] - signal[signal_length - 3];
  
  signal[1] =
    A5 * (signal[1] + (2*signal[0] - signal[2]));
  for (index = 3; index < signal_length; index += 2)
    signal[index] =
      A5 * (signal[index] + signal[index - 3]);
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] *= A4; 
  
  tmp = signal[1];
  for (index = 1; index < signal_length - 2; index += 2)
    signal[index] = signal[index + 2];
  signal[signal_length - 1] = tmp;
}


static void QccWAVD4AnalysisBoundaryEvenOdd(QccVector signal,
                                            int signal_length)
{
}


int QccWAVLiftingAnalysisDaubechies4(QccVector signal,
                                     int signal_length,
                                     int phase,
                                     int boundary)
{
  if (signal == NULL)
    return(0);
  
  if (signal_length % 2)
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisDaubechies4): Signal length must be even");
      return(1);
    }
  
  if (phase == QCCWAVWAVELET_PHASE_ODD)
    {
      QccErrorAddMessage("(QccWAVLiftingAnalysisDaubechies4): Odd phase not supported");
      return(1);
    }
  
  switch (phase)
    {
    case QCCWAVWAVELET_PHASE_EVEN:
      switch (boundary)
        {
        case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
          QccErrorAddMessage("(QccWAVLiftingAnalysisDaubechies4): Symmetric extension not allowed");
          return(1);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
          QccWAVD4AnalysisPeriodicEvenEven(signal, signal_length);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
          QccWAVD4AnalysisBoundaryEvenEven(signal, signal_length);
          break;
          
        default:
          QccErrorAddMessage("(QccWAVLiftingAnalysisDaubechies4): Undefined boundary method");
          return(1);
        }
      break;
      
    case QCCWAVWAVELET_PHASE_ODD:
      switch (boundary)
        {
        case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
          QccErrorAddMessage("(QccWAVLiftingAnalysisDaubechies4): Symmetric extension not allowed");
          return(1);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
          QccWAVD4AnalysisPeriodicEvenOdd(signal, signal_length);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
          QccWAVD4AnalysisBoundaryEvenOdd(signal, signal_length);
          break;
          
        default:
          QccErrorAddMessage("(QccWAVLiftingAnalysisDaubechies4): Undefined boundary method");
          return(1);
        }
      break;
    }      
  
  return(0);
}


static void QccWAVD4SynthesisPeriodicEvenEven(QccVector signal,
                                              int signal_length)
{
  int index;
  double tmp;

  tmp = signal[signal_length - 1];
  for (index = signal_length - 1; index > 1; index -= 2)
    signal[index] = signal[index - 2];
  signal[1] = tmp;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= A4; 
  
  signal[1] =
    (signal[1]/A5 - signal[signal_length - 2]);
  for (index = 3; index < signal_length; index += 2)
    signal[index] =
      (signal[index]/A5 - signal[index - 3]);
  
  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -= 
      A2 * signal[index + 1] + A3 * signal[index + 3];
  signal[signal_length - 2] -= 
    A2 * signal[signal_length - 1] + A3 * signal[1];
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] += A1 * signal[index - 1];
}


static void QccWAVD4SynthesisPeriodicEvenOdd(QccVector signal,
                                             int signal_length)
{
  
}


static void QccWAVD4SynthesisBoundaryEvenEven(QccVector signal,
                                              int signal_length)
{
  int index;
  double tmp;
  
  tmp = signal[signal_length - 1];
  for (index = signal_length - 1; index > 1; index -= 2)
    signal[index] = signal[index - 2];
  signal[1] = tmp;
  
  for (index = 0; index < signal_length; index += 2)
    signal[index] /= A4; 
  
  signal[1] =
    signal[1]/A5 - (2*signal[0] - signal[2]);
  for (index = 3; index < signal_length; index += 2)
    signal[index] =
      signal[index]/A5 - signal[index - 3];
  
  for (index = 0; index < signal_length - 2; index += 2)
    signal[index] -= 
      A2 * signal[index + 1] + A3 * signal[index + 3];
  signal[signal_length - 2] -=
    A6 * signal[signal_length - 1] - signal[signal_length - 3];
  
  for (index = 1; index < signal_length; index += 2)
    signal[index] += 
      A1 * signal[index - 1];
}


static void QccWAVD4SynthesisBoundaryEvenOdd(QccVector signal,
                                             int signal_length)
{

}


int QccWAVLiftingSynthesisDaubechies4(QccVector signal,
                                      int signal_length,
                                      int phase,
                                      int boundary)
{
  if (signal == NULL)
    return(0);

  if (signal_length % 2)
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisDaubechies4): Signal length must be even");
      return(1);
    }

  if (phase == QCCWAVWAVELET_PHASE_ODD)
    {
      QccErrorAddMessage("(QccWAVLiftingSynthesisDaubechies4): Odd phase not supported");
      return(1);
    }

  switch (phase)
    {
    case QCCWAVWAVELET_PHASE_EVEN:
      switch (boundary)
        {
        case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
          QccErrorAddMessage("(QccWAVLiftingSynthesisDaubechies4): Symmetric extension not allowed");
          return(1);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
          QccWAVD4SynthesisPeriodicEvenEven(signal, signal_length);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
          QccWAVD4SynthesisBoundaryEvenEven(signal, signal_length);
          break;
          
        default:
          QccErrorAddMessage("(QccWAVLiftingSynthesisDaubechies4): Undefined boundary method");
          return(1);
        }
      break;

    case QCCWAVWAVELET_PHASE_ODD:
      switch (boundary)
        {
        case QCCWAVWAVELET_BOUNDARY_SYMMETRIC_EXTENSION:
          QccErrorAddMessage("(QccWAVLiftingSynthesisDaubechies4): Symmetric extension not allowed");
          return(1);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION:
          QccWAVD4SynthesisPeriodicEvenOdd(signal, signal_length);
          break;
          
        case QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET:
          QccWAVD4SynthesisBoundaryEvenOdd(signal, signal_length);
          break;
          
        default:
          QccErrorAddMessage("(QccWAVLiftingSynthesisDaubechies4): Undefined boundary method");
          return(1);
        }
      break;
    }
      
  return(0);
}
