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


#include "avqrate.h"

#define USG_STRING "[-vo %:] [-noent %:] %s:codebook_coder %s:channelfile %s:sideinfofile"

#define AVQRATE_PROGRAM_GTR   "gtrencode"
#define AVQRATE_PROGRAM_PAUL  "paulencode"
#define AVQRATE_PROGRAM_GY    "gyencode"

QccChannel Channel;
QccAVQSideInfo SideInfo;

int ValueOnly = 0;
int Verbose;
int NoEntropyCodeSideInfo = 0;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccChannelInitialize(&Channel);
  QccAVQSideInfoInitialize(&SideInfo);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &ValueOnly,
                         &NoEntropyCodeSideInfo,
                         SideInfo.codebook_coder.filename,
                         Channel.filename,
                         SideInfo.filename))
    QccErrorExit();
  
  if (QccSQScalarQuantizerRead(&(SideInfo.codebook_coder)))
    {
      QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (QccChannelReadWholefile(&Channel))
    {
      QccErrorAddMessage("%s: Error calling QccChannelReadWholefile()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccAVQSideInfoStartRead(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  if (!strlen(SideInfo.program_name))
    QccStringSprintf(SideInfo.program_name, AVQRATE_PROGRAM_GTR);
  
  Verbose = (ValueOnly) ? 1 : 2;

  if (!strcmp(SideInfo.program_name, AVQRATE_PROGRAM_GTR))
    {
      if (QccAVQgtrCalcRate(&Channel, &SideInfo, stdout, NULL, Verbose,
                            !NoEntropyCodeSideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQgtrCalcRate()",
                             argv[0]);
          QccErrorExit();
        }
    }
  else if (!strcmp(SideInfo.program_name, AVQRATE_PROGRAM_PAUL))
    {
      if (QccAVQPaulCalcRate(&Channel, &SideInfo, stdout, NULL,
                             Verbose, !NoEntropyCodeSideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQPaulCalcRate()",
                             argv[0]);
          QccErrorExit();
        }
    }
  else if (!strcmp(SideInfo.program_name, AVQRATE_PROGRAM_GY))
    {
      if (QccAVQGershoYanoCalcRate(&Channel, &SideInfo, stdout, NULL,
                                   Verbose, !NoEntropyCodeSideInfo))
        {
          QccErrorAddMessage("%s: Error calling QccAVQGershoYanoCalcRate()",
                             argv[0]);
          QccErrorExit();
        }
    }
  else
    {
      QccErrorAddMessage("%s: Unrecognized program %s created sideinfo file %s",
                         argv[0], SideInfo.program_name,
                         SideInfo.filename);
    }

  if (QccAVQSideInfoEndRead(&SideInfo))
    {
      QccErrorAddMessage("%s: Error calling QccAVQSideInfoEndRead()",
                         argv[0]);
      QccErrorExit();
    }
  
  QccChannelFree(&Channel);

  QccExit;
}
