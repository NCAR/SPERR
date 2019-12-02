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


static int QccENTArithmeticModelInitialize(QccENTArithmeticModel *model)
{
  if (model == NULL)
    return(0);

  model->num_contexts = 0;
  model->num_symbols = NULL;
  model->translate_symbol_to_index = NULL;
  model->translate_index_to_symbol = NULL;
  model->cumulative_frequencies = NULL;
  model->frequencies = NULL;
  model->low = model->high = 0;
  model->bits_to_follow = 0;
  model->garbage_bits = 0;
  model->target_num_bits = QCCENT_ANYNUMBITS;
  model->adaptive_model = QCCENT_ADAPTIVE;
  model->context_function = NULL;
  model->current_context = 0;
  model->current_code_value = 0;

  return(0);
}


static void QccENTArithmeticFreeArray(int **array, int num_contexts)
{
  int context;
  
  if (array == NULL)
    return;
  
  for (context = 0; context < num_contexts; context++)
    if (array[context] != NULL)
      QccFree(array[context]);
  QccFree(array);
  
}


static int **QccENTArithmeticAllocArray(const int *num_symbols,
                                        int num_contexts)
{
  int **array = NULL;
  int context;
  
  if (num_symbols == NULL)
    return(NULL);

  if ((array = (int **)malloc(sizeof(int *)*num_contexts)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticAllocArray): Error allocating memory");
      return(NULL);
    }
  
  /*  Save an extra space total cumulative frequency  */
  for (context = 0; context < num_contexts; context++)
    if ((array[context] = 
         (int *)malloc(sizeof(int)*(num_symbols[context] + 1))) == NULL)
      {
        QccErrorAddMessage("(QccENTArithmeticAllocArray): Error allocating memory");
        QccENTArithmeticFreeArray(array, num_contexts);
        return(NULL);
      }
  
  return(array);
}


static void QccENTArithmeticFreeModelArrays(QccENTArithmeticModel *model)
{
  if (model == NULL)
    return;

  QccENTArithmeticFreeArray(model->translate_symbol_to_index, 
                            model->num_contexts);
  QccENTArithmeticFreeArray(model->translate_index_to_symbol, 
                            model->num_contexts);
  QccENTArithmeticFreeArray(model->cumulative_frequencies, 
                            model->num_contexts);
  QccENTArithmeticFreeArray(model->frequencies, 
                            model->num_contexts);
}


static int QccENTArithmeticAllocModelArrays(QccENTArithmeticModel *model)
{
  if (model == NULL)
    return(0);
  if (model->num_symbols == NULL)
    return(0);

  if ((model->translate_symbol_to_index = 
       QccENTArithmeticAllocArray(model->num_symbols, model->num_contexts))
      == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticAllocModelArrays): Error calling QccENTArithmeticAllocArray()");
      QccENTArithmeticFreeModelArrays(model);
      return(0);
    }

  if ((model->translate_index_to_symbol = 
       QccENTArithmeticAllocArray(model->num_symbols, model->num_contexts))
      == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticAllocModelArrays): Error calling QccENTArithmeticAllocArray()");
      QccENTArithmeticFreeModelArrays(model);
      return(0);
    }

  if ((model->cumulative_frequencies =
       QccENTArithmeticAllocArray(model->num_symbols, model->num_contexts))
      == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticAllocModelArrays): Error calling QccENTArithmeticAllocArray()");
      QccENTArithmeticFreeModelArrays(model);
      return(0);
    }

  if ((model->frequencies = 
       QccENTArithmeticAllocArray(model->num_symbols, model->num_contexts))
      == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticAllocModelArrays): Error calling QccENTArithmeticAllocArray()");
      QccENTArithmeticFreeModelArrays(model);
      return(0);
    }

  return(0);
}


