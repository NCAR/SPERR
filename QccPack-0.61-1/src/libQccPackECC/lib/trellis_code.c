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


void QccECCTrellisCodeInitialize(QccECCTrellisCode *trellis_code)
{
  QccStringMakeNull(trellis_code->filename);
  QccStringCopy(trellis_code->magic_num, QCCECCTRELLISCODE_MAGICNUM);
  QccGetQccPackVersion(&trellis_code->major_version,
                       &trellis_code->minor_version,
                       NULL);
  trellis_code->memory_order = 0;
  trellis_code->num_inputs = 0;
  trellis_code->num_outputs = 0;
  trellis_code->parity_check_matrix = NULL;
}


int QccECCTrellisCodeAlloc(QccECCTrellisCode *trellis_code)
{
  if (trellis_code == NULL)
    return(0);
  
  if ((trellis_code->memory_order <= 0) ||
      (trellis_code->num_inputs <= 0) ||
      (trellis_code->num_outputs <= 0))
    {
      QccErrorAddMessage("(QccECCTrellisCodeAlloc): Invalid code parameters");
      return(1);
    }
  
  if ((trellis_code->parity_check_matrix =
       (unsigned int *)malloc(sizeof(unsigned int) *
                               trellis_code->num_outputs)) == NULL)
    {
      QccErrorAddMessage("(QccECCTrellisCodeAlloc): Error allocating memory");
      return(1);
    }
  
  return(0);
}


void QccECCTrellisCodeFree(QccECCTrellisCode *trellis_code)
{
  if (trellis_code == NULL)
    return;
  QccFree(trellis_code->parity_check_matrix);
}


static int QccECCTrellisCodeReadHeader(FILE *infile, 
                                       QccECCTrellisCode *trellis_code)
{
  
  if ((infile == NULL) || (trellis_code == NULL))
    return(0);
  
  if (QccFileReadMagicNumber(infile,
                             trellis_code->magic_num,
                             &trellis_code->major_version,
                             &trellis_code->minor_version))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Error reading magic number in trellis code %s",
                         trellis_code->filename);
      return(1);
    }
  
  if (strcmp(trellis_code->magic_num,
             QCCECCTRELLISCODE_MAGICNUM))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): %s is not of trellis code (%s) type",
                         trellis_code->filename,
                         QCCECCTRELLISCODE_MAGICNUM);
      return(1);
    }
  
  fscanf(infile, "%d", &(trellis_code->memory_order));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Error reading memory_order in trellis code %s",
                         trellis_code->filename);
      return(1);
    }
  
  if ((trellis_code->memory_order <= 0) ||
      (trellis_code->memory_order > 31))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Invalid memory_order (%d) in trellis code %s",
                         trellis_code->memory_order,
                         trellis_code->filename);
      return(1);
    }

  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Error reading memory_order in trellis code %s",
                         trellis_code->filename);
      return(1);
    }
  
  fscanf(infile, "%d", &(trellis_code->num_inputs));
  if (ferror(infile) || feof(infile))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Error reading num_inputs in trellis code %s",
                         trellis_code->filename);
      return(1);
    }
  
  if (trellis_code->num_inputs <= 0)
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Invalid num_inputs (%d) in trellis code %s",
                         trellis_code->num_inputs,
                         trellis_code->filename);
      return(1);
    }

  trellis_code->num_outputs = trellis_code->num_inputs + 1;

  if (QccFileSkipWhiteSpace(infile, 0))
    {
      QccErrorAddMessage("(QccECCTrellisCodeReadHeader): Error reading num_inputs in trellis code %s",
                         trellis_code->filename);
      return(1);
    }
  
  return(0);
}


static int QccECCTrellisCodeReadData(FILE *infile, 
                                     QccECCTrellisCode *trellis_code)
{
  int output;

  if ((infile == NULL) || (trellis_code == NULL))
    return(0);
  
  for (output = 0; output < trellis_code->num_outputs; 
       output++)
    {
      fscanf(infile, "%o",
             &(trellis_code->parity_check_matrix[output]));
      if (ferror(infile) || feof(infile))
        {
          QccErrorAddMessage("(QccECCTrellisCodeReadData): Error reading data from %s",
                             trellis_code->filename);
          return(1);
        }
    }
  
  return(0);
}


