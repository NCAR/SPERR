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


#include "sqtocbk.h"

#define USG_STRING "%s:sqfile %s:cbkfile"

QccSQScalarQuantizer ScalarQuantizer;
QccVQCodebook Codebook;


int main(int argc, char *argv[])
{
  QccInit(argc, argv);
  
  QccSQScalarQuantizerInitialize(&ScalarQuantizer);
  QccVQCodebookInitialize(&Codebook);

  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         ScalarQuantizer.filename,
                         Codebook.filename))
    QccErrorExit();
  
  if (QccSQScalarQuantizerRead(&ScalarQuantizer))
    {
      QccErrorAddMessage("%s: Error calling QccSQScalarQuantizerRead()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccVQScalarQuantizerToVQCodebook(&ScalarQuantizer,
                                       &Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQScalarQuantizerToVQCodebook()",
                         argv[0]);
      QccErrorExit();
    }

  if (QccVQCodebookWrite(&Codebook))
    {
      QccErrorAddMessage("%s: Error calling QccVQCodebookWrite()",
                         argv[0]);
      QccErrorExit();
    }

  QccSQScalarQuantizerFree(&ScalarQuantizer);
  QccVQCodebookFree(&Codebook);

  QccExit;
}
