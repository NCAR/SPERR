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

static void QccAVQGershoYanoCalculatePartialDistortion(QccVector 
                                                       partial_distortion,
                                                       int codebook_size,
                                                       QccVector distortion,
                                                       int *partition,
                                                       int adaption_interval,
                                                       int *max_index,
                                                       int *min_index,
                                                       int *winner_cnt)
{
  double max_partial_distortion, min_partial_distortion;
  int codeword;
  int current_vector;
  int codeword_cnt;
  
  *max_index = *min_index = 0;
  max_partial_distortion = -MAXDOUBLE;
  min_partial_distortion = MAXDOUBLE;
  *winner_cnt = 0;
  
  for (codeword = 0; codeword < codebook_size; codeword++)
    {
      codeword_cnt = partial_distortion[codeword] = 0;
      
      for (current_vector = 0; current_vector < adaption_interval;
           current_vector++)
        if (partition[current_vector] == codeword)
          {
            codeword_cnt++;
            partial_distortion[codeword] += distortion[current_vector];
          }
      
      if (codeword_cnt > 0)
        /* Partial distortion = avg. distortion * partition prob */
        partial_distortion[codeword] /= adaption_interval;
      else
        partial_distortion[codeword] = 0;
      
      if (partial_distortion[codeword] > max_partial_distortion)
        {
          max_partial_distortion = partial_distortion[codeword];
          *max_index = codeword;
          *winner_cnt = codeword_cnt;
        }
      if (partial_distortion[codeword] < min_partial_distortion)
        {
          min_partial_distortion = partial_distortion[codeword];
          *min_index = codeword;
        }
    }
  
}


static int QccAVQGershoYanoGetNewCodewords(int *partition, 
                                           int adaption_interval,
                                           int winner, 
                                           QccAVQSideInfo *sideinfo,
                                           QccVQCodebook *codebook,
                                           const QccDataset *dataset,
                                           QccAVQSideInfoSymbol 
                                           **new_codeword_symbols,
                                           QccVQDistortionMeasure
                                           distortion_measure,
                                           QccVQGeneralizedLloydCentroids
                                           centroid_calculation)
{
  int return_value;
  QccDataset winners_dataset;
  QccVQCodebook new_codewords_codebook;
  int vector_dimension;
  double signal_power;
  int num_winners = 0;
  int component;
  int current_vector;
  int current_winner;
  int codeword_cnt;

  vector_dimension = dataset->vector_dimension;
  
  /*  Find number of vectors mapping to partition of codeword with
      highest partitial distortion  */
  for (current_vector = 0; current_vector < adaption_interval;
       current_vector++)
    if (partition[current_vector] == winner)
      num_winners++;
  
  QccDatasetInitialize(&winners_dataset);
  winners_dataset.num_vectors = num_winners;
  winners_dataset.vector_dimension = dataset->vector_dimension;
  winners_dataset.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  winners_dataset.num_blocks_accessed = 0;
  if (QccDatasetAlloc(&winners_dataset))
    {
      QccErrorAddMessage("(QccAVQGershoYanoGetNewCodewords): Error calling QccDatasetAlloc()");
      goto QccAVQError;
    }
  
  QccVQCodebookInitialize(&new_codewords_codebook);
  new_codewords_codebook.num_codewords = QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
  new_codewords_codebook.codeword_dimension = codebook->codeword_dimension;
  if (QccVQCodebookAlloc(&new_codewords_codebook))
    {
      QccErrorAddMessage("(QccAVQGershoYanoGetNewCodewords): Error calling QccVQCodebookAlloc()");
      goto QccAVQError;
    }

  for (current_vector = 0, current_winner = 0; 
       current_vector < adaption_interval;
       current_vector++)
    if (partition[current_vector] == winner)
      QccVectorCopy(winners_dataset.vectors[current_winner++],
                    dataset->vectors[current_vector], vector_dimension);
  
  signal_power = QccMatrixMaxSignalPower(winners_dataset.vectors,
                                              num_winners, vector_dimension);
  
  for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
       codeword_cnt++)
    {
      /*  Copy codeword with highest partial distortion  */
      QccVectorCopy((QccVector)new_codewords_codebook.codewords[codeword_cnt],
                    codebook->codewords[winner], 
                    vector_dimension);
  
      /*  Add small random displacements  */
      for (component = 0; component < vector_dimension; component++)
        new_codewords_codebook.codewords[codeword_cnt][component] +=
          ((QccMathRand() - 0.5)*2.0)*0.0001*sqrt(signal_power);
    }
  
  /*  One iteration of GLA  */
  if (QccVQGeneralizedLloydVQTraining(&winners_dataset,
                                      &new_codewords_codebook, 1, 0.0,
                                      distortion_measure,
                                      centroid_calculation, 0))
    {
      QccErrorAddMessage("(QccAVQGershoYanoGetNewCodewords): Error calling QccVQGeneralizedLloydVQTraining()");
      goto QccAVQError;
    }
  
  for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
       codeword_cnt++)
    {
      QccVectorCopy((new_codeword_symbols[codeword_cnt])->vector,
                    new_codewords_codebook.codewords[codeword_cnt],
                    vector_dimension);
    }

  return_value = 0;
  goto QccAVQExit;
  
 QccAVQError:
  return_value = 1;

 QccAVQExit:
  QccDatasetFree(&winners_dataset);
  QccVQCodebookFree(&new_codewords_codebook);
  return(return_value);
}