static QccENTArithmeticModel
*QccENTArithmeticCreateModel(const int *num_symbols,
                             int num_contexts,
                             QccENTArithmeticGetContext context_function)
{
  QccENTArithmeticModel *new_model = NULL;
  int symbol, context;

  if (num_symbols == NULL)
    return(NULL);

  if ((new_model = 
       (QccENTArithmeticModel *)malloc(sizeof(QccENTArithmeticModel))) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticCreateModel): Error allocating memory");
      goto QccError;
    }

  QccENTArithmeticModelInitialize(new_model);

  if ((new_model->num_symbols = 
       (int *)malloc(sizeof(int)*num_contexts)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticCreateModel): Error allocating memory");
      goto QccError;
    }
  /*  Save space for EOF symbol  */
  for (context = 0; context < num_contexts; context++)
    new_model->num_symbols[context] = num_symbols[context] + 1;
  new_model->num_contexts = num_contexts;
  
  if (QccENTArithmeticAllocModelArrays(new_model))
    {
      QccErrorAddMessage("(QccENTArithmeticCreateModel): Error calling QccENTArithmeticAllocModelArrays()");
      goto QccError;
    }
  
  for (context = 0; context < new_model->num_contexts; context++)
    {
      for (symbol = 0; symbol < new_model->num_symbols[context]; symbol++)
        {
          new_model->translate_symbol_to_index[context][symbol] = symbol + 1;
          new_model->translate_index_to_symbol[context][symbol + 1] =
            symbol;
        }

      if (QccENTArithmeticResetModel(new_model, context))
        {
          QccErrorAddMessage("(QccENTArithmeticCreateModel): Error calling QccENTArithmeticResetModel()");
          goto QccError;
        }
    }

  new_model->low = 0;
  new_model->high = QCCENT_TOPVALUE;
  new_model->bits_to_follow = 0;

  new_model->context_function = context_function;
  new_model->current_context = 0;

  return(new_model);

 QccError:
  if (new_model != NULL)
    QccENTArithmeticFreeModel(new_model);
  return(NULL);
}


static int QccENTArithmeticUpdateModel(QccENTArithmeticModel *model,
                                       int symbol_index, int context)
{
  int new_index;
  int cum = 0;
  int symbol1, symbol2;

  if (model == NULL)
    return(0);

  if (model->cumulative_frequencies[context][0] == QCCENT_MAXFREQUENCY)
    for (new_index = model->num_symbols[context]; new_index >= 0; new_index--)
      {
        model->frequencies[context][new_index] =
          (model->frequencies[context][new_index] + 1)/2;
        model->cumulative_frequencies[context][new_index] = cum;
        cum += model->frequencies[context][new_index];
      }

  for (new_index = symbol_index; 
       (model->frequencies[context][new_index] == 
        model->frequencies[context][new_index - 1]);
       new_index--);

  if (new_index < symbol_index)
    {
      symbol1 = model->translate_index_to_symbol[context][new_index];
      symbol2 = model->translate_index_to_symbol[context][symbol_index];
      model->translate_index_to_symbol[context][new_index] = symbol2;
      model->translate_index_to_symbol[context][symbol_index] = symbol1;
      model->translate_symbol_to_index[context][symbol1] = symbol_index;
      model->translate_symbol_to_index[context][symbol2] = new_index;
    }

  model->frequencies[context][new_index] += 1;

  while (new_index > 0)
    {
      new_index--;
      model->cumulative_frequencies[context][new_index] += 1;
    }

  return(0);
}


int QccENTArithmeticSetModelContext(QccENTArithmeticModel *model,
                                    int current_context)
{
  if ((current_context < 0) || (current_context >= model->num_contexts))
    {
      QccErrorAddMessage("(QccENTArithmeticSetModelContext): Invalid context number");
      return(1);
    }

  model->current_context = current_context;

  return(0);
}


void QccENTArithmeticSetModelAdaption(QccENTArithmeticModel *model,
                                      int adaptive)
{
  model->adaptive_model = adaptive;
}


