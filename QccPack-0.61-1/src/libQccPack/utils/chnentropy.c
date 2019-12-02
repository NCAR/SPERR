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


#include "chnentropy.h"

#define USG_STRING "[-vo %:] [-d %: %d:vector_dimension] [-o %d:order] %s:channelfile"


QccChannel Channel;
int VectorDimension;
int vector_dimension_specified = 0;

int value_only_flag = 0;

int Order = 1;

int main(int argc, char *argv[])
{
  double entropy;

  QccInit(argc, argv);

  QccChannelInitialize(&Channel);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &value_only_flag,
                         &vector_dimension_specified,
                         &VectorDimension,
                         &Order,
                         Channel.filename))
    QccErrorExit();

  if ((Order != 1) && (Order != 2))
    {
      QccErrorAddMessage("%s: Only orders 1 and 2 are valid choices",
                         argv[0]);
      QccErrorExit();
    }

  if (QccChannelReadWholefile(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelReadWholefile()",
                         argv[0]);
      QccErrorExit();
    }

  if ((entropy = QccChannelEntropy(&Channel, Order)) < 0.0)
    {
      QccErrorAddMessage("%s: Error calling QccChannelEntropy()",
                         argv[0]);
      QccErrorExit();
    }

  if (!value_only_flag)
    {
      if (Order == 1)
        printf("First-order entropy of channel\n   %s\nis:     ",
               Channel.filename);
      else
        printf("Second-order entropy of channel\n   %s\nis:     ",
               Channel.filename);
    }

  printf("%f",
         (vector_dimension_specified ? entropy/VectorDimension : entropy));
  if (!value_only_flag)
    printf(" (bits/symbol)\n");
  else
    printf("\n");

  QccChannelFree(&Channel);

  QccExit;
}