static int QccAVQGershoYanoSendSideInfo(QccAVQSideInfo *sideinfo, 
                                        QccAVQSideInfoSymbol 
                                        **new_codeword_symbols,
                                        int *addresses)
{
  QccAVQSideInfoSymbol address_symbol;
  int vector_cnt;

  for (vector_cnt = 0; vector_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS; 
       vector_cnt++)
    {
      address_symbol.type = QCCAVQSIDEINFO_ADDRESS;
      address_symbol.address = addresses[vector_cnt];
      address_symbol.vector = NULL;
      address_symbol.vector_indices = NULL;
  
      if (QccAVQSideInfoSymbolWrite(sideinfo, &address_symbol))
        {
          QccErrorAddMessage("(QccAVQGershoYanoSendSideInfo): Error calling QccAVQSideInfoSymbolWrite()");
          return(1);
        }

      (new_codeword_symbols[vector_cnt])->type = QCCAVQSIDEINFO_VECTOR;

      if (QccAVQSideInfoSymbolWrite(sideinfo, 
                                    new_codeword_symbols[vector_cnt]))
        {
          QccErrorAddMessage("(QccAVQGershoYanoSendSideInfo): Error calling QccAVQSideInfoSymbolWrite()");
          return(1);
        }
    }
  
  return(0);
}


