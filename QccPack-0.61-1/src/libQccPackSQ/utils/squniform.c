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


#include "squniform.h"

#define USG_STRING "[-max %f:max_value] [-min %f:min_value] [-u %: %f:u] [-A %: %f:A] [-nl %d:num_levels] [-q %f:stepsize] [-d %f:deadzone] %s:quantizer_file"

QccSQScalarQuantizer Quantizer;

float MaxValue = 255;
float MinValue = 0;

int ULaw = 0;
float U;

int ALaw = 0;
float A;

int NumLevels = 255;

float StepSize = -1;
float DeadZone = -1;

int main(int argc, char *argv[])
{

  QccInit(argc, argv);

  QccSQScalarQuantizerInitialize(&Quantizer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &MaxValue,
                         &MinValue,
                         &ULaw,
                         &U,
                         &ALaw,
                         &A,
                         &NumLevels,
                         &StepSize,
                         &DeadZone,
                         Quantizer.filename))
    QccErrorExit();
      
  if ((DeadZone > 0) && (StepSize <= 0))
    {
      QccErrorAddMessage("%s: Must define stepsize > 0 for dead-zone quantizer",
                         argv[0]);
      QccErrorExit();
    }

  if ((ALaw || ULaw) && (StepSize > 0))
    {
      QccErrorAddMessage("%s: Cannot specify both companding and uniform-quantizer stepsize",
                         argv[0]);
      QccErrorExit();
    }

  if (StepSize > 0)
    {
      if (DeadZone >= 0)
        {
          Quantizer.type = QCCSQSCALARQUANTIZER_DEADZONE;
          Quantizer.deadzone = DeadZone;
          Quantizer.stepsize = StepSize;
        }
      else
        {
          Quantizer.type = QCCSQSCALARQUANTIZER_UNIFORM;
          Quantizer.stepsize = StepSize;
        }
    }
  else
    {
      Quantizer.type = QCCSQSCALARQUANTIZER_GENERAL;

      if (ULaw && ALaw)
        {
          QccErrorAddMessage("%s: Cannot specify both u-Law and A-Law companding",
                             argv[0]);
          QccErrorExit();
        }
      
      if (NumLevels < 1)
        {
          QccErrorAddMessage("%s: Number of levels must be 1 or greater",
                             argv[0]);
          QccErrorExit();
        }
      Quantizer.num_levels = NumLevels;
      
      if (MaxValue <= MinValue)
        {
          QccErrorAddMessage("%s: MaxValue must be greater than MinValue",
                             argv[0]);
          QccErrorExit();
        }
      
      if (ULaw)
        {
          if (QccSQULawMakeQuantizer(&Quantizer,
                                     (double)U,
                                     (double)MaxValue, (double)MinValue))
            {
              QccErrorAddMessage("%s: Error calling QccSQULawMakeQuantizer()",
                                 argv[0]);
              QccErrorExit();
            }
        }
      else if (ALaw)
        {
          if (QccSQALawMakeQuantizer(&Quantizer,
                                     (double)A,
                                     (double)MaxValue, (double)MinValue))
            {
              QccErrorAddMessage("%s: Error calling QccSQALawMakeQuantizer()",
                                 argv[0]);
              QccErrorExit();
            }
        }
      else
        {
          if (QccSQUniformMakeQuantizer(&Quantizer,
                                        (double)MaxValue, (double)MinValue,
                                        QCCSQ_NOOVERLOAD))
            {
              QccErrorAddMessage("%s: Error calling QccSQUniformMakeQuantizer()",
                                 argv[0]);
              QccErrorExit();
            }
        }
    }

  if (QccSQScalarQuantizerWrite(&Quantizer))
    {
      QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccSQScalarQuantizerFree(&Quantizer);

  QccExit;
}
