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


int QccWAVFilterBankInitialize(QccWAVFilterBank *filter_bank)
{
  if (filter_bank == NULL)
    return(0);
  
  QccStringMakeNull(filter_bank->filename);

  QccStringCopy(filter_bank->magic_num, QCCWAVFILTERBANK_MAGICNUM);
  QccGetQccPackVersion(&filter_bank->major_version,
                       &filter_bank->minor_version,
                       NULL);
  
  filter_bank->orthogonality = -1;
  QccFilterInitialize(&filter_bank->lowpass_analysis_filter);
  QccFilterInitialize(&filter_bank->highpass_analysis_filter);
  QccFilterInitialize(&filter_bank->lowpass_synthesis_filter);
  QccFilterInitialize(&filter_bank->highpass_synthesis_filter);
  
  return(0);
}


int QccWAVFilterBankAlloc(QccWAVFilterBank *filter_bank)
{
  int return_value;
  
  if (filter_bank == NULL)
    return(0);
  
  if (QccFilterAlloc(&filter_bank->lowpass_analysis_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankAlloc): Error calling QccFilterAlloc()");
      goto Error;
    }
  if (QccFilterAlloc(&filter_bank->highpass_analysis_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankAlloc): Error calling QccFilterAlloc()");
      goto Error;
    }
  if (QccFilterAlloc(&filter_bank->lowpass_synthesis_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankAlloc): Error calling QccFilterAlloc()");
      goto Error;
    }
  if (QccFilterAlloc(&filter_bank->highpass_synthesis_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankAlloc): Error calling QccFilterAlloc()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVFilterBankFree(QccWAVFilterBank *filter_bank)
{
  if (filter_bank == NULL)
    return;
  
  QccFilterFree(&filter_bank->lowpass_analysis_filter);
  QccFilterFree(&filter_bank->highpass_analysis_filter);
  QccFilterFree(&filter_bank->lowpass_synthesis_filter);
  QccFilterFree(&filter_bank->highpass_synthesis_filter);
  
}


int QccWAVFilterBankPrint(const QccWAVFilterBank *filter_bank)
{
  if (filter_bank == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(filter_bank->filename,
                           filter_bank->magic_num,
                           filter_bank->major_version,
                           filter_bank->minor_version))
    return(1);
  
  
  switch (filter_bank->orthogonality)
    {
    case QCCWAVFILTERBANK_ORTHOGONAL:
      printf("  Orthogonal filter bank\n\n");
      break;
    case QCCWAVFILTERBANK_BIORTHOGONAL:
      printf("  Biorthogonal filter bank\n\n");
      break;
    case QCCWAVFILTERBANK_GENERAL:
    default:
      break;
    }
  
  printf("---------------- Lowpass Analysis Filter ----------------\n");
  if (QccFilterPrint(&filter_bank->lowpass_analysis_filter))
    goto Error;
  
  printf("---------------- Highpass Analysis Filter ----------------\n");
  if (QccFilterPrint(&filter_bank->highpass_analysis_filter))
    goto Error;
  
  printf("---------------- Lowpass Synthesis Filter ----------------\n");
  if (QccFilterPrint(&filter_bank->lowpass_synthesis_filter))
    goto Error;
  
  printf("---------------- Highpass Synthesis Filter ----------------\n");
  if (QccFilterPrint(&filter_bank->highpass_synthesis_filter))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVFilterBankPrint): Error calling QccFilterPrint()");
  return(1);
}


int QccWAVFilterBankMakeOrthogonal(QccWAVFilterBank *filter_bank,
                                   const QccFilter *primary_filter)
{
  if (filter_bank == NULL)
    return(0);
  if (primary_filter == NULL)
    return(0);
  
  if (primary_filter->causality != QCCFILTER_CAUSAL)
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error primary filter must be causal");
      return(1);
    }
  
  filter_bank->lowpass_analysis_filter.length =
    filter_bank->highpass_analysis_filter.length =
    filter_bank->lowpass_synthesis_filter.length =
    filter_bank->highpass_synthesis_filter.length =
    primary_filter->length;
  
  if (QccWAVFilterBankAlloc(filter_bank))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error calling QccWAVFilterBankAlloc()");
      return(1);
    }
  
  /*   h[n]  */
  if (QccFilterCopy(&(filter_bank->lowpass_synthesis_filter),
                    primary_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error calling QccFilterCopy()");
      return(1);
    }
  
  /*  h[-n]  */
  if (QccFilterReversal(&(filter_bank->lowpass_synthesis_filter),
                        &(filter_bank->lowpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error calling QccFilterReversal()");
      return(1);
    }
  
  /*  g[n] = (-(-1)^n) * h[N - 1 - n]  */
  if (QccFilterReversal(&(filter_bank->lowpass_synthesis_filter),
                        &(filter_bank->highpass_synthesis_filter)))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error calling QccFilterReversal()");
      return(1);
    }
  if (QccFilterAlternateSignFlip(&(filter_bank->highpass_synthesis_filter)))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error calling QccFilterAlternateSignFlip()");
      return(1);
    }
  filter_bank->highpass_synthesis_filter.causality =
    filter_bank->lowpass_synthesis_filter.causality;

  /*  g[-n]  */
  if (QccFilterReversal(&(filter_bank->highpass_synthesis_filter),
                        &(filter_bank->highpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeOrthogonal): Error calling QccFilterReversal()");
      return(1);
    }
  
  return(0);
}


int QccWAVFilterBankMakeBiorthogonal(QccWAVFilterBank *filter_bank,
                                     const QccFilter *primary_filter,
                                     const QccFilter *dual_filter)
{
  if (filter_bank == NULL)
    return(0);
  if (primary_filter == NULL)
    return(0);
  if (dual_filter == NULL)
    return(0);
  
  if ((primary_filter->causality != QCCFILTER_SYMMETRICWHOLE) ||
      (dual_filter->causality != QCCFILTER_SYMMETRICWHOLE))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): primary filter and dual filter must by of whole-sample symmetric causality");
      return(1);
    }
  
  filter_bank->lowpass_synthesis_filter.length =
    filter_bank->highpass_analysis_filter.length =
    primary_filter->length;
  
  filter_bank->highpass_synthesis_filter.length =
    filter_bank->lowpass_analysis_filter.length =
    dual_filter->length;
  
  if (QccWAVFilterBankAlloc(filter_bank))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccWAVFilterBankAlloc()");
      return(1);
    }
  
  /*   h[n]  */
  if (QccFilterCopy(&(filter_bank->lowpass_synthesis_filter),
                    primary_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccFilterCopy()");
      return(1);
    }
  
  /*  ~h[-n]  */
  if (QccFilterCopy(&(filter_bank->lowpass_analysis_filter),
                    dual_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccFilterCopy()");
      return(1);
    }
  
  /*  ~g[-n]  */
  if (QccFilterCopy(&(filter_bank->highpass_analysis_filter),
                    primary_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccFilterCopy()");
      return(1);
    }
  if (QccFilterAlternateSignFlip(&(filter_bank->highpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccFilterAlternateSignFlip()");
      return(1);
    }
  
  /*  g[n]  */
  if (QccFilterCopy(&(filter_bank->highpass_synthesis_filter),
                    dual_filter))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccFilterCopy()");
      return(1);
    }
  if (QccFilterAlternateSignFlip(&(filter_bank->highpass_synthesis_filter)))
    {
      QccErrorAddMessage("(QccWAVFilterBankMakeBiorthogonal): Error calling QccFilterAlternateSignFlip()");
      return(1);
    }
  
  return(0);
}


int QccWAVFilterBankBiorthogonal(const QccWAVFilterBank *filter_bank)
{
  return(filter_bank->orthogonality == QCCWAVFILTERBANK_BIORTHOGONAL);
}


static int QccWAVFilterBankReadHeader(FILE *infile,
                                      QccWAVFilterBank *filter_bank)
{
  
  if ((infile == NULL) || (filter_bank == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             filter_bank->magic_num, 
                             &filter_bank->major_version,
                             &filter_bank->minor_version))
    {
      QccErrorAddMessage("(QccWAVFilterBankReadHeader): Error reading magic number in filter bank %s",
                         filter_bank->filename);
      return(1);
    }
  
  if (strcmp(filter_bank->magic_num, QCCWAVFILTERBANK_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVFilterBankReadHeader): %s is not of filter-bank (%s) type",
                         filter_bank->filename, QCCWAVFILTERBANK_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(filter_bank->orthogonality));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccWAVFilterBankReadHeader): Error reading filter-bank orthogonality in filter bank %s",
                         filter_bank->filename);
      return(1);
    }
  
  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccWAVFilterBankReadHeader): Error reading filter-bank orthogonality in filter bank %s",
                         filter_bank->filename);
      return(1);
    }
  
  return(0);
}


