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


static void QccAVQgtrCalcProb(QccVQCodebook *codebook, const QccVector cnt)
{
  int codeword;
  double sum_cnt = 0;
  
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    sum_cnt += cnt[codeword];
  for (codeword = 0; codeword < codebook->num_codewords; codeword++)
    codebook->codeword_probs[codeword] = cnt[codeword]/sum_cnt;
}


static int QccAVQgtrSendSideInfo(QccAVQSideInfo *sideinfo, 
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
      QccErrorAddMessage("(QccAVQgtrSendSideInfo): Error calling QccAVQSideInfoSymbolWrite()");
      return(1);
    }
  
  if (update_flag == QCCAVQSIDEINFO_FLAG_UPDATE)
    {
      updatevector_symbol->type = QCCAVQSIDEINFO_VECTOR;
      if (QccAVQSideInfoSymbolWrite(sideinfo, updatevector_symbol))
        {
          QccErrorAddMessage("(QccAVQgtrSendSideInfo): Error calling QccAVQSideInfoSymbolWrite()");
          return(1);
        }
    }
  
  return(0);
}


static void QccAVQgtrCalcUpdateCosts(double *update_distortion,
                                     double *update_rate,
                                     const
                                     QccAVQSideInfoSymbol *new_codeword_symbol,
                                     QccVector current_vector,
                                     QccAVQSideInfo *sideinfo)
{
  int offset;
  int component;
  double current_prob;

  offset = sideinfo->codebook_coder.num_levels/2;

  *update_distortion = 
    QccVectorSquareDistance(current_vector,
                            new_codeword_symbol->vector,
                            sideinfo->vector_dimension);

  *update_rate = 0;

  switch (sideinfo->codebook_coder.type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      for (component = 0; component < sideinfo->vector_dimension; component++)
        {
          current_prob = sideinfo->codebook_coder.probs
               [new_codeword_symbol->vector_indices[component]];
          if (current_prob != 0.0)
            *update_rate += -QccMathLog2(current_prob);
        }
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
    case QCCSQSCALARQUANTIZER_DEADZONE:
      for (component = 0; component < sideinfo->vector_dimension; component++)
        {
          current_prob = sideinfo->codebook_coder.probs
            [new_codeword_symbol->vector_indices[component] + offset];
          if (current_prob != 0.0)
            *update_rate += -QccMathLog2(current_prob);
        }
      break;
    }
}


static void QccAVQgtrUpdateCodebookCoderProbs(QccAVQSideInfo *sideinfo,
                                              const QccAVQSideInfoSymbol 
                                              *new_codeword_symbol,
                                              double omega)
{
  int cnt_total = 0;
  int component;
  int offset;
  int codeword;

  offset = sideinfo->codebook_coder.num_levels/2;

  for (codeword = 0; codeword < sideinfo->codebook_coder.num_levels; 
       codeword++)
    {
      sideinfo->codebook_coder.probs[codeword] =
        (int)(sideinfo->codebook_coder.probs[codeword] * omega + 0.5);
      cnt_total +=
        sideinfo->codebook_coder.probs[codeword];
    }

  switch (sideinfo->codebook_coder.type)
    {
    case QCCSQSCALARQUANTIZER_GENERAL:
      for (component = 0; component < sideinfo->vector_dimension; component++)
        sideinfo->codebook_coder.probs
          [new_codeword_symbol->vector_indices[component]] += 1;
      break;

    case QCCSQSCALARQUANTIZER_UNIFORM:
    case QCCSQSCALARQUANTIZER_DEADZONE:
      for (component = 0; component < sideinfo->vector_dimension; component++)
        sideinfo->codebook_coder.probs
          [new_codeword_symbol->vector_indices[component] + offset] += 1;
      break;
    }

  cnt_total += sideinfo->vector_dimension;

  for (codeword = 0; codeword < sideinfo->codebook_coder.num_levels; 
       codeword++)
    sideinfo->codebook_coder.probs[codeword] /= cnt_total;

}


