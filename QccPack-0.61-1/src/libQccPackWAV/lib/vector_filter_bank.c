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


int QccWAVVectorFilterBankInitialize(QccWAVVectorFilterBank
                                     *vector_filter_bank)
{
  if (vector_filter_bank == NULL)
    return(0);
  
  QccStringMakeNull(vector_filter_bank->filename);

  QccStringCopy(vector_filter_bank->magic_num,
                QCCWAVVECTORFILTERBANK_MAGICNUM);
  QccGetQccPackVersion(&vector_filter_bank->major_version,
                       &vector_filter_bank->minor_version,
                       NULL);
  
  QccWAVVectorFilterInitialize(&vector_filter_bank->lowpass_analysis_filter);
  QccWAVVectorFilterInitialize(&vector_filter_bank->highpass_analysis_filter);
  QccWAVVectorFilterInitialize(&vector_filter_bank->lowpass_synthesis_filter);
  QccWAVVectorFilterInitialize(&vector_filter_bank->highpass_synthesis_filter);

  return(0);
}


int QccWAVVectorFilterBankAlloc(QccWAVVectorFilterBank *vector_filter_bank)
{
  int return_value;
  
  if (vector_filter_bank == NULL)
    return(0);
  
  if (QccWAVVectorFilterAlloc(&vector_filter_bank->lowpass_analysis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAlloc): Error calling QccWAVVectorFilterAlloc()");
      goto Error;
    }
  if (QccWAVVectorFilterAlloc(&vector_filter_bank->highpass_analysis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAlloc): Error calling QccWAVVectorFilterAlloc()");
      goto Error;
    }
  if (QccWAVVectorFilterAlloc(&vector_filter_bank->lowpass_synthesis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAlloc): Error calling QccWAVVectorFilterAlloc()");
      goto Error;
    }
  if (QccWAVVectorFilterAlloc(&vector_filter_bank->highpass_synthesis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAlloc): Error calling QccWAVVectorFilterAlloc()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


void QccWAVVectorFilterBankFree(QccWAVVectorFilterBank *vector_filter_bank)
{
  if (vector_filter_bank == NULL)
    return;
  
  QccWAVVectorFilterFree(&vector_filter_bank->lowpass_analysis_filter);
  QccWAVVectorFilterFree(&vector_filter_bank->highpass_analysis_filter);
  QccWAVVectorFilterFree(&vector_filter_bank->lowpass_synthesis_filter);
  QccWAVVectorFilterFree(&vector_filter_bank->highpass_synthesis_filter);
  
}


int QccWAVVectorFilterBankMakeOrthogonal(QccWAVVectorFilterBank
                                         *vector_filter_bank,
                                         const QccWAVVectorFilter
                                         *lowpass_vector_filter,
                                         const QccWAVVectorFilter
                                         *highpass_vector_filter)
{
  if (vector_filter_bank == NULL)
    return(0);
  if (lowpass_vector_filter == NULL)
    return(0);
  if (highpass_vector_filter == NULL)
    return(0);
  
  if (lowpass_vector_filter->causality != QCCWAVVECTORFILTER_CAUSAL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error lowpass filter must be causal");
      return(1);
    }
  if (highpass_vector_filter->causality != QCCWAVVECTORFILTER_CAUSAL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error highpass filter must be causal");
      return(1);
    }
  
  vector_filter_bank->lowpass_analysis_filter.length =
    vector_filter_bank->highpass_analysis_filter.length =
    vector_filter_bank->lowpass_synthesis_filter.length =
    vector_filter_bank->highpass_synthesis_filter.length =
    lowpass_vector_filter->length;
  
  vector_filter_bank->lowpass_analysis_filter.dimension =
    vector_filter_bank->highpass_analysis_filter.dimension =
    vector_filter_bank->lowpass_synthesis_filter.dimension =
    vector_filter_bank->highpass_synthesis_filter.dimension =
    lowpass_vector_filter->dimension;
  
  if (QccWAVVectorFilterBankAlloc(vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error calling QccWAVVectorFilterBankAlloc()");
      return(1);
    }
  
  /*   H^T[n]  */
  if (QccWAVVectorFilterCopy(&vector_filter_bank->lowpass_synthesis_filter,
                             lowpass_vector_filter,
                             1))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error calling QccWAVVectorFilterCopy()");
      return(1);
    }
  
  /*  H[-n]  */
  if (QccWAVVectorFilterReversal(lowpass_vector_filter,
                                 &(vector_filter_bank->lowpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error calling QccWAVVectorFilterReversal()");
      return(1);
    }
  
  /*  G^T[n] */
  if (QccWAVVectorFilterCopy(&(vector_filter_bank->highpass_synthesis_filter),
                             highpass_vector_filter,
                             1))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error calling QccWAVVectorFilterCopy()");
      return(1);
    }
  
  /*  G[-n]  */
  if (QccWAVVectorFilterReversal(highpass_vector_filter,
                                 &(vector_filter_bank->highpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeOrthogonal): Error calling QccWAVVectorFilterReversal()");
      return(1);
    }
  
  return(0);
}


int QccWAVVectorFilterBankMakeBiorthogonal(QccWAVVectorFilterBank
                                           *vector_filter_bank,
                                           const QccWAVVectorFilter
                                           *primary_lowpass_vector_filter,
                                           const QccWAVVectorFilter
                                           *primary_highpass_vector_filter,
                                           const QccWAVVectorFilter
                                           *dual_lowpass_vector_filter,
                                           const QccWAVVectorFilter
                                           *dual_highpass_vector_filter)
{
  if (vector_filter_bank == NULL)
    return(0);
  if (primary_lowpass_vector_filter == NULL)
    return(0);
  if (primary_highpass_vector_filter == NULL)
    return(0);
  if (dual_lowpass_vector_filter == NULL)
    return(0);
  if (dual_highpass_vector_filter == NULL)
    return(0);
  
  vector_filter_bank->lowpass_analysis_filter.length =
    dual_lowpass_vector_filter->length;
  vector_filter_bank->highpass_analysis_filter.length =
    dual_highpass_vector_filter->length;
  vector_filter_bank->lowpass_synthesis_filter.length =
    primary_lowpass_vector_filter->length;
  vector_filter_bank->highpass_synthesis_filter.length =
    primary_highpass_vector_filter->length;

  vector_filter_bank->lowpass_analysis_filter.dimension =
    dual_lowpass_vector_filter->dimension;
  vector_filter_bank->highpass_analysis_filter.dimension =
    dual_highpass_vector_filter->dimension;
  vector_filter_bank->lowpass_synthesis_filter.dimension =
    primary_lowpass_vector_filter->dimension;
  vector_filter_bank->highpass_synthesis_filter.dimension =
    primary_highpass_vector_filter->dimension;
  
  if (QccWAVVectorFilterBankAlloc(vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeBiorthogonal): Error calling QccWAVVectorFilterBankAlloc()");
      return(1);
    }
  
  /*  ~H[-n]  */
  if (QccWAVVectorFilterReversal(dual_lowpass_vector_filter,
                                 &(vector_filter_bank->lowpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeBiorthogonal): Error calling QccWAVVectorFilterReversal()");
      return(1);
    }

  /*   H^T[n]  */
  if (QccWAVVectorFilterCopy(&vector_filter_bank->lowpass_synthesis_filter,
                             primary_lowpass_vector_filter,
                             1))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeBiorthogonal): Error calling QccWAVVectorFilterCopy()");
      return(1);
    }

  /*  ~G[-n]  */
  if (QccWAVVectorFilterReversal(dual_highpass_vector_filter,
                                 &(vector_filter_bank->highpass_analysis_filter)))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeBiorthogonal): Error calling QccWAVVectorFilterReversal()");
      return(1);
    }

  /*   G^T[n]  */
  if (QccWAVVectorFilterCopy(&vector_filter_bank->highpass_synthesis_filter,
                             primary_highpass_vector_filter,
                             1))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankMakeBiorthogonal): Error calling QccWAVVectorFilterCopy()");
      return(1);
    }

  return(0);
}