int QccAVQGershoYanoEncode(const QccDataset *dataset,
                           QccVQCodebook *codebook, 
                           QccChannel *channel,
                           QccAVQSideInfo *sideinfo,
                           int adaption_interval,
                           QccVQDistortionMeasure distortion_measure,
                           QccVQGeneralizedLloydCentroids
                           centroid_calculation)
{
  int return_value;
  double vector_dimension;
  double block_size;
  double codebook_size;
  int *partition = NULL;
  QccVector distortion = NULL;
  QccVector partial_distortion = NULL;
  QccAVQSideInfoSymbol **new_codeword_symbols = NULL;
  int codeword_cnt;
  int winners[QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS];
  int winner_cnt;
  int current_vector;
  
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
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Codebook %s, dataset %s, and sideinfo %s",
                         codebook->filename, dataset->filename, sideinfo->filename);
      QccErrorAddMessage("have different vector dimensions");
      goto QccAVQError;
    }
  
  block_size = QccDatasetGetBlockSize(dataset);
  
  codebook_size = codebook->num_codewords;
  
  if ((partition = (int *)malloc(sizeof(int)*block_size)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error allocating memory");
      goto QccAVQError;
    }
  if ((distortion = QccVectorAlloc(block_size)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error allocating memory");
      goto QccAVQError;
    }
  if ((partial_distortion = QccVectorAlloc(codebook_size)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error allocating memory");
      goto QccAVQError;
    }
  
  if ((new_codeword_symbols = 
       (QccAVQSideInfoSymbol **)malloc(sizeof(QccAVQSideInfoSymbol *) *
                                       QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS))
      == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error allocating memory");
      goto QccAVQError;
    }
  for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
       codeword_cnt++)
    new_codeword_symbols[codeword_cnt] = NULL;
  for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
       codeword_cnt++)
    if ((new_codeword_symbols[codeword_cnt] =
         QccAVQSideInfoSymbolAlloc(vector_dimension)) == NULL)
      {
        QccErrorAddMessage("(QccAVQGershoYanoEncode): Error calling QccAVQSideInfoSymbolAlloc()");
        goto QccAVQError;
      }

  if (QccVQVectorQuantization(dataset, codebook, 
                              distortion, partition, distortion_measure))
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error calling QccVQVectorQuantization()");
      goto QccAVQError;
    }
  
  QccAVQGershoYanoCalculatePartialDistortion(partial_distortion,
                                             codebook_size,
                                             distortion,
                                             partition,
                                             adaption_interval,
                                             &winners[0],
                                             &winners[1],
                                             &winner_cnt);
  
  if (winner_cnt)
    {
      /*  winners[] must be change for more than 2 winners (winner, loser) */
      if (QccAVQGershoYanoGetNewCodewords(partition, adaption_interval,
                                          winners[0], 
                                          sideinfo, codebook, dataset,
                                          new_codeword_symbols,
                                          distortion_measure,
                                          centroid_calculation))
        {
          QccErrorAddMessage("(QccAVQGershoYanoEncoder): Error calling QccAVQGershoYanoGetNEwCodewords()");
          goto QccAVQError;
        }
    }
  else
    {
      for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
           codeword_cnt++)
        QccVectorCopy((new_codeword_symbols[codeword_cnt])->vector,
                      codebook->codewords[winners[codeword_cnt]],
                      vector_dimension);
    }
  
  for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
       codeword_cnt++)
    QccAVQSideInfoCodebookCoder(sideinfo,
                                new_codeword_symbols[codeword_cnt]);

  if (QccAVQGershoYanoSendSideInfo(sideinfo, new_codeword_symbols, 
                                   winners))
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error calling QccAVQGershoYanoSendSideInfo()");
      goto QccAVQError;
    }
  
  for (codeword_cnt = 0; codeword_cnt < QCCAVQ_GERSHOYANO_NUMUPDATEVECTORS;
       codeword_cnt++)
    QccVectorCopy(codebook->codewords[winners[codeword_cnt]],
                  (new_codeword_symbols[codeword_cnt])->vector,
                  vector_dimension);
  
  if (QccVQVectorQuantization(dataset, codebook, 
                              distortion, partition, distortion_measure))
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Error calling QccVQVectorQuantization()");
      goto QccAVQError;
    }
  
  for (current_vector = 0; current_vector < adaption_interval; 
       current_vector++)
    channel->channel_symbols[current_vector] = partition[current_vector];
  
  return_value = 0;
  goto QccAVQExit;
  
 QccAVQError:
  return_value = 1;

 QccAVQExit:
  if (partition != NULL)
    QccFree(partition);
  if (distortion != NULL)
    QccVectorFree(distortion);
  if (partial_distortion != NULL)
    QccVectorFree(partial_distortion);
  if (new_codeword_symbols != NULL)
    {
    }
  return(return_value);
}