int QccAVQgtrEncode(const QccDataset *dataset, 
                    QccVQCodebook *codebook, 
                    QccChannel *channel,
                    QccAVQSideInfo *sideinfo,
                    double lambda,
                    double omega,
                    QccVQDistortionMeasure distortion_measure)
{
  int return_value;
  int block_size;
  int vector_dimension;
  int current_vector;
  double winning_distortion;
  double update_distortion;
  double update_rate;
  int winner;
  QccDataset tmp_dataset;
  QccVector cnt = NULL;
  int codeword;
  double newcnt;
  double delta_J;
  QccAVQSideInfoSymbol *new_codeword_symbol = NULL;

  if ((dataset == NULL) || (codebook == NULL) || (channel == NULL) ||
      (sideinfo == NULL))
    return(0);

  if (dataset->vectors == NULL)
    return(0);
  
  vector_dimension = dataset->vector_dimension;
  if ((vector_dimension != codebook->codeword_dimension) ||
      (vector_dimension != sideinfo->vector_dimension))
    {
      QccErrorAddMessage("(QccAVQgtrEncode): Codebook %s, dataset %s, and sideinfo %s have different vector dimensions",
                         codebook->filename, dataset->filename, sideinfo->filename);
      goto QccAVQError;
    }
  
  if ((new_codeword_symbol = 
       QccAVQSideInfoSymbolAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccAVQSideInfoSymbolAlloc()");
      goto QccAVQError;
    }
  new_codeword_symbol->type = QCCAVQSIDEINFO_VECTOR;

  /*
    precision = 
    (int)(log((double)sideinfo->codebook_coder.num_levels)/log(2.0) + 0.5);
  */

  block_size = QccDatasetGetBlockSize(dataset);
  
  QccDatasetInitialize(&tmp_dataset);
  tmp_dataset.num_vectors = 1;
  tmp_dataset.vector_dimension = dataset->vector_dimension;
  tmp_dataset.access_block_size = QCCDATASET_ACCESSWHOLEFILE;
  tmp_dataset.num_blocks_accessed = 0;
  if (QccDatasetAlloc(&tmp_dataset))
    {
      QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccDatasetAlloc()");
      goto QccAVQError;
    }
  
  if ((cnt = 
       QccVectorAlloc(sideinfo->max_codebook_size)) == NULL)
    {
      QccErrorAddMessage("(QccAVQgtrEncode): Error allocating memory");
      goto QccAVQError;
    }
  
  for (current_vector = 0; current_vector < block_size; current_vector++)
    {
      QccVectorCopy(tmp_dataset.vectors[0],
                    dataset->vectors[current_vector],
                    vector_dimension);
      
      if (QccVQEntropyConstrainedVQ(&tmp_dataset, codebook, lambda, 
                                    (QccVector)&winning_distortion, &winner,
                                    distortion_measure))
        {
          QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccVQEntropyConstrainedVQ()");
          goto QccAVQError;
        }

      QccVectorCopy(new_codeword_symbol->vector,
                    dataset->vectors[current_vector],
                    vector_dimension);
      QccAVQSideInfoCodebookCoder(sideinfo, new_codeword_symbol);

      /*
        update_distortion = 
        QccVectorSquareDistance(dataset->vectors[current_vector],
        new_codeword_symbol->vector,
        vector_dimension);
      */

      QccAVQgtrCalcUpdateCosts(&update_distortion,
                               &update_rate,
                               new_codeword_symbol,
                               dataset->vectors[current_vector],
                               sideinfo);

      for (codeword = 0; codeword < codebook->num_codewords; codeword++)
        cnt[codeword] = 
          (int)(codebook->codeword_probs[codeword] * omega + 0.5);

      /*
        delta_J = (update_distortion - winning_distortion) +
        lambda*vector_dimension*precision;
      */
      delta_J = (update_distortion - winning_distortion) +
        lambda * update_rate;
      
      if (delta_J < 0.0)
        {
          /*  Update codebook  */
          if (codebook->num_codewords < sideinfo->max_codebook_size)
            {
              if (QccVQCodebookAddCodeword(codebook,
                                           (QccVQCodeword)
                                           new_codeword_symbol->vector))
                {
                  QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccVQCodebookAddCodeword()");
                  goto QccAVQError;
                }
              winner = codebook->num_codewords - 1;
              newcnt = 1;
            }
          else
            {
              QccVectorCopy((QccVector)codebook->codewords
                            [codebook->num_codewords - 1], 
                            new_codeword_symbol->vector,
                            vector_dimension);
              newcnt = cnt[winner] + 1;
            }
          
          if (QccVQCodebookMoveCodewordToFront(codebook, 
                                               codebook->num_codewords - 1))
            {
              QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccVQCodebookMoveCodewordToFront()");
              goto QccAVQError;
            }
          
          if (winner != (codebook->num_codewords - 1))
            {
              cnt[winner] = newcnt/2;
              cnt[codebook->num_codewords - 1] = cnt[winner];
            }
          else
            cnt[winner] = newcnt;
          
          QccAVQgtrCalcProb(codebook, cnt);
          QccVectorMoveComponentToFront(codebook->codeword_probs,
                                        codebook->num_codewords,
                                        codebook->num_codewords - 1);
          
          channel->channel_symbols[current_vector] = QCCCHANNEL_NULLSYMBOL;
          
          if (QccAVQgtrSendSideInfo(sideinfo, new_codeword_symbol, 
                                    QCCAVQSIDEINFO_FLAG_UPDATE))
            {
              QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccAVQgtrSendSideInfo()");
              goto QccAVQError;
            }
          QccAVQgtrUpdateCodebookCoderProbs(sideinfo,
                                            new_codeword_symbol,
                                            omega);
        }
      else
        {
          /*  No codebook update  */
          QccVQCodebookMoveCodewordToFront(codebook, winner);
          cnt[winner]++;
          QccAVQgtrCalcProb(codebook, cnt);
          if (QccVectorMoveComponentToFront(codebook->codeword_probs,
                                            codebook->num_codewords,
                                            winner))
            {
              QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccVQCodebookMoveCodewordToFront()");
              goto QccAVQError;
            }

          channel->channel_symbols[current_vector] = winner;

          if (QccAVQgtrSendSideInfo(sideinfo, NULL,
                                    QCCAVQSIDEINFO_FLAG_NOUPDATE))
            {
              QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccAVQgtrSendSideInfo()");
              goto QccAVQError;
            }
        }
    }
  
  QccVQCodebookSetCodewordLengths(codebook);
  
  return_value = 0;
  goto QccAVQExit;
  
 QccAVQError:
  return_value = 1;

 QccAVQExit:
  QccDatasetFree(&tmp_dataset);
  QccVectorFree(cnt);
  if (new_codeword_symbol != NULL)
    QccAVQSideInfoSymbolFree(new_codeword_symbol);

  return(return_value);
}


int QccAVQgtrDecode(QccDataset *dataset,
                    QccVQCodebook *codebook,
                    const QccChannel *channel,
                    QccAVQSideInfo *sideinfo)
{
  int return_value;
  int vector_dimension, block_size;
  int current_vector;
  QccAVQSideInfoSymbol *current_symbol = NULL;
  
  if ((dataset == NULL) || (codebook == NULL) || (channel == NULL) ||
      (sideinfo == NULL))
    return(0);
  if (sideinfo->fileptr == NULL)
    return(0);
  if ((dataset->vectors == NULL) ||
      (channel->channel_symbols == NULL))
    return(0);
  
  vector_dimension = dataset->vector_dimension;
  if ((vector_dimension != codebook->codeword_dimension) ||
      (vector_dimension != sideinfo->vector_dimension))
    {
      QccErrorAddMessage("(QccAVQgtrEncode): Codebook %s, dataset %s, and sideinfo %s have different vector dimensions",
                         codebook->filename, dataset->filename, sideinfo->filename);
      goto QccAVQError;
    }
  
  if ((current_symbol = QccAVQSideInfoSymbolAlloc(vector_dimension)) == NULL)
    {
      QccErrorAddMessage("(QccAVQgtrDecode): Error calling QccAVQSideInfoSymbolAlloc()");
      goto QccAVQError;
    }

  block_size = QccDatasetGetBlockSize(dataset);
  
  for (current_vector = 0; current_vector < block_size; current_vector++)
    {
      if (QccAVQSideInfoSymbolReadDesired(sideinfo,
                                          QCCAVQSIDEINFO_FLAG,
                                          current_symbol) == NULL)
        {
          QccErrorAddMessage("(QccAVQgtrDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
          goto QccAVQError;
        }
      
      if (current_symbol->flag == QCCAVQSIDEINFO_FLAG_UPDATE)
        {
          /*  Update codebook  */
          if (QccAVQSideInfoSymbolReadDesired(sideinfo,
                                              QCCAVQSIDEINFO_VECTOR,
                                              current_symbol) == NULL)
            {
              QccErrorAddMessage("(QccAVQgtrDecode): Error calling QccAVQSideInfoSymbolReadDesired()");
              goto QccAVQError;
            }
          if (codebook->num_codewords < sideinfo->max_codebook_size)
            {
              if (QccVQCodebookAddCodeword(codebook,
                                           (QccVQCodeword)
                                           current_symbol->vector))
                {
                  QccErrorAddMessage("(QccAVQgtrDecode): Error calling QccVQCodebookAddCodeword()");
                  goto QccAVQError;
                }
            }
          else
            QccVectorCopy((QccVector)codebook->codewords
                          [codebook->num_codewords - 1], 
                          current_symbol->vector, vector_dimension);

          if (QccVQCodebookMoveCodewordToFront(codebook, 
                                               codebook->num_codewords - 1))
            {
              QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccVQCodebookMoveCodewordToFront()");
              goto QccAVQError;
            }
        }
      else
        {
          /*  No codebook update  */
          if (QccVQCodebookMoveCodewordToFront(codebook,
                                               channel->channel_symbols
                                               [current_vector]))
            {
              QccErrorAddMessage("(QccAVQgtrEncode): Error calling QccVQCodebookMoveCodewordToFront()");
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


int QccAVQgtrCalcRate(QccChannel *channel, 
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
      QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccAVQSideInfoSymbolAlloc()");
      goto Error;
    }

  QccChannelInitialize(&sideinfo_channel);

  sideinfo_channel.channel_length = channel->channel_length *
    sideinfo->vector_dimension;
  sideinfo_channel.alphabet_size = sideinfo->codebook_coder.num_levels;
  sideinfo_channel.access_block_size = QCCCHANNEL_ACCESSWHOLEFILE;
  if (QccChannelAlloc(&sideinfo_channel))
    {
      QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccChannelAlloc()");
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
          QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccAVQSideInfoSymbolReadNextFlag()");
          goto Error;
        }
      if (sideinfo_symbol->flag == QCCAVQSIDEINFO_FLAG_UPDATE)
        {
          if (QccAVQSideInfoSymbolReadDesired(sideinfo,
                                              QCCAVQSIDEINFO_VECTOR,
                                              sideinfo_symbol) == NULL)
            {
              QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccAVQSideInfoSymbolReadDesired()");
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

  if (sideinfo->codebook_coder.type != QCCSQSCALARQUANTIZER_GENERAL)
    {
      if (QccChannelNormalize(&sideinfo_channel))
        {
          QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccChannelNormalize()");
          goto Error;
        }
    }

  if ((sideinfo_entropy = QccChannelEntropy(&sideinfo_channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccChannelEntropy()");
      goto Error;
    }

  if (QccChannelRemoveNullSymbols(channel))
    {
      QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccChannelRemoveNullSymbols()");
      goto Error;
    }

  if ((channel_entropy = QccChannelEntropy(channel, 1)) < 0)
    {
      QccErrorAddMessage("(QccAVQgtrCalcRate): Error calling QccChannelEntropy()");
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
              "Rate of GTR with channel\n   %s\n",
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
