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


#include "sqlloyd.h"

#define USG_STRING "[-max %f:max_value] [-min %f:min_value] [-nl %d:num_levels] [-v %f:variance] [-l %:] [-t %f:stop_threshold] [-i %d:integration_intervals] %s:quantizer_file"

QccSQScalarQuantizer Quantizer;

float MaxValue = 10;
float MinValue = -10;

int NumLevels = 255;

float Variance = 1.0;

float Threshold = 0.001;
int NumIntegrationIntervals = 100;

int Laplace = 0;

double (*ProbDensity)();

int main(int argc, char *argv[])
{

  QccInit(argc, argv);

  QccSQScalarQuantizerInitialize(&Quantizer);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &MaxValue,
                         &MinValue,
                         &NumLevels,
                         &Variance,
                         &Laplace,
                         &Threshold,
                         &NumIntegrationIntervals,
                         Quantizer.filename))
    QccErrorExit();
      
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

  if (NumIntegrationIntervals < 10)
    {
      QccErrorAddMessage("%s: NumIntegrationIntervals must be 10 or greater",
                         argv[0]);
      QccErrorExit();
    }

  if (Laplace)
    ProbDensity = QccMathLaplacianDensity;
  else
    ProbDensity = QccMathGaussianDensity;

  if (QccSQLloydMakeQuantizer(&Quantizer,
                              ProbDensity,
                              (double)Variance,
                              (double)(MaxValue + MinValue)/2,
                              (double)MaxValue,
                              (double)MinValue,
                              (double)Threshold,
                              NumIntegrationIntervals))
    {
      QccErrorAddMessage("%s: Error calling QccSQLloydMakeQuantizer()",
                         argv[0]);
      QccErrorExit();
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
