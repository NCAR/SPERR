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


static QccString QccProgramName;
#ifdef QCC_USE_PTHREADS
static pthread_mutex_t QccProgramNameMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


void QccExtractProgramName(const char *argv0)
{
  const char *start;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccProgramNameMutex))
    {
      QccErrorAddMessage("(QccExtractProgramName): Error locking mutex");
      QccExit;
    }
#endif

  QccStringMakeNull(QccProgramName);

  if (argv0 == NULL)
    goto Return;

  if (strlen(argv0) < 1)
    goto Return;

  start = strrchr(argv0, '/');
  if (start == NULL)
    QccConvertToQccString((char *)QccProgramName, argv0);
  else
    QccConvertToQccString((char *)QccProgramName, (char *)(start + 1));

 Return:
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccProgramNameMutex))
    {
      QccErrorAddMessage("(QccExtractProgramName): Error unlocking mutex");
      QccExit;
    }
#endif

  return;
}


int QccGetProgramName(QccString program_name)
{
  if (program_name == NULL)
    return(0);

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccProgramNameMutex))
    {
      QccErrorAddMessage("(QccGetProgramName): Error locking mutex");
      QccExit;
    }
#endif

  QccStringMakeNull(program_name);
  
  if (QccStringNull(QccProgramName))
    {
      QccErrorAddMessage("(QccGetProgramName): No valid program name has been set");
      return(1);
    }

  QccStringCopy(program_name, QccProgramName);

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccProgramNameMutex))
    {
      QccErrorAddMessage("(QccGetProgramName): Error unlocking mutex");
      QccExit;
    }
#endif

  return(0);
  
}


static void QccInit2(void)
{
  QccVersionInit();
  QccErrorInit();
  QccFileInit();
  QccWAVInit();
  QccTimeInit();
#ifdef QCC_USE_MTRACE
  mtrace();
#endif
#ifdef QCC_USE_GSL
  gsl_set_error_handler_off();
#endif
}

void QccInit(int argc, char *argv[])
{
#ifdef QCC_USE_PTHREADS
  static pthread_once_t first_time = PTHREAD_ONCE_INIT;
#else
  static int first_time = 1;
#endif

  QccExtractProgramName((argv != NULL) ? argv[0] : NULL);

  setvbuf(stdout, (char *)NULL, _IOLBF, 0);

#ifdef QCC_USE_PTHREADS
  pthread_once(&first_time, QccInit2);
#else
  if (first_time)
    {
      first_time = 0;
      QccInit2();
    }
#endif
}
