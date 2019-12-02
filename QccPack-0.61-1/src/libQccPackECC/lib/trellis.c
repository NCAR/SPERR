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


void QccECCTrellisStateTableInitialize(QccECCTrellisStateTable *state_table)
{
  if (state_table == NULL)
    return;
  state_table->state_table = NULL;
  state_table->num_states = 0;
  state_table->num_input_symbols = 0;
  state_table->num_syndromes = 0;
}


int QccECCTrellisStateTableAlloc(QccECCTrellisStateTable *state_table)
{
  int return_value;
  int state;
  int input_symbol;
  int syndrome;

  if (state_table == NULL)
    return(0);
  if (state_table->num_states <= 0)
    return(0);
  if (state_table->num_input_symbols <= 0)
    return(0);
  if (state_table->num_syndromes <= 0)
    return(0);

  if ((state_table->state_table =
       (QccECCTrellisStateTableEntry ***)
       malloc(sizeof(QccECCTrellisStateTableEntry **) *
              state_table->num_states)) == NULL)
    {
      QccErrorAddMessage("(QccECCTrellisStateTableAlloc): Error allocating memory");
      goto Error;
    }

  for (state = 0; state < state_table->num_states; state++)
    {
      if ((state_table->state_table[state] =
           (QccECCTrellisStateTableEntry **)
           malloc(sizeof(QccECCTrellisStateTableEntry *) *
                  state_table->num_input_symbols)) == NULL)
        {
          QccErrorAddMessage("(QccECCTrellisStateTableAlloc): Error allocating memory");
          goto Error;
        }
      
      for (input_symbol = 0;
           input_symbol < state_table->num_input_symbols; input_symbol++)
        {
          if ((state_table->state_table[state][input_symbol] =
               (QccECCTrellisStateTableEntry *)
               malloc(sizeof(QccECCTrellisStateTableEntry) *
                      state_table->num_syndromes)) == NULL)
            {
              QccErrorAddMessage("(QccECCTrellisStateTableAlloc): Error allocating memory");
              goto Error;
            }
          
          for (syndrome = 0; syndrome < state_table->num_syndromes; syndrome++)
            state_table->state_table
              [state][input_symbol][syndrome].next_state = 0;
        }
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


void QccECCTrellisStateTableFree(QccECCTrellisStateTable *state_table)
{
  int state;
  int input_symbol;

  if (state_table == NULL)
    return;
  
  for (state = 0; state < state_table->num_states; state++)
    if (state_table->state_table[state] != NULL)
      {
        for (input_symbol = 0;
             input_symbol < state_table->num_input_symbols; input_symbol++)
          QccFree(state_table->state_table[state][input_symbol]);
        QccFree(state_table->state_table[state]);
      }
  QccFree(state_table->state_table);
}


void QccECCTrellisStateTablePrint(const QccECCTrellisStateTable *state_table)
{
  int state;
  int input;
  int syndrome;
  int state_field_width;
  int input_field_width;
  int syndrome_field_width;
  int num_outputs;
  int output_field_width;

  if (state_table == NULL)
    return;
  if (state_table->state_table == NULL)
    return;

  state_field_width =
    (int)floor(log((double)state_table->num_states)/log(8.0) + 1);
  input_field_width =
    (int)floor(log((double)state_table->num_input_symbols)/log(8.0) + 1);
  syndrome_field_width =
    (int)floor(log((double)state_table->num_syndromes)/log(8.0) + 1);

  num_outputs = 0;
  for (syndrome = 0; syndrome < state_table->num_syndromes; syndrome++)
    for (state = 0; state < state_table->num_states; state++)
      for (input = 0; input < state_table->num_input_symbols; input++)
        if (state_table->state_table[state][input][syndrome].output_symbol >
            num_outputs)
          num_outputs =
            state_table->state_table[state][input][syndrome].output_symbol;
        
  num_outputs++;

  output_field_width =
    (int)floor(log((double)num_outputs)/log(8.0) + 1);

  printf("       Number of states: %d\n",
         state_table->num_states);
  printf("Number of input symbols: %d\n",
         state_table->num_input_symbols);
  printf("    Number of syndromes: %d\n\n",
         state_table->num_syndromes);

  for (syndrome = 0; syndrome < state_table->num_syndromes; syndrome++)
    for (state = 0; state < state_table->num_states; state++)
      for (input = 0; input < state_table->num_input_symbols; input++)
        printf("(%0*d, %0*d, %0*d) -> (%0*d, %0*d)\n",
               state_field_width,
               state,
               input_field_width,
               input,
               syndrome_field_width,
               syndrome,
               state_field_width,
               state_table->state_table[state][input][syndrome].next_state,
               output_field_width,
               state_table->state_table[state][input][syndrome].output_symbol);
}
