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


#define QCCECCVITERBI_INVALIDSTATE -1

typedef struct
{
  unsigned int previous_state;
  unsigned int output_symbol;
  double accumulated_error;
} QccECCViterbiStateHistoryEntry;
typedef QccECCViterbiStateHistoryEntry **QccECCViterbiStateHistory;


double QccECCViterbiSquaredErrorCallback(unsigned int output_symbol,
                                         double modulated_symbol,
                                         int index,
                                         const void *callback_data)
{
  double *channel_symbol_table;
  double difference;

  channel_symbol_table = (double *)callback_data;
  difference = channel_symbol_table[output_symbol] - modulated_symbol;

  return(difference * difference);
}


static QccECCViterbiStateHistory
QccECCViterbiStateHistoryAlloc(int num_states,
                               int traceback_depth)
{
  QccECCViterbiStateHistory state_history = NULL;
  int stage;

  if ((state_history =
       (QccECCViterbiStateHistory)
       malloc(sizeof(QccECCViterbiStateHistoryEntry *) * traceback_depth)) ==
      NULL)
    {
      QccErrorAddMessage("(QccECCViterbiStateHistoryAlloc): Error allocating memory");
      goto Error;
    }

  for (stage = 0; stage < traceback_depth; stage++)
    if ((state_history[stage] =
         (QccECCViterbiStateHistoryEntry *)
         malloc(sizeof(QccECCViterbiStateHistoryEntry) * num_states)) == NULL)
      {
        QccErrorAddMessage("(QccECCViterbiStateHistoryAlloc): Error allocating memory");
        goto Error;
      }
  
  return(state_history);
 Error:
  return(NULL);
}


static void QccECCViterbiStateHistoryFree(QccECCViterbiStateHistory
                                          state_history,
                                          int traceback_depth)
{
  int stage;
  
  if (state_history == NULL)
    return;
  
  for (stage = 0; stage < traceback_depth; stage++)
    QccFree(state_history[stage]);
  QccFree(state_history);
}


static int QccECCViterbiDecodeProcessState(int stage,
                                           int state,
                                           int index,
                                           unsigned int received_symbol,
                                           double modulated_symbol,
                                           QccECCViterbiErrorMetricCallback
                                           callback,
                                           const void *callback_data,
                                           unsigned int syndrome,
                                           const QccECCTrellisStateTable
                                           *state_table,
                                           const QccECCViterbiStateHistory
                                           state_history,
                                           int hard_coded)
{
  int input_symbol;
  unsigned int next_state;
  unsigned int output_symbol;
  double error;
  double accumulated_error;

  for (input_symbol = 0; input_symbol < state_table->num_input_symbols;
       input_symbol++)
    {
      accumulated_error = state_history[stage][state].accumulated_error;
      if (accumulated_error != QCCECCVITERBI_INVALIDSTATE)
        {
          next_state =
            state_table->state_table
            [state][input_symbol][syndrome].next_state;
          output_symbol =
            state_table->state_table
            [state][input_symbol][syndrome].output_symbol;

          if (hard_coded)
            error =
              QccECCTrellisCodeHammingWeight(output_symbol ^ received_symbol);
          else
            error = callback(output_symbol,
                             modulated_symbol,
                             index,
                             callback_data);

          accumulated_error += error;

          if ((state_history[stage + 1][next_state].accumulated_error ==
               QCCECCVITERBI_INVALIDSTATE) ||
              (accumulated_error <
               state_history[stage + 1][next_state].accumulated_error))
            {
              state_history[stage + 1][next_state].previous_state = state;
              state_history[stage + 1][next_state].output_symbol =
                output_symbol;
              state_history[stage + 1][next_state].accumulated_error =
                accumulated_error;
            }
        }
    }

  return(0);
}  


static int QccECCViterbiDecodeTraceback(unsigned int *coded_message,
                                        int stage,
                                        int received_symbol,
                                        unsigned int *initial_state,
                                        int num_states,
                                        const QccECCViterbiStateHistory
                                        state_history,
                                        double *partial_path_error)
{
  int state;
  double min_error;
  unsigned int winner = 0;

  min_error = MAXDOUBLE;
  for (state = 0; state < num_states; state++)
    {
      if ((state_history[stage][state].accumulated_error !=
           QCCECCVITERBI_INVALIDSTATE) &&
          (state_history[stage][state].accumulated_error < min_error))
        {
          min_error =
            state_history[stage][state].accumulated_error;
          winner = state;
        }
    }
  
  if (partial_path_error != NULL)
    *partial_path_error = min_error;

  *initial_state = winner;

  while (stage > 0)
    {
      coded_message[received_symbol] =
        state_history[stage][winner].output_symbol;
      winner = state_history[stage][winner].previous_state;
      received_symbol--;
      stage--;
    }

  return(0);
}


int QccECCViterbiDecode(unsigned int *coded_message,
                        int coded_message_length,
                        const double *modulated_message,
                        QccECCViterbiErrorMetricCallback callback,
                        const void *callback_data,
                        const unsigned int *syndrome,
                        const QccECCTrellisStateTable *state_table,
                        int traceback_depth,
                        double *distance)
{
  int return_value;
  QccECCViterbiStateHistory state_history = NULL;
  int stage;
  int state;
  int received_symbol;
  double path_error;
  double partial_path_error;
  int hard_coded;
  unsigned int initial_state;

  if (coded_message == NULL)
    return(0);
  if (state_table == NULL)
    return(0);

  if ((state_history =
       QccECCViterbiStateHistoryAlloc(state_table->num_states,
                                      traceback_depth)) == NULL)
    {
      QccErrorAddMessage("(QccECCViterbiDecode): Error calling QccECCViterbiStateHistoryAlloc()");
      goto Error;
    }
  
  hard_coded = (modulated_message == NULL) || (callback == NULL);

  received_symbol = 0;
  initial_state = 0;

  path_error = 0;

  while (received_symbol < coded_message_length)
    {
      for (stage = 0; stage < traceback_depth; stage++)
        for (state = 0; state < state_table->num_states; state++)
          state_history[stage][state].accumulated_error = 
            QCCECCVITERBI_INVALIDSTATE;
      state_history[0][initial_state].accumulated_error = 0;
      
      for (stage = 0;
           (stage < traceback_depth - 1) &&
             (received_symbol < coded_message_length);
           stage++, received_symbol++)
        for (state = 0; state < state_table->num_states; state++)
          if (QccECCViterbiDecodeProcessState(stage,
                                              state,
                                              received_symbol,
                                              coded_message
                                              [received_symbol],
                                              (!hard_coded) ?
                                              modulated_message
                                              [received_symbol] : 0.0,
                                              callback,
                                              callback_data,
                                              (syndrome != NULL) ?
                                              syndrome[received_symbol] :
                                              0,
                                              state_table,
                                              state_history,
                                              hard_coded))
            {
              QccErrorAddMessage("(QccECCViterbiDecode): Error calling QccECCViterbiDecodeProcessState()");
              goto Error;
            }
      
      if (QccECCViterbiDecodeTraceback(coded_message,
                                       stage,
                                       received_symbol - 1,
                                       &initial_state,
                                       state_table->num_states,
                                       state_history,
                                       &partial_path_error))
        {
          QccErrorAddMessage("(QccECCViterbiDecode): Error calling QccECCViterbiDecodeTraceback()");
          goto Error;
        }

      path_error += partial_path_error;
    }
  
  if (distance != NULL)
    *distance = path_error;

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  QccECCViterbiStateHistoryFree(state_history, traceback_depth);
  return(return_value);
}
