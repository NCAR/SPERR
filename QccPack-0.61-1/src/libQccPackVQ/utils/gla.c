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


#include "gla.h"

#define USG_STRING "[-v %:] [-i %: %s:initialcodebookfile] [-s %: %d:codebooksize] [-I %: %d:iterations] [-t %: %f:threshold] %s:trainingfile %s:codebookfile"


int initialcodebook_specified = 0;
int codebooksize_specified = 0;
int iterations_specified = 0;
int threshold_specified = 0;

QccVQCodebook Codebook;
int CodebookSize;
int Iterations;
float Threshold;

QccDataset TrainingData;

QccString FinalCodebookFilename;

int Verbose = 0;

int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccDatasetInitialize(&TrainingData);
  QccVQCodebookInitialize(&Codebook);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &Verbose,
                         &initialcodebook_specified,
                         Codebook.filename,
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
      if (QccVQCodebookRead(&Codebook))
        {
          QccErrorAddMessage("%s: Error calling QccVQCodebookRead()",
                             argv[0]);
          QccErrorExit();
        }
      if (Codebook.codeword_dimension !=
          TrainingData.vector_dimension)
        {
          QccErrorAddMessage("%s: Vector dimensions of %s and %s must be the same\n",
                             argv[0], Codebook.filename, TrainingData.filename);
          QccErrorExit();
        }
    }
  else
    {
      Codebook.codeword_dimension = TrainingData.vector_dimension;
      Codebook.num_codewords = CodebookSize;
      QccVQCodebookSetIndexLength(&Codebook);
      if (QccVQCodebookCreateRandomCodebook(&Codebook,
                                            TrainingData.max_val,
                                            TrainingData.min_val))
        {
          QccErrorAddMessage("%s: Error calling QccVQCodebookCreateRandomCodebook()",
                             argv[0]);
          QccErrorExit();
        }
      
    }
  
  if (iterations_specified)
    {
      if (QccVQGeneralizedLloydVQTraining(&TrainingData,
                                          &Codebook,
                                          Iterations, (double)0.0,
                                          NULL, NULL, Verbose))
        {
          QccErrorAddMessage("%s: Error calling QccVQGeneralizedLloydVQTraining()",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      if (QccVQGeneralizedLloydVQTraining(&TrainingData,
                                          &Codebook,
                                          (int)0, Threshold,
                                          NULL, NULL, Verbose))
        {
          QccErrorAddMessage("%s: Error calling QccVQGeneralizedLloydVQTraining()",
                             argv[0]);
          QccErrorExit();
        }
    }
  
  QccStringCopy(Codebook.filename, FinalCodebookFilename);
  
  if (QccVQCodebookWrite(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccDatasetFree(&TrainingData);
  QccVQCodebookFree(&Codebook);

  QccExit;
}
