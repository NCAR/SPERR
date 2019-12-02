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
#include "version.h"


static QccString QccUserHeader;
#ifdef QCC_USE_PTHREADS
static pthread_mutex_t QccUserHeaderMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


void QccVersionInit()
{
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccUserHeaderMutex))
    {
      QccErrorAddMessage("(QccVersionInit): Error locking mutex");
      QccExit;
    }
#endif
  QccStringMakeNull(QccUserHeader);
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccUserHeaderMutex))
    {
      QccErrorAddMessage("(QccVersionInit): Error unlocking mutex");
      QccExit;
    }
#endif
}


void QccSetUserHeader(const QccString user_header)
{
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccUserHeaderMutex))
    {
      QccErrorAddMessage("(QccSetUserHeader): Error locking mutex");
      QccExit;
    }
#endif
  QccStringCopy(QccUserHeader, user_header);
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccUserHeaderMutex))
    {
      QccErrorAddMessage("(QccSetUserHeader): Error unlocking mutex");
      QccExit;
    }
#endif
}


void QccGetQccPackVersion(int *major, int *minor, QccString date)
{
  if (major != NULL)
    *major = QccPackMajorVersion;
  if (minor != NULL)
    *minor = QccPackMinorVersion;
  if (date != NULL)
    QccStringCopy(date, QccPackDate);
}


void QccPrintQccPackVersion(FILE *outfile)
{
  int major, minor;
  QccString date;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccUserHeaderMutex))
    {
      QccErrorAddMessage("(QccPrintQccPackVersion): Error locking mutex");
      QccExit;
    }
#endif
  if (strlen(QccUserHeader) > 0)
    {
      fprintf(outfile, "%s\n\n", QccUserHeader);
    }
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccUserHeaderMutex))
    {
      QccErrorAddMessage("(QccPrintQccPackVersion): Error unlocking mutex");
      QccExit;
    }
#endif

  QccGetQccPackVersion(&major, &minor, date);

  fprintf(outfile,
          "%s Version %d.%d %s,\n%s",
          QccPackName,
          major, minor, date,
          QccPackCopyright);
  if (strlen(QccPackNote))
    fprintf(outfile,
            ",\n%s",
            QccPackNote);
  fprintf(outfile, "\n\n");
}


int QccCompareQccPackVersions(int major1, int minor1,
                              int major2, int minor2)
{
  if (major1 < major2)
    return(-1);
  if (major1 > major2)
    return(1);
  if (minor1 < minor2)
    return(-1);
  if (minor1 > minor2)
    return(1);
  return(0);
}
