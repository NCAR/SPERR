/*
 *
 * QccPack: Quantization, compression, and coding utilities
 * Copyright (C) 1997-2016  James E. Fowler
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#include "msvqtrain.h"

#define USG_STRING "[-v %:] [-i %: %s:initialcodebookfile] [-n %: %d:num_stages] [-s %: %d:codebooksize] [-I %: %d:iterations] [-t %: %f:threshold] %s:trainingfile %s:codebookfile"


int initialcodebook_specified = 0;
int numcodebooks_specified = 0;
int codebooksize_specified = 0;
int iterations_specified = 0;
int threshold_specified = 0;

QccVQMultiStageCodebook MultiStageCodebook;
int NumStages;
int CodebookSize;
int Iterations;
float Threshold;

QccDataset TrainingData;

QccString FinalCodebookFilename;

int Verbose = 0;

int main(int argc, char *argv[])
{
  int stage;
  
  QccInit(argc, argv);
  
  QccDatasetInitialize(&TrainingData);
  QccVQMultiStageCodebookInitialize(&MultiStageCodebook);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &Verbose,
                         &initialcodebook_specified,
                         MultiStageCodebook.filename,
                         &numcodebooks_specified,
                         &NumStages,
                         &codebooksize_specified,
                         &CodebookSize,
                         &iterations_specified,
                         &Iterations,
                         &threshold_specified,
                         &Threshold,
                         TrainingData.filename, 
                         FinalCodebookFilename))
    QccErrorExit();
  
  if ((initialcodebook_specified && codebooksize_specified) ||
      (!initialcodebook_specified && !codebooksize_specified))
    {
      QccErrorAddMessage("%s: Either -i or -s must be specified (but not both)\n",
                         argv[0]);
      QccErrorExit();
    }
  
  if ((iterations_specified && threshold_specified) ||
      (!iterations_specified && !threshold_specified))
    {
      QccErrorAddMessage("%s: Either -I or -t must be specified (but not both)\n",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccDatasetReadWholefile(&TrainingData))
    {
      QccErrorAddMessage("%s: Error calling QccDatasetReadWholefile()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (initialcodebook_specified)
    {
      if (QccVQMultiStageCodebookRead(&MultiStageCodebook))
        {
          QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookRead()",
                             argv[0]);
          QccErrorExit();
        }
      for (stage = 0; stage < MultiStageCodebook.num_codebooks; stage++)
        if (MultiStageCodebook.codebooks[stage].codeword_dimension !=
            TrainingData.vector_dimension)
          {
            QccErrorAddMessage("%s: Vector dimensions all codebooks must be same as that of dataset\n",
                               argv[0]);
            QccErrorExit();
          }
    }
  else
    {
      MultiStageCodebook.num_codebooks = NumStages;
      if (QccVQMultiStageCodebookAllocCodebooks(&MultiStageCodebook))
        {
          QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookAllocCodebooks()",
                             argv[0]);
          QccErrorExit();
        }
      
      for (stage = 0; stage < MultiStageCodebook.num_codebooks; stage++)
        {
          MultiStageCodebook.codebooks[stage].codeword_dimension =
            TrainingData.vector_dimension;
          MultiStageCodebook.codebooks[stage].num_codewords = CodebookSize;
          QccVQCodebookSetIndexLength(&MultiStageCodebook.codebooks[stage]);
          if (QccVQCodebookCreateRandomCodebook(&MultiStageCodebook.codebooks
                                                [stage],
                                                TrainingData.max_val,
                                                TrainingData.min_val))
            {
              QccErrorAddMessage("%s: Error calling QccVQCodebookCreateRandomCodebook()",
                                 argv[0]);
              QccErrorExit();
            }
        }
    }
  
  if (QccVQMultiStageVQTraining(&TrainingData,
                                &MultiStageCodebook,
                                ((iterations_specified) ? Iterations : 0),
                                ((iterations_specified) ? 0.0 : Threshold),
                                NULL, NULL, Verbose))
    {
      QccErrorAddMessage("%s: Error calling QccVQMultiStageVQTraining()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccStringCopy(MultiStageCodebook.filename, FinalCodebookFilename);
  
  if (QccVQMultiStageCodebookWrite(&MultiStageCodebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQMultiStageCodebookWrite()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccDatasetFree(&TrainingData);
  QccVQMultiStageCodebookFreeCodebooks(&MultiStageCodebook);
  
  QccExit;
}