int QccENTArithmeticSetModelProbabilities(QccENTArithmeticModel *model,
                                          const double *probabilities,
                                          int context)
{
  int symbol;
  int cum = 0;
  int freq;
  int max_freq;

  if (model == NULL)
    return(0);
  if (probabilities == NULL)
    return(0);
  
  if (model->adaptive_model == QCCENT_ADAPTIVE)
    {
      QccErrorAddMessage("(QccENTArithmeticSetModelProbabilities): Cannot manually set model probabilities in an adaptive model");
      return(1);
    }

  for (symbol = model->num_symbols[context] - 2; symbol >= 0; symbol--)
    if ((probabilities[symbol] < 0.0) ||
        (probabilities[symbol] > 1.0))
    {
      QccErrorAddMessage("(QccENTArithmeticSetModelProbabilities): Invalid probability encountered");
      return(1);
    }

  max_freq = QCCENT_MAXFREQUENCY - model->num_symbols[context] + 1;

  for (symbol = model->num_symbols[context] - 2; symbol >= 0; symbol--)
    {
      freq = (int)rint(QCCENT_MAXFREQUENCY * probabilities[symbol]);
      
      if (freq > max_freq)
        model->frequencies[context]
          [model->translate_symbol_to_index[context][symbol]] =
          max_freq;
      else
        if (freq <= 0)
          model->frequencies[context]
            [model->translate_symbol_to_index[context][symbol]] = 1;
        else
          model->frequencies[context]
            [model->translate_symbol_to_index[context][symbol]] = freq;
    }

  for (symbol = model->num_symbols[context]; symbol >= 0; symbol--)
    {
      model->cumulative_frequencies[context][symbol] = cum;
      cum += model->frequencies[context][symbol];
    }

  cum = 0;
  if (model->cumulative_frequencies[context][0] >= QCCENT_MAXFREQUENCY)
    for (symbol = model->num_symbols[context]; symbol >= 0; symbol--)
      {
        model->frequencies[context][symbol] =
          (model->frequencies[context][symbol] + 1)/2;
        model->cumulative_frequencies[context][symbol] = cum;
        cum += model->frequencies[context][symbol];
      }

  return(0);
}


int QccENTArithmeticResetModel(QccENTArithmeticModel *model,
                               int context)
{
  int symbol;
  
  for (symbol = 0; symbol <= model->num_symbols[context]; symbol++)
    {
      model->frequencies[context][symbol] = 1;
      model->cumulative_frequencies[context][symbol] =
        model->num_symbols[context] - symbol;
    }

  model->frequencies[context][0] = 0;
  
  return(0);
}


void QccENTArithmeticFreeModel(QccENTArithmeticModel *model)
{
  if (model == NULL)
    return;

  QccENTArithmeticFreeModelArrays(model);
  if (model->num_symbols != NULL)
    QccFree(model->num_symbols);    
  QccFree(model);

}


static int 
QccENTArithmeticOutputBitPlusFollowingOppositeBits(int bit_value, 
                                                   QccENTArithmeticModel 
                                                   *model,
                                                   QccBitBuffer *output_buffer)
{
  if (QccBitBufferPutBit(output_buffer, bit_value))
    {
      QccErrorAddMessage("(QccENTArithmeticOutputBitPlusFollowingOppositeBits): Error calling QccBitBufferPutBit()");
      return(1);
    }
  if (model->target_num_bits != QCCENT_ANYNUMBITS)
    if (output_buffer->bit_cnt >= model->target_num_bits)
      return(2);
  while (model->bits_to_follow > 0)
    {
      if (QccBitBufferPutBit(output_buffer, !bit_value))
        {
          QccErrorAddMessage("(QccENTArithmeticOutputBitPlusFollowingOppositeBits): Error calling QccBitBufferPutBit()");
          return(1);
        }
      if (model->target_num_bits != QCCENT_ANYNUMBITS)
        if (output_buffer->bit_cnt >= model->target_num_bits)
          return(2);
      model->bits_to_follow--;
    }

  return(0);
}


static int QccENTArithmeticEncodeSymbol(int symbol_index, int context,
                                        QccENTArithmeticModel *model, 
                                        QccBitBuffer *output_buffer)
{
  int return_value;
  long range;

  range = (long)(model->high - model->low) + 1;
  model->high = model->low +
    (range * model->cumulative_frequencies[context][symbol_index - 1]) /
    model->cumulative_frequencies[context][0] - 1;
  model->low = model->low +
    (range * model->cumulative_frequencies[context][symbol_index]) /
    model->cumulative_frequencies[context][0];

  for (;;)
    {
      if (model->high < QCCENT_HALFVALUE)
        {
          return_value =
            QccENTArithmeticOutputBitPlusFollowingOppositeBits(0, model,
                                                               output_buffer);
          if (return_value == 2)
            return(2);
          else
            if (return_value == 1)
              {
                QccErrorAddMessage("(QccENTArithmeticEncodeSymbol): Error calling QccENTArithmeticOutputBitPlusFollowingOppositeBits()");
                return(1);
              }
        }
      else if (model->low >= QCCENT_HALFVALUE)
        {
          return_value =
            QccENTArithmeticOutputBitPlusFollowingOppositeBits(1, model,
                                                               output_buffer);
          if (return_value == 2)
            return(2);
          else
            if (return_value == 1)
              {
                QccErrorAddMessage("(QccENTArithmeticEncodeSymbol): Error calling QccENTArithmeticOutputBitPlusFollowingOppositeBits()");
                return(1);
              }
          model->low -= QCCENT_HALFVALUE;
          model->high -= QCCENT_HALFVALUE;
        }
      else if ((model->low >= QCCENT_FIRSTQUARTERVALUE) &&
               (model->high < QCCENT_THIRDQUARTERVALUE))
        {
          model->bits_to_follow += 1;
          model->low -= QCCENT_FIRSTQUARTERVALUE;
          model->high -= QCCENT_FIRSTQUARTERVALUE;
        }
      else
        break;
      model->low *= 2;
      model->high = model->high*2 + 1;
    }

  return(0);
}


