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



static int QccAVQPaulSendSideInfo(QccAVQSideInfo *sideinfo, 
                                  QccAVQSideInfoSymbol *updatevector_symbol,
                                  int update_flag)
{
  QccAVQSideInfoSymbol flag_symbol;
  
  flag_symbol.type = QCCAVQSIDEINFO_FLAG;
  flag_symbol.flag = update_flag;
  flag_symbol.vector = NULL;
  flag_symbol.vector_indices = NULL;

  if (QccAVQSideInfoSymbolWrite(sideinfo, &flag_symbol))
    {
      QccErrorAddMessage("(QccAVQPaulSendSideInfo): Error calling QccAVQSideInfoSymbolWrite()");
      return(1);
    }
  
  if (update_flag == QCCAVQSIDEINFO_FLAG_UPDATE)
    {
      updatevector_symbol->type = QCCAVQSIDEINFO_VECTOR;
      if (QccAVQSideInfoSymbolWrite(sideinfo, updatevector_symbol))
        {
          QccErrorAddMessage("(QccAVQPaulSendSideInfo): Error calling QccAVQSideInfoSymbolWrite()");
          return(1);
        }
    }
  
  return(0);
}


int QccAVQPaulEncode(const QccDataset *dataset,
                     QccVQCodebook *codebook,
                     QccChannel *channel,
                     QccAVQSideInfo *sideinfo,
                     double distortion_threshold,
                     QccVQDistortionMeasure distortion_measure)
{
  int return_value;
  int block_size;
  int vector_dimension;
  int current_vector;
  double winning_distortion;
  int winner;
  QccDataset tmp_dataset;
  int codebook_size;
  QccAVQSideInfoSymbol *new_codeword_symbol = NULL;
  
  if ((dataset == NULL) || (codebook == NULL) || (channel == NULL) ||
      (sideinfo == NULL))
    return(0);
  if ((dataset->vectors == NULL) || 
      (codebook->codewords == NULL))
    return(0);
  
  vector_dimension = dataset->vector_dimension;
  if ((vector_dimension != codebook->codeword_dimension) ||
      (vector_dimension != sideinfo->vector_dimension))
    {
      QccErrorAddMessage("(QccAVQPaulEncode): Codebook %s, dataset %s, and sideinfo %s",
                         codebook->filename, dataset->filename, sideinfo->filename);
      QccErrorAddMessage("have different vector dimensions");
      goto QccAVQError;
    }
  
  block_size = QccDatasetGetBlockSize(dataset);
  
  QccDatasetInitialize(&tmp_dataset);
  tmp_dataset.num_vectors = 1;
  tmp_dataset.vector_dimension = dataset->vector_dimension;
  tmp_dataset.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  tmp_dataset.num_blocks_accessed = 0;
  if (QccDatasetAlloc(&tmp_dataset))
    {
      QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccDatasetAlloc()");
      goto QccAVQError;
    }
  
  if ((new_codeword_symbol = 
       QccAVQSideInfoSymbolAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccAVQSideInfoSymbolAlloc()");
      goto QccAVQError;
    }
  new_codeword_symbol->type = QCCAVQSIDEINFO_VECTOR;

  codebook_size = codebook->num_codewords;
  
  for (current_vector = 0; current_vector < block_size; current_vector++)
    {
      QccVectorCopy(tmp_dataset.vectors[0],
                    dataset->vectors[current_vector],
                    vector_dimension);
      
      if (QccVQVectorQuantization(&tmp_dataset, codebook, 
                                  (QccVector)&winning_distortion, &winner,
                                  distortion_measure))
        {
          QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccVQVectorQuantization()");
          goto QccAVQError;
        }
      
      QccVectorCopy(new_codeword_symbol->vector,
                    dataset->vectors[current_vector],
                    vector_dimension);
      QccAVQSideInfoCodebookCoder(sideinfo, new_codeword_symbol);

      if (winning_distortion > distortion_threshold)
        {
          /*  Update codebook  */
          QccVectorCopy((QccVector)codebook->codewords[codebook_size - 1], 
                        new_codeword_symbol->vector, vector_dimension);
          if (QccVQCodebookMoveCodewordToFront(codebook, 
                                               codebook_size - 1))
            {
              QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccVQCodebookMoveCodewordToFront()");
              goto QccAVQError;
            }
          
          channel->channel_symbols[current_vector] = QCCCHANNEL_NULLSYMBOL;
          
          if (QccAVQPaulSendSideInfo(sideinfo, new_codeword_symbol, 
                                     QCCAVQSIDEINFO_FLAG_UPDATE))
            {
              QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccAVQPaulSendSideInfo()");
              goto QccAVQError;
            }
        }
      else
        {
          /*  No codebook update  */
          QccVQCodebookMoveCodewordToFront(codebook, winner);

          channel->channel_symbols[current_vector] = winner;
          
          if (QccAVQPaulSendSideInfo(sideinfo, NULL, 
                                     QCCAVQSIDEINFO_FLAG_NOUPDATE))
            {
              QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccAVQPaulSendSideInfo()");
              goto QccAVQError;
            }
        }
    }
  
  return_value = 0;
  goto QccAVQExit;
  
 QccAVQError:
  return_value = 1;

 QccAVQExit:
  QccDatasetFree(&tmp_dataset);
  if (new_codeword_symbol != NULL)
    QccAVQSideInfoSymbolFree(new_codeword_symbol);

  return(return_value);
}


int QccAVQPaulDecode(QccDataset *dataset,
                     QccVQCodebook *codebook,
                     const QccChannel *channel,
                     QccAVQSideInfo *sideinfo)
{
  int return_value;
  int vector_dimension, block_size, codebook_size;
  int current_vector;
  QccAVQSideInfoSymbol *current_symbol = NULL;
  
  if ((dataset == NULL) || (codebook == NULL) || (channel == NULL) ||
      (sideinfo == NULL))
    return(0);
  if (sideinfo->fileptr == NULL)
    return(0);
  if ((dataset->vectors == NULL) || 
      (codebook->codewords == NULL) ||
      (channel->channel_symbols == NULL))
    return(0);
  
  vector_dimension = dataset->vector_dimension;
  if ((vector_dimension != codebook->codeword_dimension) ||
      (vector_dimension != sideinfo->vector_dimension))
    {
      QccErrorAddMessage("(QccAVQPaulEncode): Codebook %s, dataset %s, and sideinfo %s",
                         codebook->filename, dataset->filename, sideinfo->filename);
      QccErrorAddMessage("have different vector dimensions");
      goto QccAVQError;
    }
  
  if ((current_symbol = QccAVQSideInfoSymbolAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQPaulDecode): Error calling QccAVQSideInfoSymbolAlloc()");
      goto QccAVQError;
    }
  
  block_size = QccDatasetGetBlockSize(dataset);
  
  codebook_size = codebook->num_codewords;
  
  for (current_vector = 0; current_vector < block_size; current_vector++)
    {
      if ((current_symbol = 
           QccAVQSideInfoSymbolReadDesired(sideinfo,
                                           QCCAVQSIDEINFO_FLAG,
                                           current_symbol)) == NULL)
        {
          QccErrorAddMessage("(QccAVQPaulDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
          goto QccAVQError;
        }
      
      if (current_symbol->flag == QCCAVQSIDEINFO_FLAG_UPDATE)
        {
          /*  Update codebook  */
          if ((current_symbol = 
               QccAVQSideInfoSymbolReadDesired(sideinfo,
                                               QCCAVQSIDEINFO_VECTOR,
                                               current_symbol)) == NULL)
            {
              QccErrorAddMessage("(QccAVQPaulDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
              goto QccAVQError;
            }
          QccVectorCopy((QccVector)codebook->codewords[codebook_size - 1], 
                        current_symbol->vector, vector_dimension);
          if (QccVQCodebookMoveCodewordToFront(codebook, 
                                               codebook_size - 1))
            {
              QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccVQCodebookMoveCodewordToFront()");
              goto QccAVQError;
            }
        }
      else
        {
          /*  No codebook update  */
          if (QccVQCodebookMoveCodewordToFront(codebook,
                                               channel->channel_symbols[current_vector]))
            {
              QccErrorAddMessage("(QccAVQPaulEncode): Error calling QccVQCodebookMoveCodewordToFront()");
              goto QccAVQError;
            }
        }
      
      QccVectorCopy(dataset->vectors[current_vector],
                    (QccVector)codebook->codewords[0],
                    vector_dimension);
    }
  
  return_value = 0;
  goto QccAVQExit;
  
 QccAVQError:
  return_value = 1;

 QccAVQExit:
  if (current_symbol != NULL)
    QccAVQSideInfoSymbolFree(current_symbol);
  return(return_value);
}


int QccAVQPaulCalcRate(QccChannel *channel, 
                       QccAVQSideInfo *sideinfo,
                       FILE *outfile, 
                       double *rate, 
                       int verbose,
                       int entropy_code_sideinfo)
{
  int return_value;
  int total_updated, num_input_vectors;
  double channel_entropy;
  double num_bits_channel, num_bits_sideinfo, num_bits_total;
  double num_bits_sideinfo_updatevectors, num_bits_sideinfo_flags;
  double total_rate, channel_rate, sideinfo_rate;
  double sideinfo_updatevectors_rate, sideinfo_flags_rate;
  int symbol, component;
  int precision;
  QccAVQSideInfoSymbol *sideinfo_symbol = NULL;
  QccChannel sideinfo_channel;
  double sideinfo_entropy, sideinfo_flags_entropy;
  double prob1, prob2;
  
  if ((outfile == NULL) ||
      (channel == NULL) ||
      (sideinfo == NULL))
    return(0);
  
  if ((sideinfo_symbol = 
       QccAVQSideInfoSymbolAlloc(sideinfo->vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccAVQSideInfoSymbolAlloc()");
      goto Error;
    }

  QccChannelInitialize(&sideinfo_channel);

  sideinfo_channel.channel_length = channel->channel_length *
    sideinfo->vector_dimension;
  sideinfo_channel.alphabet_size = sideinfo->codebook_coder.num_levels;
  sideinfo_channel.access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  if (QccChannelAlloc(&sideinfo_channel))
    {
      QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccChannelAlloc()");
      goto Error;
    }
  sideinfo_channel.channel_length = 0;

  num_input_vectors = sideinfo->N;

  total_updated = 0;

  for (symbol = 0; symbol < num_input_vectors; symbol++)
    {
      if (QccAVQSideInfoSymbolReadNextFlag(sideinfo,
                                           sideinfo_symbol) == NULL)
        {
          QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccAVQSideInfoSymbolReadNextFlag()");
          goto Error;
        }
      if (sideinfo_symbol->flag == QCCAVQSIDEINFO_FLAG_UPDATE)
        {
          if (QccAVQSideInfoSymbolReadDesired(sideinfo,
                                              QCCAVQSIDEINFO_VECTOR,
                                              sideinfo_symbol) == NULL)
            {
              QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccAVQSideInfoSymbolReadDesired()");
              goto Error;
            }
          for (component = 0; component < sideinfo->vector_dimension;
               component++, sideinfo_channel.channel_length++)
            sideinfo_channel.channel_symbols
              [sideinfo_channel.channel_length] = 
              sideinfo_symbol->vector_indices[component];

          total_updated++;
        }
    }

  if ((sideinfo_entropy = QccChannelEntropy(&sideinfo_channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccChannelEntropy()");
      goto Error;
    }

  if (QccChannelRemoveNullSymbols(channel))
    {
      QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccChannelRemoveNullSymbols()");
      goto Error;
    }

  if ((channel_entropy = QccChannelEntropy(channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQPaulCalcRate): Error calling QccChannelEntropy()");
      goto Error;
    }
  
  prob1 = (double)total_updated/num_input_vectors;
  prob2 = 1.0 - prob1;
  sideinfo_flags_entropy =
    (-prob1*QccMathLog2(prob1) - prob2*QccMathLog2(prob2));

  if (!entropy_code_sideinfo)
    {
      precision = 
        (int)(log((double)sideinfo->codebook_coder.num_levels)/log(2.0) + 0.5);

      num_bits_sideinfo_flags = num_input_vectors;
      num_bits_sideinfo_updatevectors =
        total_updated * sideinfo->vector_dimension * precision;
    }
  else
    {
      num_bits_sideinfo_flags = num_input_vectors * sideinfo_flags_entropy;
      num_bits_sideinfo_updatevectors =
        total_updated * sideinfo->vector_dimension * sideinfo_entropy;
    }

  num_bits_channel = (num_input_vectors - total_updated) * channel_entropy;
  num_bits_sideinfo = 
    num_bits_sideinfo_flags + num_bits_sideinfo_updatevectors;
  num_bits_total = num_bits_channel + num_bits_sideinfo;
  channel_rate = num_bits_channel/num_input_vectors/sideinfo->vector_dimension;
  sideinfo_updatevectors_rate =
    num_bits_sideinfo_updatevectors / 
    num_input_vectors/sideinfo->vector_dimension;
  sideinfo_flags_rate =
    num_bits_sideinfo_flags / 
    num_input_vectors/sideinfo->vector_dimension;
  sideinfo_rate = 
    num_bits_sideinfo/num_input_vectors/sideinfo->vector_dimension;
  total_rate = 
    num_bits_total/num_input_vectors/sideinfo->vector_dimension;
  
  if (verbose >= 2)
    {
      fprintf(outfile,
              "Rate of the Paul algorithm with channel\n   %s\n",
              channel->filename);
      fprintf(outfile,
              "     and side information\n   %s\n",
              sideinfo->filename);
      fprintf(outfile,
              "---------------------------------------------------\n");
      fprintf(outfile,
              "  Rate for VQ channel: %f\n",
              channel_rate);
      fprintf(outfile,
              "  Rate for side information:\t");
      fprintf(outfile,
              "%.2f%% of vectors updated\n",
              QccMathPercent(total_updated, num_input_vectors));
      fprintf(outfile,
              "      Rate for flags:\t\t%f (%5.2f%% of total rate)\n",
              sideinfo_flags_rate,
              QccMathPercent(sideinfo_flags_rate, total_rate));
      fprintf(outfile,
              "      Rate for update vectors:\t%f (%5.2f%% of total rate)\n",
              sideinfo_updatevectors_rate,
              QccMathPercent(sideinfo_updatevectors_rate, total_rate));
      fprintf(outfile,
              "    Total side information:\t%f (%5.2f%% of total rate)\n",
              sideinfo_rate,
              QccMathPercent(sideinfo_rate, total_rate));
      fprintf(outfile,
              "---------------------------------------------------\n");
      fprintf(outfile,
              "  Overall rate:      %f bits per original source symbol\n", 
              total_rate);
    }
  else
    if (verbose == 1)
      fprintf(outfile,
              "%f\n", total_rate);
  
  if (rate != NULL)
    *rate = total_rate;
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccAVQSideInfoSymbolFree(sideinfo_symbol);
  QccChannelFree(&sideinfo_channel);
  return(return_value);
}