static int QccWAVFilterBankReadData(FILE *infile,
                                    QccWAVFilterBank *filter_bank)
{
  int return_value;
  QccFilter primary_filter;
  QccFilter dual_filter;
  
  if (infile == NULL)
    return(0);
  if (filter_bank == NULL)
    return(0);
  
  QccFilterInitialize(&primary_filter);
  QccFilterInitialize(&dual_filter);
  
  switch (filter_bank->orthogonality)
    {
    case QCCWAVFILTERBANK_ORTHOGONAL:
      if (QccFilterRead(infile,
                        &primary_filter))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      if (QccWAVFilterBankMakeOrthogonal(filter_bank, &primary_filter))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccWAVFilterBankMakeOrthogonal()");
          goto Error;
        }
      break;
    case QCCWAVFILTERBANK_BIORTHOGONAL:
      if (QccFilterRead(infile,
                        &primary_filter))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      if (QccFilterRead(infile,
                        &dual_filter))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      if (QccWAVFilterBankMakeBiorthogonal(filter_bank,
                                           &primary_filter,
                                           &dual_filter))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccWAVFilterBankMakeBiorthogonal()");
          goto Error;
        }
      break;
    case QCCWAVFILTERBANK_GENERAL:
      if (QccFilterRead(infile,
                        &(filter_bank->lowpass_analysis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      if (QccFilterRead(infile,
                        &(filter_bank->highpass_analysis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      if (QccFilterRead(infile,
                        &(filter_bank->lowpass_synthesis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      if (QccFilterRead(infile,
                        &(filter_bank->highpass_synthesis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankReadData): Error calling QccFilterRead()");
          goto Error;
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVFilterBankReadData): Undefined orthogonality");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccFilterFree(&primary_filter);
  QccFilterFree(&dual_filter);
  return(return_value);
}


int QccWAVFilterBankRead(QccWAVFilterBank *filter_bank)
{
  FILE *infile = NULL;

  if (filter_bank == NULL)
    return(0);
  
  if ((infile = 
       QccFileOpen(filter_bank->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVFilterBankRead): Error calling QccFileOpen()");
      return(1);
    }

  if (QccWAVFilterBankReadHeader(infile, filter_bank))
    {
      QccErrorAddMessage("(QccWAVFilterBankRead): Error calling QccWAVFilterBankReadHeader()");
      return(1);
    }
  
  if (QccWAVFilterBankReadData(infile, filter_bank))
    {
      QccErrorAddMessage("(QccWAVFilterBankRead): Error calling QccWAVFilterBankReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccWAVFilterBankWriteHeader(FILE *outfile,
                                       const QccWAVFilterBank *filter_bank)
{
  if ((outfile == NULL) || (filter_bank == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCWAVFILTERBANK_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n",
          filter_bank->orthogonality);
  
  if (ferror(outfile))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVFilterBankWriteHeader): Error writing header to %s",
                     filter_bank->filename);
  return(1);
  
}


static int QccWAVFilterBankWriteData(FILE *outfile,
                                     const QccWAVFilterBank *filter_bank)
{
  if (outfile == NULL)
    return(0);
  if (filter_bank == NULL)
    return(0);
  
  switch (filter_bank->orthogonality)
    {
    case QCCWAVFILTERBANK_ORTHOGONAL:
      if (QccFilterWrite(outfile,
                         &(filter_bank->lowpass_synthesis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      break;
    case QCCWAVFILTERBANK_BIORTHOGONAL:
      if (QccFilterWrite(outfile,
                         &(filter_bank->lowpass_synthesis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      if (QccFilterWrite(outfile,
                         &(filter_bank->lowpass_analysis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      break;
    case QCCWAVFILTERBANK_GENERAL:
      if (QccFilterWrite(outfile,
                         &(filter_bank->lowpass_analysis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      if (QccFilterWrite(outfile,
                         &(filter_bank->lowpass_synthesis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      if (QccFilterWrite(outfile,
                         &(filter_bank->highpass_analysis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      if (QccFilterWrite(outfile,
                         &(filter_bank->highpass_synthesis_filter)))
        {
          QccErrorAddMessage("(QccWAVFilterBankWriteData): Error calling QccFilterWrite()");
          return(1);
        }
      break;
    default:
      QccErrorAddMessage("(QccWAVFilterBankWriteData): Undefined orthogonality");
      return(1);
    }
  
  return(0);
}


int QccWAVFilterBankWrite(const QccWAVFilterBank *filter_bank)
{
  FILE *outfile;
  
  if (filter_bank == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(filter_bank->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVFilterBankWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVFilterBankWriteHeader(outfile, filter_bank))
    {
      QccErrorAddMessage("(QccWAVFilterBankWrite): Error calling QccWAVFilterBankWriteHeader()");
      return(1);
    }
  
  if (QccWAVFilterBankWriteData(outfile, filter_bank))
    {
      QccErrorAddMessage("(QccWAVFilterBankWrite): Error calling QccWAVFilterBankWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccWAVFilterBankAnalysis(QccVector signal,
                             int signal_length,
                             int phase,
                             const QccWAVFilterBank *filter_bank,
                             int boundary_extension)
{
  int return_value = 0;
  QccVector lowpass_band;
  int lowpass_band_length;
  int lowpass_subsampling;
  QccVector highpass_band;
  int highpass_band_length;
  int highpass_subsampling;
  QccVector signal2 = NULL;

  if (signal == NULL)
    return(0);
  if (filter_bank == NULL)
    return(0);
  
  if (!signal_length)
    return(0);

  if (boundary_extension == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET)
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Boundary wavelets not supported for filter banks");
      goto Error;
    }

  if ((filter_bank->orthogonality == QCCWAVFILTERBANK_ORTHOGONAL) &&
      (boundary_extension != QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION))
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Orthogonal filter banks must use periodic boundary extension");
      goto Error;
    }
  
  if ((boundary_extension == QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION) &&
      (signal_length % 2))
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Signal length must be even for periodic extension");
      goto Error;
    }

  if (signal_length == 1)
    {
      if (phase == QCCWAVFILTERBANK_PHASE_EVEN)
        signal[0] *= M_SQRT2;
      else
        signal[0] /= M_SQRT2;
      return(0);
    }
  
  if ((signal2 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Error calling QccVectorAlloc()");
      goto Error;
    }
  if (QccVectorCopy(signal2, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (signal_length % 2)
    if (phase == QCCWAVFILTERBANK_PHASE_ODD)
      lowpass_band_length = signal_length / 2;
    else
      lowpass_band_length = signal_length / 2 + 1;
  else
    lowpass_band_length = signal_length / 2;

  if (phase == QCCWAVFILTERBANK_PHASE_ODD)
    {
      lowpass_subsampling = QCCFILTER_SUBSAMPLEODD;
      highpass_subsampling =
        (filter_bank->orthogonality == QCCWAVFILTERBANK_BIORTHOGONAL) ?
        QCCFILTER_SUBSAMPLEEVEN :
        QCCFILTER_SUBSAMPLEODD;
    }
  else
    {
      lowpass_subsampling = QCCFILTER_SUBSAMPLEEVEN;
      highpass_subsampling =
        (filter_bank->orthogonality == QCCWAVFILTERBANK_BIORTHOGONAL) ?
        QCCFILTER_SUBSAMPLEODD :
        QCCFILTER_SUBSAMPLEEVEN;
    }

  highpass_band_length = signal_length - lowpass_band_length;

  lowpass_band = signal;
  highpass_band = &(signal[lowpass_band_length]);
  
  if (QccFilterMultiRateFilterVector(signal2,
                                     signal_length,
                                     lowpass_band,
                                     lowpass_band_length,
                                     &filter_bank->lowpass_analysis_filter,
                                     QCCFILTER_SAMESAMPLING,
                                     lowpass_subsampling,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  if (QccFilterMultiRateFilterVector(signal2,
                                     signal_length,
                                     highpass_band,
                                     highpass_band_length,
                                     &filter_bank->highpass_analysis_filter,
                                     QCCFILTER_SAMESAMPLING,
                                     highpass_subsampling,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankAnalysis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(signal2);
  return(return_value);
}


int QccWAVFilterBankSynthesis(QccVector signal,
                              int signal_length,
                              int phase,
                              const QccWAVFilterBank *filter_bank,
                              int boundary_extension)
{
  int return_value = 0;
  QccVector lowpass_band;
  int lowpass_band_length;
  int lowpass_upsampling;
  QccVector highpass_band;
  int highpass_band_length;
  int highpass_upsampling;
  QccVector temp_vector = NULL;
  QccVector signal2 = NULL;
  
  if (signal == NULL)
    return(0);
  if (filter_bank == NULL)
    return(0);
  
  if (!signal_length)
    return(0);
  
  if (boundary_extension == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET)
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Boundary wavelets not supported for filter banks");
      goto Error;
    }

  if ((filter_bank->orthogonality == QCCWAVFILTERBANK_ORTHOGONAL) &&
      (boundary_extension != QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION))
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Orthogonal filter banks must use periodic boundary extension");
      goto Error;
    }
  
  if ((boundary_extension == QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION) &&
      (signal_length % 2))
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Signal length must be even for periodic extension");
      goto Error;
    }

  if (signal_length == 1)
    {
      if (phase == QCCWAVFILTERBANK_PHASE_EVEN)
        signal[0] /= M_SQRT2;
      else
        signal[0] *= M_SQRT2;
      return(0);
    }

  if ((signal2 = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }
  if (QccVectorCopy(signal2, signal, signal_length))
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Error calling QccVectorCopy()");
      goto Error;
    }

  if (signal_length % 2)
    if (phase == QCCWAVFILTERBANK_PHASE_ODD)
      lowpass_band_length = signal_length / 2;
    else
      lowpass_band_length = signal_length / 2 + 1;
  else
    lowpass_band_length = signal_length / 2;

  highpass_band_length = signal_length - lowpass_band_length;

  if (phase == QCCWAVFILTERBANK_PHASE_ODD)
    {
      lowpass_upsampling = QCCFILTER_UPSAMPLEODD;
      highpass_upsampling =
        (filter_bank->orthogonality == QCCWAVFILTERBANK_BIORTHOGONAL) ?
        QCCFILTER_UPSAMPLEEVEN :
        QCCFILTER_UPSAMPLEODD;
    }
  else
    {
      lowpass_upsampling = QCCFILTER_UPSAMPLEEVEN;
      highpass_upsampling =
        (filter_bank->orthogonality == QCCWAVFILTERBANK_BIORTHOGONAL) ?
        QCCFILTER_UPSAMPLEODD :
        QCCFILTER_UPSAMPLEEVEN;
    }

  lowpass_band = signal2;
  highpass_band = &(signal2[lowpass_band_length]);
  
  if ((temp_vector = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  if (QccFilterMultiRateFilterVector(lowpass_band,
                                     lowpass_band_length,
                                     temp_vector,
                                     signal_length,
                                     &filter_bank->lowpass_synthesis_filter,
                                     lowpass_upsampling,
                                     QCCFILTER_SAMESAMPLING,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  if (QccFilterMultiRateFilterVector(highpass_band,
                                     highpass_band_length,
                                     signal,
                                     signal_length,
                                     &filter_bank->highpass_synthesis_filter,
                                     highpass_upsampling,
                                     QCCFILTER_SAMESAMPLING,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  if (QccVectorAdd(signal,
                   temp_vector,
                   signal_length))
    {
      QccErrorAddMessage("(QccWAVFilterBankSynthesis): Error calling QccVectorAdd()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(signal2);
  QccVectorFree(temp_vector);
  return(return_value);
}


int QccWAVFilterBankRedundantAnalysis(const QccVector input_signal,
                                      QccVector output_signal_low,
                                      QccVector output_signal_high,
                                      int signal_length,
                                      const QccWAVFilterBank *filter_bank,
                                      int boundary_extension)
{
  int return_value = 0;

  if (input_signal == NULL)
    return(0);
  if (output_signal_low == NULL)
    return(0);
  if (output_signal_high == NULL)
    return(0);
  if (filter_bank == NULL)
    return(0);
  
  if (!signal_length)
    return(0);

  if (signal_length == 1)
    {
      output_signal_low[0] = input_signal[0] * M_SQRT2;
      output_signal_high[0] = input_signal[0] / M_SQRT2;
      return(0);
    }
  
  if (boundary_extension == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET)
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantAnalysis): Boundary wavelets not supported for filter banks");
      goto Error;
    }

  if ((filter_bank->orthogonality == QCCWAVFILTERBANK_ORTHOGONAL) &&
      (boundary_extension != QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantAnalysis): Orthogonal filter banks must use periodic boundary extension");
      goto Error;
    }
  
  if ((boundary_extension == QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION) &&
      (signal_length % 2))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantAnalysis): Signal length must be even for periodic extension");
      goto Error;
    }

  if (QccFilterMultiRateFilterVector(input_signal,
                                     signal_length,
                                     output_signal_low,
                                     signal_length,
                                     &filter_bank->lowpass_analysis_filter,
                                     QCCFILTER_SAMESAMPLING,
                                     QCCFILTER_SAMESAMPLING,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantAnalysis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  if (QccFilterMultiRateFilterVector(input_signal,
                                     signal_length,
                                     output_signal_high,
                                     signal_length,
                                     &filter_bank->highpass_analysis_filter,
                                     QCCFILTER_SAMESAMPLING,
                                     QCCFILTER_SAMESAMPLING,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantAnalysis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVFilterBankRedundantSynthesis(const QccVector input_signal_low,
                                       const QccVector input_signal_high,
                                       QccVector output_signal,
                                       int signal_length,
                                       const QccWAVFilterBank *filter_bank,
                                       int boundary_extension)
{
  int return_value = 0;
  QccVector temp_signal = NULL;

  if (input_signal_low == NULL)
    return(0);
  if (input_signal_high == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (filter_bank == NULL)
    return(0);
  
  if (!signal_length)
    return(0);

  if (signal_length == 1)
    {
      output_signal[0] =
        (input_signal_low[0] / M_SQRT2 + input_signal_high[0] * M_SQRT2) / 2;
      return(0);
    }
  
  if ((temp_signal = QccVectorAlloc(signal_length)) == NULL)
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Error calling QccVectorAlloc()");
      goto Error;
    }

  if (boundary_extension == QCCWAVWAVELET_BOUNDARY_BOUNDARY_WAVELET)
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Boundary wavelets not supported for filter banks");
      goto Error;
    }

  if ((filter_bank->orthogonality == QCCWAVFILTERBANK_ORTHOGONAL) &&
      (boundary_extension != QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Orthogonal filter banks must use periodic boundary extension");
      goto Error;
    }
  
  if ((boundary_extension == QCCWAVWAVELET_BOUNDARY_PERIODIC_EXTENSION) &&
      (signal_length % 2))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Signal length must be even for periodic extension");
      goto Error;
    }

  if (QccFilterMultiRateFilterVector(input_signal_low,
                                     signal_length,
                                     output_signal,
                                     signal_length,
                                     &filter_bank->lowpass_synthesis_filter,
                                     QCCFILTER_SAMESAMPLING,
                                     QCCFILTER_SAMESAMPLING,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  if (QccFilterMultiRateFilterVector(input_signal_high,
                                     signal_length,
                                     temp_signal,
                                     signal_length,
                                     &filter_bank->highpass_synthesis_filter,
                                     QCCFILTER_SAMESAMPLING,
                                     QCCFILTER_SAMESAMPLING,
                                     boundary_extension))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Error calling QccFilterMultiRateFilterVector()");
      goto Error;
    }
  
  if (QccVectorAdd(output_signal,
                   temp_signal,
                   signal_length))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Error calling QccVectorAdd()");
      goto Error;
    }

  if (QccVectorScalarMult(output_signal,
                          0.5,
                          signal_length))
    {
      QccErrorAddMessage("(QccWAVFilterBankRedundantSynthesis): Error calling QccVectorScalarMult()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(temp_signal);
  return(return_value);
}