static int QccENTArithmeticEncodeEOFSymbol(int context,
                                           QccENTArithmeticModel *model, 
                                           QccBitBuffer *output_buffer)
{
  int return_value;
  int EOF_symbol_index;
  
  EOF_symbol_index = model->num_symbols[context];

  return_value =
    QccENTArithmeticEncodeSymbol(EOF_symbol_index,
                                 context,
                                 model, output_buffer);
  if (return_value == 2)
    return(2);
  else
    if (return_value == 1)
      {
        QccErrorAddMessage("(QccENTArithmeticEncodeEOFSymbol): Error calling QccENTArithmeticEncodeSymbol()");
        return(1);
      }
  
  return(0);
}


QccENTArithmeticModel *QccENTArithmeticEncodeStart(const int *num_symbols, 
                                                   int num_contexts,
                                                   QccENTArithmeticGetContext
                                                   context_function,
                                                   int target_num_bits)
{
  QccENTArithmeticModel *new_model = NULL;

  if ((new_model = 
       QccENTArithmeticCreateModel(num_symbols, num_contexts,
                                   context_function)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeStart): Error calling QccENTArithmeticCreateModel()");
      return(NULL);
    }

  new_model->target_num_bits = target_num_bits;

  return(new_model);
}


int QccENTArithmeticEncodeEnd(QccENTArithmeticModel *model,
                              int final_context,
                              QccBitBuffer *output_buffer)
{
  int return_value;

  if ((model == NULL) || (output_buffer == NULL))
    return(0);

  if (model->num_contexts <= 1)
    final_context = 0;

  return_value =
    QccENTArithmeticEncodeEOFSymbol(final_context, model, output_buffer);
  if (return_value == 2)
    return(2);
  else
    if (return_value == 1)
      {
        QccErrorAddMessage("(QccENTArithmeticEncodeEnd): Error calling QccENTArithmeticEncodeEOFSymbol()");
        return(1);
      }

  return_value = QccENTArithmeticEncodeFlush(model, output_buffer);
  if (return_value == 2)
    return(2);
  else
    if (return_value == 1)
      {
        QccErrorAddMessage("(QccENTArithmeticEncode): Error calling QccENTArithmeticEncodeFlush()");
        return(1);
      }
  
  return(0);
}


int QccENTArithmeticEncode(const int *symbol_stream,
                           int symbol_stream_length,
                           QccENTArithmeticModel *model,
                           QccBitBuffer *output_buffer)
{
  int return_value;
  int current_symbol;
  int symbol_index;

  if (symbol_stream == NULL)
    return(0);
  if (output_buffer == NULL)
    return(0);
  if (model == NULL)
    return(0);

  for (current_symbol = 0; current_symbol < symbol_stream_length;
       current_symbol++)
    {
      if (model->context_function != NULL)
        model->current_context =
          (model->context_function)(symbol_stream, current_symbol);

      if ((model->current_context < 0) ||
          (model->current_context >= model->num_contexts))
        {
          QccErrorAddMessage("(QccENTArithmeticEncode): Invalid context");
          return(1);
        }
      if ((symbol_stream[current_symbol] >=
           model->num_symbols[model->current_context] - 1) ||
          (symbol_stream[current_symbol] < 0))
        {
          QccErrorAddMessage("(QccENTArithmeticEncode): Invalid symbol in symbol stream");
          return(1);
        }
      symbol_index = 
        model->translate_symbol_to_index
        [model->current_context][symbol_stream[current_symbol]];
      return_value =
        QccENTArithmeticEncodeSymbol(symbol_index, model->current_context,
                                     model, output_buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccENTArithmeticEncode): Error calling QccENTArithmeticEncodeSymbol()");
            return(1);
          }
      if (model->adaptive_model == QCCENT_ADAPTIVE)
        if (QccENTArithmeticUpdateModel(model,
                                        symbol_index,
                                        model->current_context))
          {
            QccErrorAddMessage("(QccENTArithmeticEncode): Error calling QccENTArithmeticUpdateModel()");
            return(1);
          }
    }

  return(0);
}