int QccAVQGershoYanoDecode(QccDataset *dataset, QccVQCodebook *codebook,
                           QccChannel *channel, QccAVQSideInfo *sideinfo)
{
  int vector_dimension;
  int block_size;
  int current_vector;
  QccAVQSideInfoSymbol *current_symbol = NULL;
  int address1, address2;
  
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
  block_size = QccDatasetGetBlockSize(dataset);
  
  if ((vector_dimension != codebook->codeword_dimension) ||
      (vector_dimension != sideinfo->vector_dimension))
    {
      QccErrorAddMessage("(QccAVQGershoYanoEncode): Codebook %s, dataset %s, and sideinfo %s",
                         codebook->filename, dataset->filename, sideinfo->filename);
      QccErrorAddMessage("have different vector dimensions");
      goto QccAVQGershoYanoDecodeErr;
    }
  
  if (channel->access_block_size != block_size)
    {
      QccErrorAddMessage("(QccAVQGershoYanoDecode): Channel %s must have same block size as dataset %s", 
                         channel->filename, dataset->filename);
      goto QccAVQGershoYanoDecodeErr;
    }
  
  if ((current_symbol = 
       QccAVQSideInfoSymbolReadDesired(sideinfo,
                                       QCCAVQSIDEINFO_ADDRESS,
                                       NULL)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
      goto QccAVQGershoYanoDecodeErr;
    }
  address1 = current_symbol->address;
  QccAVQSideInfoSymbolFree(current_symbol);
  if ((current_symbol = 
       QccAVQSideInfoSymbolReadDesired(sideinfo,
                                       QCCAVQSIDEINFO_VECTOR,
                                       NULL)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
      goto QccAVQGershoYanoDecodeErr;
    }
  QccVectorCopy((QccVector)codebook->codewords[address1], 
                current_symbol->vector, vector_dimension);
  
  QccAVQSideInfoSymbolFree(current_symbol);
  if ((current_symbol = 
       QccAVQSideInfoSymbolReadDesired(sideinfo,
                                       QCCAVQSIDEINFO_ADDRESS,
                                       NULL)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
      goto QccAVQGershoYanoDecodeErr;
    }
  address2 = current_symbol->address;
  QccAVQSideInfoSymbolFree(current_symbol);
  if ((current_symbol = 
       QccAVQSideInfoSymbolReadDesired(sideinfo,
                                       QCCAVQSIDEINFO_VECTOR,
                                       NULL)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
      goto QccAVQGershoYanoDecodeErr;
    }
  QccVectorCopy((QccVector)codebook->codewords[address2], 
                current_symbol->vector, vector_dimension);
  
  for (current_vector = 0; current_vector < block_size; current_vector++)
    QccVectorCopy(dataset->vectors[current_vector],
                  (QccVector)codebook->codewords
                  [channel->channel_symbols[current_vector]], 
                  vector_dimension);
  
  if (current_symbol != NULL)
    QccAVQSideInfoSymbolFree(current_symbol);
  return(0);
  
 QccAVQGershoYanoDecodeErr:
  if (current_symbol != NULL)
    QccAVQSideInfoSymbolFree(current_symbol);
  return(1);
}


int QccAVQGershoYanoCalcRate(QccChannel *channel, 
                             QccAVQSideInfo *sideinfo,
                             FILE *outfile, 
                             double *rate, 
                             int verbose,
                             int entropy_code_sideinfo)
{
  int return_value;
  int num_input_vectors;
  int total_updated;
  int adaption_interval;
  int num_adaption_intervals;
  double channel_entropy;
  double num_bits_sideinfo;
  double total_rate, channel_rate, sideinfo_rate;
  int precision;
  QccAVQSideInfoSymbol *sideinfo_symbol = NULL;
  QccChannel sideinfo_address_channel;
  QccChannel sideinfo_vector_channel;
  double sideinfo_address_entropy;
  double sideinfo_vector_entropy;
  int update, component;

  if ((outfile == NULL) ||
      (channel == NULL) ||
      (sideinfo == NULL))
    return(0);
  
  if (QccChannelRemoveNullSymbols(channel))
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccChannelRemoveNullSymbols()");
      goto Error;
    }

  if ((channel_entropy = QccChannelEntropy(channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccChannelEntropy()");
      goto Error;
    }
  
  if ((sideinfo_symbol = 
       QccAVQSideInfoSymbolAlloc(sideinfo->vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccAVQSideInfoSymbolAlloc()");
      goto Error;
    }

  num_input_vectors = channel->channel_length;
  adaption_interval = sideinfo->N;
  num_adaption_intervals = 
    (int)ceil((double)num_input_vectors / adaption_interval);
  total_updated = num_adaption_intervals * 2;

  QccChannelInitialize(&sideinfo_address_channel);
  sideinfo_address_channel.channel_length = total_updated;
  sideinfo_address_channel.alphabet_size =
    channel->alphabet_size;
  sideinfo_address_channel.access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  if (QccChannelAlloc(&sideinfo_address_channel))
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccChannelAlloc()");
      goto Error;
    }

  QccChannelInitialize(&sideinfo_vector_channel);
  sideinfo_vector_channel.channel_length =
    total_updated * sideinfo->vector_dimension;
  sideinfo_vector_channel.alphabet_size = sideinfo->codebook_coder.num_levels;
  sideinfo_vector_channel.access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  if (QccChannelAlloc(&sideinfo_vector_channel))
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccChannelAlloc()");
      goto Error;
    }
  
  for (update = 0; update < total_updated; update++)
    {
      if (QccAVQSideInfoSymbolReadDesired(sideinfo,
                                          QCCAVQSIDEINFO_ADDRESS,
                                          sideinfo_symbol) == NULL)
        {
          QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccAVQSideInfoSymbolReadDesired()");
          goto Error;
        }
      sideinfo_address_channel.channel_symbols[update] =
        sideinfo_symbol->address;

      if (QccAVQSideInfoSymbolReadDesired(sideinfo,
                                          QCCAVQSIDEINFO_VECTOR,
                                          sideinfo_symbol) == NULL)
        {
          QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccAVQSideInfoSymbolReadDesired()");
          goto Error;
        }
      for (component = 0; component < sideinfo->vector_dimension;
           component++)
        sideinfo_vector_channel.channel_symbols[update * 
                                               sideinfo->vector_dimension +
                                               component] =
          sideinfo_symbol->vector_indices[component];
    }

  if ((sideinfo_address_entropy = 
       QccChannelEntropy(&sideinfo_address_channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccChannelEntropy()");
      goto Error;
    }
  if ((sideinfo_vector_entropy = 
       QccChannelEntropy(&sideinfo_vector_channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQGershoYanoCalcRate): Error calling QccChannelEntropy()");
      goto Error;
    }

  if (!entropy_code_sideinfo)
    {
      precision = 
        (int)(log((double)sideinfo->codebook_coder.num_levels)/log(2.0) + 0.5);

      num_bits_sideinfo =
        total_updated * 
        ((int)(log((double)channel->alphabet_size)/log(2.0) + 0.5) +
         sideinfo->vector_dimension*precision);
    }
  else
    {
      num_bits_sideinfo =
        total_updated *
        (sideinfo_address_entropy +
         sideinfo_vector_entropy * sideinfo->vector_dimension);
    }

  channel_rate = channel_entropy/sideinfo->vector_dimension;

  sideinfo_rate = 
    num_bits_sideinfo/num_input_vectors/sideinfo->vector_dimension;

  total_rate = 
    channel_rate + sideinfo_rate;

  if (verbose >= 2)
    {
      fprintf(outfile,
              "Rate of the Gersho-Yano algorithm with channel\n   %s\n",
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
              "  Rate for side information: %f\n",
              sideinfo_rate);
      fprintf(outfile,
              "\t(%5.2f%% of total rate)\n",
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
  QccChannelFree(&sideinfo_address_channel);
  QccChannelFree(&sideinfo_vector_channel);
  return(return_value);
}
