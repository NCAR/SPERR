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


#include "rsencode.h"

#define USG_STRING "[-k %d:k] [-n %d:n] %s:infile %s:outfile"


QccString InfileName;
QccString OutfileName;


int main(int argc, char *argv[])
{
  QccECCReedSolomonCode code;
  QccECCFieldElement *message = NULL;
  QccECCFieldElement *coded_message = NULL;
  FILE *infile = NULL;
  FILE *outfile = NULL;
  int index;
  char current_ch;

  QccInit(argc, argv);

  QccECCReedSolomonCodeInitialize(&code);

  /*  Default: (15, 10) code - 10 messages symbols  */
  /*  protected by 5 FEC symbols  */
  code.message_length = 10;
  code.code_length = 15;

  /*  A message alphabet of 256 symbols  */
  code.field.size_exponent = 8;

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &code.message_length,
                         &code.code_length,
                         InfileName,
                         OutfileName))
    QccErrorExit();

  if ((message =
       (QccECCFieldElement *)malloc(sizeof(QccECCFieldElement) *
                                    code.message_length)) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory");
      QccErrorExit();
    }

  if ((coded_message =
       (QccECCFieldElement *)malloc(sizeof(QccECCFieldElement) *
                                    code.code_length)) == NULL)
    {
      QccErrorAddMessage("%s: Error allocating memory");
      QccErrorExit();
    }

  if (QccECCReedSolomonCodeAllocate(&code))
    {
      QccErrorAddMessage("%s: Error calling QccECCReedSolomonCodeAllocate()");
      QccErrorExit();
    }

  if ((infile = QccFileOpen(InfileName, "r")) == NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()");
      QccErrorExit();
    }

  if ((outfile = QccFileOpen(OutfileName, "w")) == NULL)
    {
      QccErrorAddMessage("%s: Error calling QccFileOpen()");
      QccErrorExit();
    }

  index = 0;
  while (!feof(infile))
    {
      for (index = 0; index < code.message_length; index++)
        if (QccFileReadChar(infile, &current_ch))
          message[index] = 0;
        else
          message[index] = (QccECCFieldElement)current_ch;

      if (QccECCReedSolomonEncode(message, coded_message, &code))
        {
          QccErrorAddMessage("%s: Error calling QccECCReedSolomonEncode()");
          QccErrorExit();
        }

      for (index = 0; index < code.code_length; index++)
        if (QccFileWriteChar(outfile,
                             (char)coded_message[index]))
        {
          QccErrorAddMessage("%s: Error calling QccFileWriteChar()");
          QccErrorExit();
        }
    }

  QccFileClose(infile);
  QccFileClose(outfile);

  QccECCReedSolomonCodeFree(&code);

  QccExit;
}