int QccENTArithmeticEncodeFlush(QccENTArithmeticModel *model, 
                                QccBitBuffer *output_buffer)
{
  int return_value;

  if (model == NULL)
    return(0);

  model->bits_to_follow += 1;
  if (model->low < QCCENT_FIRSTQUARTERVALUE)
    {
      return_value =
        QccENTArithmeticOutputBitPlusFollowingOppositeBits(0, model, 
                                                           output_buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccENTArithmeticEncodeFlush): Error calling QccENTArithmeticOutputBitPlusFollowingOppositeBits()");
            return(1);
          }
    }
  else
    {
      return_value =
        QccENTArithmeticOutputBitPlusFollowingOppositeBits(1, model, 
                                                           output_buffer);
      if (return_value == 2)
        return(2);
      else
        if (return_value == 1)
          {
            QccErrorAddMessage("(QccENTArithmeticEncodeFlush): Error calling QccENTArithmeticOutputBitPlusFollowingOppositeBits()");
            return(1);
          }
    }

  if (QccBitBufferFlush(output_buffer))
    {
      QccErrorAddMessage("(QccENTArithmeticEncodeFlush): Error calling QccBitBufferFlush()");
      return(1);
    }

  model->low = 0;
  model->high = QCCENT_TOPVALUE;
  model->bits_to_follow = 0;

  return(0);
}


static int QccENTArithmeticDecodeGetBit(QccENTArithmeticModel *model,
                                        QccBitBuffer *input_buffer,
                                        int *bit_value)
{
  int beyond_bounds;
  
  if (model->target_num_bits == QCCENT_ANYNUMBITS)
    beyond_bounds = QccBitBufferGetBit(input_buffer, bit_value);
  else
    if (input_buffer->bit_cnt >= model->target_num_bits)
      beyond_bounds = 1;
    else
      beyond_bounds = QccBitBufferGetBit(input_buffer, bit_value);
  
  if (beyond_bounds)
    {
      QccErrorClearMessages();
      model->garbage_bits++;
      if (model->garbage_bits > QCCENT_CODEVALUEBITS - 2)
        return(1);

      *bit_value = 0;
    }

  return(0);
}


static int QccENTArithmeticDecodeSymbol(int *symbol_index, 
                                        int context,
                                        QccENTArithmeticModel *model, 
                                        QccBitBuffer *input_buffer)
{
  long range;
  int cumulative_frequency;
  int bit_value;

  range = (long)(model->high - model->low) + 1;
  cumulative_frequency = 
    (((long)(model->current_code_value - model->low) + 1) *
     model->cumulative_frequencies[context][0] - 1) / range;

  for (*symbol_index = 1;
       model->cumulative_frequencies[context][*symbol_index] > 
         cumulative_frequency;
       *symbol_index += 1) ;

  model->high = model->low +
    (range * model->cumulative_frequencies[context][*symbol_index - 1]) /
    model->cumulative_frequencies[context][0] - 1;
  model->low = model->low +
    (range * model->cumulative_frequencies[context][*symbol_index]) /
    model->cumulative_frequencies[context][0];

  for (;;)
    {
      if (model->high < QCCENT_HALFVALUE) ;
      else if (model->low >= QCCENT_HALFVALUE)
        {
          model->current_code_value -= QCCENT_HALFVALUE;
          model->low -= QCCENT_HALFVALUE;
          model->high -= QCCENT_HALFVALUE;
        }
      else if (model->low >= QCCENT_FIRSTQUARTERVALUE &&
               model->high < QCCENT_THIRDQUARTERVALUE)
        {
          model->current_code_value -= QCCENT_FIRSTQUARTERVALUE;
          model->low -= QCCENT_FIRSTQUARTERVALUE;
          model->high -= QCCENT_FIRSTQUARTERVALUE;
        }
      else
        break;

      model->low = 2 * model->low;
      model->high = 2 * model->high + 1;

      if (QccENTArithmeticDecodeGetBit(model, input_buffer, &bit_value))
        return(1);

      model->current_code_value = 2 * (model->current_code_value) +
        bit_value;

    }

  return(0);
}