int QccECCTrellisCodeRead(QccECCTrellisCode *trellis_code)
{
  FILE *infile = NULL;
  
  if (trellis_code == NULL)
    return(0);
  
  if ((infile = 
       QccFilePathSearchOpenRead(trellis_code->filename,
                                 QCCECCTRELLISCODE_PATH_ENV,
                                 QCCMAKESTRING(QCCPACK_CODES_PATH_DEFAULT)))
      == NULL)
    {
      QccErrorAddMessage("(QccECCTrellisCodeRead): Error calling QccFilePathSearchOpenRead()");
      return(1);
    }

  if (QccECCTrellisCodeReadHeader(infile, trellis_code))
    {
      QccErrorAddMessage("(QccECCTrellisCodeRead): Error calling QccECCTrellisCodeReadHeader()");
      return(1);
    }
  
  if (QccECCTrellisCodeAlloc(trellis_code))
    {
      QccErrorAddMessage("(QccECCTrellisCodeRead): Error calling QccECCTrellisCodeAlloc()");
      return(1);
    }
  
  if (QccECCTrellisCodeReadData(infile, trellis_code))
    {
      QccErrorAddMessage("(QccECCTrellisCodeRead): Error calling QccECCTrellisCodeReadData()");
      return(1);
    }
  
  QccFileClose(infile);
  return(0);
}


static int QccECCTrellisCodeWriteHeader(FILE *outfile, 
                                        const QccECCTrellisCode *trellis_code)
{
  if ((outfile == NULL) || (trellis_code == NULL))
    return(0);
  
  if (QccFileWriteMagicNumber(outfile, QCCECCTRELLISCODE_MAGICNUM))
    goto QccErr;
  
  fprintf(outfile, "%d\n%d\n",
          trellis_code->memory_order,
          trellis_code->num_inputs);
  
  if (ferror(outfile))
    goto QccErr;
  
  return(0);
  
 QccErr:
  QccErrorAddMessage("(QccECCTrellisCodeWriteHeader): Error writing header to %s",
                     trellis_code->filename);
  return(1);
}


static int QccECCTrellisCodeWriteData(FILE *outfile,
                                      const QccECCTrellisCode *trellis_code)
{
  int output;

  if ((outfile == NULL) || (trellis_code == NULL))
    return(0);
  
  for (output = 0; output < trellis_code->num_outputs; output++)
    fprintf(outfile, "%o\n",
            trellis_code->parity_check_matrix[output]);

  if (ferror(outfile))
    {
      QccErrorAddMessage("(QccECCTrellisCodeWriteData): Error writing data to %s",
                         trellis_code->filename);
      return(1);
    }
  
  return(0);
}


int QccECCTrellisCodeWrite(const QccECCTrellisCode *trellis_code)
{
  FILE *outfile;
  
  if (trellis_code == NULL)
    return(0);
  
  if ((outfile = QccFileOpen(trellis_code->filename, "w")) == NULL)
    {
      QccErrorAddMessage("(QccECCTrellisCodeWrite): Error calling QccFileOpen()");
      return(1);
    }
  
  if (QccECCTrellisCodeWriteHeader(outfile, trellis_code))
    {
      QccErrorAddMessage("(QccECCTrellisCodeWrite): Error calling QccECCTrellisCodeWriteHeader()");
      return(1);
    }
  
  if (QccECCTrellisCodeWriteData(outfile, trellis_code))
    {
      QccErrorAddMessage("(QccECCTrellisCodeWrite): Error calling QccECCTrellisCodeWriteData()");
      return(1);
    }
  
  QccFileClose(outfile);
  return(0);
}


int QccECCTrellisCodePrint(const QccECCTrellisCode *trellis_code)
{
  int output;
  
  if (trellis_code == NULL)
    return(0);
  
  if (QccFilePrintFileInfo(trellis_code->filename,
                           trellis_code->magic_num,
                           trellis_code->major_version,
                           trellis_code->minor_version))
    return(1);
  
  printf("Trellis Code:\n");
  printf("  Memory order: %d\n",
         trellis_code->memory_order);
  printf("  Memory number of inputs: %d\n",
         trellis_code->num_inputs);
  printf("  Memory number of outputs: %d\n\n",
         trellis_code->num_outputs);
  printf("  Parity-Check Matrix: ");
  for (output = 0; output < trellis_code->num_outputs; output++)
    printf("    %o\n", 
           trellis_code->parity_check_matrix[output]);
  
  return(0);
}


int QccECCTrellisCodeHammingWeight(unsigned int symbol)
{
  int weight = 0;

  while (symbol)
    {
      if (symbol & 1)
        weight++;
      symbol >>= 1;
    }

  return(weight);
}


