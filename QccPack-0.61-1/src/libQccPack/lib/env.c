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


int QccGetEnv(const char *environment_variable,
              QccString returned_value)
{
  int return_value;
  char *value = NULL;

  if (environment_variable == NULL)
    return(0);

  QccStringMakeNull(returned_value);

  if ((value = getenv(environment_variable)) == NULL)
    goto Error;

  if (returned_value != NULL)
    QccConvertToQccString(returned_value, value);

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}


int QccSetEnv(const char *environment_variable,
              const char *value)
{
  int length;
  char *new_string = NULL;
  int return_value;

  if (environment_variable == NULL)
    return(0);
  if (value == NULL)
    return(0);

  if (!QccGetEnv(environment_variable, NULL))
    return(0);

  length = strlen(environment_variable) + strlen(value) + 2;
  
  if ((new_string = (char *)malloc(sizeof(char) * length)) == NULL)
    {
      QccErrorAddMessage("(QccSetEnv): Error allocating memory");
      goto Error;
    }

#ifdef QCC_NO_SNPRINTF
  sprintf(new_string, "%s=%s", environment_variable, value);
#else
  snprintf(new_string, length, "%s=%s", environment_variable, value);
#endif

  if (putenv(new_string))
    {
      QccErrorAddMessage("(QccSetEnv): Error calling putenv() with string \"%s\"",
                         new_string);
      goto Error;
    }

  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  return(return_value);
}