static int QccENTArithmeticSymbolIsEOF(int symbol_index,
                                       int context,
                                       QccENTArithmeticModel *model)
{
  int EOF_symbol_index;

  EOF_symbol_index = model->num_symbols[context];

  return(symbol_index == EOF_symbol_index);
}


QccENTArithmeticModel *QccENTArithmeticDecodeStart(QccBitBuffer *input_buffer,
                                                   const int *num_symbols, 
                                                   int num_contexts,
                                                   QccENTArithmeticGetContext
                                                   context_function,
                                                   int target_num_bits)
{
  int bit_index;
  int bit_value;

  QccENTArithmeticModel *new_model = NULL;

  if ((new_model = 
       QccENTArithmeticCreateModel(num_symbols, num_contexts,
                                   context_function)) == NULL)
    {
      QccErrorAddMessage("(QccENTArithmeticDecodeStart): Error calling QccENTArithmeticCreateModel()");
      return(NULL);
    }

  new_model->current_code_value = 0;
  new_model->garbage_bits = 0;
  new_model->target_num_bits = target_num_bits;
  for (bit_index = 1; bit_index <= QCCENT_CODEVALUEBITS; bit_index++)
    {
      if (QccENTArithmeticDecodeGetBit(new_model, input_buffer, &bit_value))
        {
          QccErrorAddMessage("(QccENTArithmeticDecodeStart): Error calling QccENTArithmeticDecodeGetBit()");
          QccENTArithmeticFreeModel(new_model);
          return(NULL);
        }
      new_model->current_code_value = 2*(new_model->current_code_value) + 
        bit_value;
    }

  return(new_model);
}


int QccENTArithmeticDecodeRestart(QccBitBuffer *input_buffer,
                                  QccENTArithmeticModel *model,
                                  int target_num_bits)
{
  int bit_index;
  int bit_value;

  if (model == NULL)
    return(0);
  if (input_buffer == NULL)
    return(0);

  if (QccBitBufferFlush(input_buffer))
    {
      QccErrorAddMessage("(QccENTArithmeticDecodeRestart): Error calling QccBitBufferFlush()");
      return(1);
    }

  model->current_code_value = 0;
  model->garbage_bits = 0;
  model->target_num_bits = target_num_bits;
  model->low = 0;
  model->high = QCCENT_TOPVALUE;

  for (bit_index = 1; bit_index <= QCCENT_CODEVALUEBITS; bit_index++)
    {
      if (QccENTArithmeticDecodeGetBit(model, input_buffer, &bit_value))
        {
          QccErrorAddMessage("(QccENTArithmeticDecodeRestart): Error calling QccENTArithmeticDecodeGetBit()");
          return(1);
        }
      model->current_code_value = 2*(model->current_code_value) + 
        bit_value;
    }

  return(0);
}


int QccENTArithmeticDecode(QccBitBuffer *input_buffer,
                           QccENTArithmeticModel *model,
                           int *symbol_stream,
                           int symbol_stream_length)
{
  int current_symbol;
  int symbol_index;

  if (input_buffer == NULL)
    return(0);
  if (symbol_stream == NULL)
    return(0);
  if (model == NULL)
    return(0);

  for (current_symbol = 0; current_symbol < symbol_stream_length; 
       current_symbol++)
    {
      if (model->context_function != NULL)
        model->current_context =
          (model->context_function)(symbol_stream, current_symbol);

      if (QccENTArithmeticDecodeSymbol(&symbol_index, model->current_context,
                                       model, input_buffer))
        {
          QccErrorAddMessage("(QccENTArithmeticDecode): Error calling QccENTArithmeticDecodeSymbol()");
          return(1);
        }

      if (QccENTArithmeticSymbolIsEOF(symbol_index,
                                      model->current_context, model))
        return(2);

      symbol_stream[current_symbol] =
        model->translate_index_to_symbol[model->current_context][symbol_index];

      if (model->adaptive_model == QCCENT_ADAPTIVE)
        if (QccENTArithmeticUpdateModel(model,
                                        symbol_index, model->current_context))
          {
            QccErrorAddMessage("(QccENTArithmeticDecode): Error calling QccENTArithmeticUpdateModel()");
            return(1);
          }
    }

  return(0);
}