static int QccWAVVectorFilterBankReadHeader(FILE *infile,
                                            QccWAVVectorFilterBank
                                            *vector_filter_bank)
{
  
  if ((infile == NULL) || (vector_filter_bank == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             vector_filter_bank->magic_num, 
                             &vector_filter_bank->major_version,
                             &vector_filter_bank->minor_version))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankReadHeader): Error reading magic number in filter bank %s",
                         vector_filter_bank->filename);
      return(1);
    }
  
  if (strcmp(vector_filter_bank->magic_num, QCCWAVVECTORFILTERBANK_MAGICNUM))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankReadHeader): %s is not of filter-bank (%s) type",
                         vector_filter_bank->filename,
                         QCCWAVVECTORFILTERBANK_MAGICNUM);
      return(1);
    }
  
  if (QccCompareQccPackVersions(vector_filter_bank->major_version,
                                vector_filter_bank->minor_version,
                                0, 17) < 0)
    vector_filter_bank->orthogonality =
      QCCWAVVECTORFILTERBANK_ORTHOGONAL;
  else
    {
      fscanf(infile, "%d", &vector_filter_bank->orthogonality);
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadHeader): Error reading orthogonality of vector filter bank");
          return(1);
        }
    }

  return(0);
}


static int QccWAVVectorFilterBankReadData(FILE *infile,
                                          QccWAVVectorFilterBank
                                          *vector_filter_bank)
{
  int return_value;
  QccWAVVectorFilter primary_lowpass_vector_filter;
  QccWAVVectorFilter primary_highpass_vector_filter;
  QccWAVVectorFilter dual_lowpass_vector_filter;
  QccWAVVectorFilter dual_highpass_vector_filter;
  
  if (infile == NULL)
    return(0);
  if (vector_filter_bank == NULL)
    return(0);
  
  QccWAVVectorFilterInitialize(&primary_lowpass_vector_filter);
  QccWAVVectorFilterInitialize(&primary_highpass_vector_filter);
  QccWAVVectorFilterInitialize(&dual_lowpass_vector_filter);
  QccWAVVectorFilterInitialize(&dual_highpass_vector_filter);
  
  switch (vector_filter_bank->orthogonality)
    {
    case QCCWAVVECTORFILTERBANK_ORTHOGONAL:
      if (QccWAVVectorFilterRead(infile,
                                 &primary_lowpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterRead()");
          goto Error;
        }
      if (QccWAVVectorFilterRead(infile,
                                 &primary_highpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterRead()");
          goto Error;
        }
      
      if (QccWAVVectorFilterBankMakeOrthogonal(vector_filter_bank,
                                               &primary_lowpass_vector_filter,
                                               &primary_highpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterBankMakeOrthogonal()");
          goto Error;
        }
      break;

    case QCCWAVVECTORFILTERBANK_BIORTHOGONAL:
      if (QccWAVVectorFilterRead(infile,
                                 &primary_lowpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterRead()");
          goto Error;
        }
      if (QccWAVVectorFilterRead(infile,
                                 &primary_highpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterRead()");
          goto Error;
        }
      if (QccWAVVectorFilterRead(infile,
                                 &dual_lowpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterRead()");
          goto Error;
        }
      if (QccWAVVectorFilterRead(infile,
                                 &dual_highpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterRead()");
          goto Error;
        }

      if (QccWAVVectorFilterBankMakeBiorthogonal(vector_filter_bank,
                                                 &primary_lowpass_vector_filter,
                                                 &primary_highpass_vector_filter,
                                                 &dual_lowpass_vector_filter,
                                                 &dual_highpass_vector_filter))
        {
          QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Error calling QccWAVVectorFilterBankMakeBiorthogonal()");
          goto Error;
        }
      break;

    default:
      QccErrorAddMessage("(QccWAVVectorFilterBankReadData): Undefined orthogonality value in vector filter bank");
      goto Error;
    }
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccWAVVectorFilterFree(&primary_lowpass_vector_filter);
  QccWAVVectorFilterFree(&primary_highpass_vector_filter);
  QccWAVVectorFilterFree(&dual_lowpass_vector_filter);
  QccWAVVectorFilterFree(&dual_highpass_vector_filter);
  return(return_value);
}


int QccWAVVectorFilterBankRead(QccWAVVectorFilterBank *vector_filter_bank)
{
  FILE *infile = NULL;

  if (vector_filter_bank == NULL)
    return(0);
  
  if ((infile = 
       QccFileOpen(vector_filter_bank->filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankRead): Error calling QccFileOpen()");
      return(1);
    }

  if (QccWAVVectorFilterBankReadHeader(infile, vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankRead): Error calling QccWAVVectorFilterBankReadHeader()");
      return(1);
    }
  
  if (QccWAVVectorFilterBankReadData(infile, vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankRead): Error calling QccWAVVectorFilterBankReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccWAVVectorFilterBankWriteHeader(FILE *outfile,
                                             const QccWAVVectorFilterBank
                                             *vector_filter_bank)
{
  if ((outfile == NULL) || (vector_filter_bank == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCWAVVECTORFILTERBANK_MAGICNUM))
    goto Error;
  
  fprintf(outfile, "%d\n", vector_filter_bank->orthogonality);
  if (ferror(outfile))
    goto Error;

  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVVectorFilterBankWriteHeader): Error writing header to %s",
                     vector_filter_bank->filename);
  return(1);
  
}


static int QccWAVVectorFilterBankWriteData(FILE *outfile,
                                           const QccWAVVectorFilterBank
                                           *vector_filter_bank)
{
  if (outfile == NULL)
    return(0);
  if (vector_filter_bank == NULL)
    return(0);
  
  if (QccWAVVectorFilterWrite(outfile,
                              &(vector_filter_bank->lowpass_synthesis_filter)))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankWriteData): Error calling QccWAVVectorFilterWrite()");
      return(1);
    }

  if (QccWAVVectorFilterWrite(outfile,
                              &(vector_filter_bank->highpass_synthesis_filter)))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankWriteData): Error calling QccWAVVectorFilterWrite()");
      return(1);
    }

  return(0);
}


int QccWAVVectorFilterBankWrite(const QccWAVVectorFilterBank
                                *vector_filter_bank)
{
  FILE *outfile;
  
  if (vector_filter_bank == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(vector_filter_bank->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccWAVVectorFilterBankWriteHeader(outfile, vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankWrite): Error calling QccWAVVectorFilterBankWriteHeader()");
      return(1);
    }
  
  if (QccWAVVectorFilterBankWriteData(outfile, vector_filter_bank))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankWrite): Error calling QccWAVVectorFilterBankWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccWAVVectorFilterBankPrint(const QccWAVVectorFilterBank
                                *vector_filter_bank)
{
  if (vector_filter_bank == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(vector_filter_bank->filename,
                           vector_filter_bank->magic_num,
                           vector_filter_bank->major_version,
                           vector_filter_bank->minor_version))
    return(1);
  
  
  switch (vector_filter_bank->orthogonality)
    {
    case QCCWAVVECTORFILTERBANK_ORTHOGONAL:
      printf("  Orthogonal vector filter bank\n\n");
      break;
    case QCCWAVVECTORFILTERBANK_BIORTHOGONAL:
      printf("  Biorthogonal vector filter bank\n\n");
      break;
    default:
      break;
    }
  
  printf("---------------- Lowpass Analysis Filter ----------------\n");
  if (QccWAVVectorFilterPrint(&vector_filter_bank->lowpass_analysis_filter))
    goto Error;
  
  printf("---------------- Highpass Analysis Filter ----------------\n");
  if (QccWAVVectorFilterPrint(&vector_filter_bank->highpass_analysis_filter))
    goto Error;
  
  printf("---------------- Lowpass Synthesis Filter ----------------\n");
  if (QccWAVVectorFilterPrint(&vector_filter_bank->lowpass_synthesis_filter))
    goto Error;
  
  printf("---------------- Highpass Synthesis Filter ----------------\n");
  if (QccWAVVectorFilterPrint(&vector_filter_bank->highpass_synthesis_filter))
    goto Error;
  
  return(0);
  
 Error:
  QccErrorAddMessage("(QccWAVVectorFilterBankPrint): Error calling QccWAVVectorFilterPrint()");
  return(1);
}


static int QccWAVVectorFilterBankAnalysisFilter(const QccVector *input_signal,
                                                QccVector *output_signal,
                                                int signal_length,
                                                int vector_dimension,
                                                const QccWAVVectorFilter
                                                *filter)
{
  int return_value;
  int filter_index;
  int input_index;
  int output_index;
  QccVector tmp_vector = NULL;

  if ((tmp_vector = QccVectorAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysisFilter): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  switch (filter->causality)
    {
    case QCCWAVVECTORFILTER_CAUSAL:
      for (output_index = 0; output_index < signal_length / 2; output_index++)
        {
          QccVectorZero(output_signal[output_index],
                        vector_dimension);
          for (filter_index = 0;
               filter_index < filter->length; filter_index++)
            {
              input_index =
                QccMathModulus(2 * output_index - filter_index, signal_length);
              
              if (QccMatrixVectorMultiply(filter->coefficients[filter_index],
                                          input_signal[input_index],
                                          tmp_vector,
                                          vector_dimension,
                                          vector_dimension))
                {
                  QccErrorAddMessage("(QccWAVVectorFilterBankAnalysisFilter): Error calling QccMatrixVectorMultiply()");
                  goto Error;
                }
              
              QccVectorAdd(output_signal[output_index],
                           tmp_vector,
                           vector_dimension);
            }
        }
      break;

    case QCCWAVVECTORFILTER_ANTICAUSAL:
      for (output_index = 0; output_index < signal_length / 2; output_index++)
        {
          QccVectorZero(output_signal[output_index],
                        vector_dimension);
          for (filter_index = 0;
               filter_index < filter->length; filter_index++)
            {
              input_index =
                QccMathModulus(2 * output_index + filter_index, signal_length);
              
              if (QccMatrixVectorMultiply(filter->coefficients
                                          [filter->length - 1 - filter_index],
                                          input_signal[input_index],
                                          tmp_vector,
                                          vector_dimension,
                                          vector_dimension))
                {
                  QccErrorAddMessage("(QccWAVVectorFilterBankAnalysisFilter): Error calling QccMatrixVectorMultiply()");
                  goto Error;
                }
              
              QccVectorAdd(output_signal[output_index],
                           tmp_vector,
                           vector_dimension);
            }
        }
      break;

    }      
      
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(tmp_vector);
  return(return_value);
}


static int QccWAVVectorFilterBankSynthesisFilter(const QccVector *input_signal,
                                                 QccVector *output_signal,
                                                 int signal_length,
                                                 int vector_dimension,
                                                 const QccWAVVectorFilter
                                                 *filter)
{
  int return_value;
  int filter_index;
  int input_index;
  int output_index;
  QccVector tmp_vector = NULL;

  if ((tmp_vector = QccVectorAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysisFilter): Error calling QccVectorAlloc()");
      goto Error;
    }
  
  switch (filter->causality)
    {
    case QCCWAVVECTORFILTER_CAUSAL:
      for (output_index = 0; output_index < signal_length; output_index++)
        for (filter_index = 0;
             filter_index < filter->length; filter_index++)
          {
            input_index =
              QccMathModulus(output_index - filter_index, signal_length);
            
            if (!(input_index % 2))
              {
                if (QccMatrixVectorMultiply(filter->coefficients[filter_index],
                                            input_signal[input_index / 2],
                                            tmp_vector,
                                            vector_dimension,
                                            vector_dimension))
                  {
                    QccErrorAddMessage("(QccWAVVectorFilterBankAnalysisFilter): Error calling QccMatrixVectorMultiply()");
                    goto Error;
                  }
                
                QccVectorAdd(output_signal[output_index],
                             tmp_vector,
                             vector_dimension);
              }
          }
      break;

    case QCCWAVVECTORFILTER_ANTICAUSAL:
      for (output_index = 0; output_index < signal_length; output_index++)
        for (filter_index = 0;
             filter_index < filter->length; filter_index++)
          {
            input_index =
              QccMathModulus(output_index + filter_index, signal_length);
            
            if (!(input_index %2))
              {
                if (QccMatrixVectorMultiply(filter->coefficients
                                            [filter->length - 1 -
                                            filter_index],
                                            input_signal[input_index / 2],
                                            tmp_vector,
                                            vector_dimension,
                                            vector_dimension))
                  {
                    QccErrorAddMessage("(QccWAVVectorFilterBankAnalysisFilter): Error calling QccMatrixVectorMultiply()");
                    goto Error;
                  }
                
                QccVectorAdd(output_signal[output_index],
                             tmp_vector,
                             vector_dimension);
              }
          }
      break;

    }      
      
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccVectorFree(tmp_vector);
  return(return_value);
}


int QccWAVVectorFilterBankAnalysis(const QccDataset *input_signal,
                                   QccDataset *output_signal,
                                   const QccWAVVectorFilterBank
                                   *vector_filter_bank)
{
  int signal_length;
  int vector_dimension;
  int return_value = 0;

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (vector_filter_bank == NULL)
    return(0);
  
  signal_length = input_signal->num_vectors;
  vector_dimension = input_signal->vector_dimension;

  if ((vector_filter_bank->lowpass_analysis_filter.dimension !=
       vector_dimension) ||
      (vector_filter_bank->highpass_analysis_filter.dimension !=
       vector_dimension) ||
      (vector_filter_bank->lowpass_synthesis_filter.dimension !=
       vector_dimension) ||
      (vector_filter_bank->highpass_synthesis_filter.dimension !=
       vector_dimension))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysis): Vector filter bank filters must have same vector dimension as input signal");
      goto Error;
    }

  if ((signal_length != output_signal->num_vectors) ||
      (vector_dimension != output_signal->vector_dimension))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysis): Input and output signals must have same length and same vector dimension");
      goto Error;
    }

  if (signal_length < 2)
    {
      QccDatasetCopy(output_signal, input_signal);
      goto Return;
    }
  
  if (signal_length % 2)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysis): Signal length must be even");
      goto Error;
    }

  if (QccWAVVectorFilterBankAnalysisFilter(input_signal->vectors,
                                           &output_signal->vectors[0],
                                           signal_length,
                                           vector_dimension,
                                           &vector_filter_bank->lowpass_analysis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysis): Error calling QcCWAVVectorFilterBankAnalysisFilter()");
      goto Error;
    }

  if (QccWAVVectorFilterBankAnalysisFilter(input_signal->vectors,
                                           &output_signal->vectors
                                           [signal_length / 2],
                                           signal_length,
                                           vector_dimension,
                                           &vector_filter_bank->highpass_analysis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysis): Error calling QccWAVVectorFilterBankAnalysisFilter()");
      goto Error;
    }

  if (QccDatasetSetMaxMinValues(output_signal))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankAnalysis): Error calling QccDatasetSetMaxMinValues()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccWAVVectorFilterBankSynthesis(const QccDataset *input_signal,
                                    QccDataset *output_signal,
                                    const QccWAVVectorFilterBank
                                    *vector_filter_bank)
{
  int signal_length;
  int vector_dimension;
  int return_value = 0;
  int output_index;

  if (input_signal == NULL)
    return(0);
  if (output_signal == NULL)
    return(0);
  if (vector_filter_bank == NULL)
    return(0);
  
  signal_length = input_signal->num_vectors;
  vector_dimension = input_signal->vector_dimension;

  if ((vector_filter_bank->lowpass_analysis_filter.dimension !=
       vector_dimension) ||
      (vector_filter_bank->highpass_analysis_filter.dimension !=
       vector_dimension) ||
      (vector_filter_bank->lowpass_synthesis_filter.dimension !=
       vector_dimension) ||
      (vector_filter_bank->highpass_synthesis_filter.dimension !=
       vector_dimension))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankSynthesis): Vector filter bank filters must have same vector dimension as input signal");
      goto Error;
    }

  if ((signal_length != output_signal->num_vectors) ||
      (vector_dimension != output_signal->vector_dimension))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankSynthesis): Input and output signals must have same length and same vector dimension");
      goto Error;
    }

  if (signal_length < 2)
    {
      QccDatasetCopy(output_signal, input_signal);
      goto Return;
    }
  
  if (signal_length % 2)
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankSynthesis): Signal length must be even");
      goto Error;
    }

  for (output_index = 0; output_index < signal_length; output_index++)
    QccVectorZero(output_signal->vectors[output_index], vector_dimension);

  if (QccWAVVectorFilterBankSynthesisFilter(&input_signal->vectors[0],
                                            output_signal->vectors,
                                            signal_length,
                                            vector_dimension,
                                            &vector_filter_bank->lowpass_synthesis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankSynthesis): Error calling QccWAVVectorFilterBankSynthesisFilter()");
      goto Error;
    }

  if (QccWAVVectorFilterBankSynthesisFilter(&input_signal->vectors
                                            [signal_length / 2],
                                            output_signal->vectors,
                                            signal_length,
                                            vector_dimension,
                                            &vector_filter_bank->highpass_synthesis_filter))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankSynthesis): Error calling QccWAVVectorFilterBankSynthesisFilter()");
      goto Error;
    }

  if (QccDatasetSetMaxMinValues(output_signal))
    {
      QccErrorAddMessage("(QccWAVVectorFilterBankSynthesis): Error calling QccDatasetSetMaxMinValues()");
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