unsigned int QccECCTrellisCodeGetOutput(unsigned int input_symbol,
                                        unsigned int syndrome,
                                        unsigned int current_state,
                                        const QccECCTrellisCode *trellis_code)
{
  unsigned int output_symbol = 0;

  output_symbol = (input_symbol << 1) | ((current_state ^ syndrome) & 1);

  return(output_symbol);
}


unsigned int QccECCTrellisCodeGetNextState(unsigned int input_symbol,
                                           unsigned int current_state,
                                           const QccECCTrellisCode
                                           *trellis_code)
{
  unsigned int next_state = current_state;
  int input;
  unsigned int mask;
  int input_bit;
  int feedback_bit;

  feedback_bit = next_state & 1;

  for (input = 0; input < trellis_code->num_inputs; input++)
    {
      mask = (1 << input);
      input_bit = (input_symbol & mask) ? 1 : 0;
      next_state ^=
        input_bit * trellis_code->parity_check_matrix[input + 1];
    }
  
  next_state ^= feedback_bit * trellis_code->parity_check_matrix[0];
  
  return(next_state >> 1);
}  


int QccECCTrellisCodeCreateStateTable(const QccECCTrellisCode *trellis_code,
                                      QccECCTrellisStateTable *state_table)
{
  int return_value;
  int state;
  int input_symbol;
  int syndrome;

  if (trellis_code == NULL)
    return(0);
  if (state_table == NULL)
    return(0);

  state_table->num_states = (1 << trellis_code->memory_order);
  state_table->num_input_symbols = (1 << trellis_code->num_inputs);
  state_table->num_syndromes =
    (1 << (trellis_code->num_outputs - trellis_code->num_inputs));

  if (QccECCTrellisStateTableAlloc(state_table))
    {
      QccErrorAddMessage("(QccECCTrellisCodeCreateStateTable): Error calling QccECCTrellisStateTableAlloc()");
      goto Error;
    }

  for (state = 0; state < state_table->num_states; state++)
    for (input_symbol = 0;
         input_symbol < state_table->num_input_symbols; input_symbol++)
      for (syndrome = 0; syndrome < state_table->num_syndromes; syndrome++)
        {
          state_table->state_table[state][input_symbol][syndrome].next_state =
            QccECCTrellisCodeGetNextState((unsigned int)input_symbol,
                                          (unsigned int)state,
                                          trellis_code);
          state_table->state_table
            [state][input_symbol][syndrome].output_symbol =
            QccECCTrellisCodeGetOutput((unsigned int)input_symbol,
                                       (unsigned int)syndrome,
                                       (unsigned int)state,
                                       trellis_code);
        }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccECCTrellisCodeEncode(const unsigned int *message,
                            int message_length,
                            unsigned int *coded_message,
                            const QccECCTrellisCode *trellis_code)
{
  unsigned int current_state = 0;
  int symbol;
  unsigned int current_symbol;

  for (symbol = 0;
       symbol < message_length + trellis_code->memory_order;
       symbol++)
    {
      current_symbol = (symbol < message_length) ?
        message[symbol] : 0;
      coded_message[symbol] =
        QccECCTrellisCodeGetOutput(current_symbol,
                                   0,
                                   current_state,
                                   trellis_code);

      current_state =
        QccECCTrellisCodeGetNextState(current_symbol,
                                      current_state,
                                      trellis_code);
    }
  
  return(0);
}


int QccECCTrellisCodeSyndrome(const unsigned int *coded_message,
                              int coded_message_length,
                              unsigned int *syndrome,
                              const QccECCTrellisCode *trellis_code)
{
  int symbol;
  unsigned int current_symbol;
  unsigned int current_syndrome = 0;
  int output;
  unsigned int mask;
  int output_bit;

  for (symbol = 0; symbol < coded_message_length; symbol++)
    {
      current_symbol = coded_message[symbol];
      for (output = 0; output < trellis_code->num_outputs; output++)
        {
          mask = (1 << output);
          output_bit = (current_symbol & mask) ? 1 : 0;
          current_syndrome ^=
            output_bit * trellis_code->parity_check_matrix[output];
        }
      syndrome[symbol] = current_syndrome & 1;
      current_syndrome ^=
        trellis_code->parity_check_matrix[0] * syndrome[symbol];
      current_syndrome >>= 1;
    }
  
  return(0);
}
