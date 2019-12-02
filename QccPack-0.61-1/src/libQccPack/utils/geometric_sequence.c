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


#include "geometric_sequence.h"

#define USG_STRING "%f:start %f:stop %d:num_values"


float Start, Stop;
int NumValues;


int main(int argc, char *argv[])
{
  double interval;
  double current_value;
  int i;

  QccInit(argc, argv);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &Start,
                         &Stop,
                         &NumValues))
    QccErrorExit();

  if (Start <= 0.0)
    {
      QccErrorAddMessage("%s: starting value must be positive",
                         argv[0]);
      QccErrorExit();
    }
    
  if (Stop <= Start)
    {
      QccErrorAddMessage("%s: stopping value must be greater than starting value",
                         argv[0]);
      QccErrorExit();
    }

  if (NumValues < 2)
    {
      QccErrorAddMessage("%s: num_values must be 2 or greater",
                         argv[0]);
      QccErrorExit();
    }

  interval = pow((Stop/Start), (1.0/(NumValues - 1)));

  current_value = Start;
  printf("%f ", current_value);

  for (i = 2; i <= NumValues; i++)
    {
      current_value *= interval;
      printf("%f ", current_value);
    }           
  printf("\n");

  QccExit;
}
