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


#ifdef QCC_USE_PTHREADS
static pthread_mutex_t QccTimeMutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static struct timeval QccTimeStartTime;
static int QccTimeTableLength;
typedef struct
{
#ifdef QCC_USE_PTHREADS
  pthread_t thread_id;
#endif
  struct timeval time;
} QccTimeTableEntry;
static QccTimeTableEntry *QccTimeTable;


void QccTimeInit()
{
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccTimeMutex))
    {
      QccErrorAddMessage("(QccTimeInit): Error locking mutex");
      QccExit;
    }
#endif

  if (gettimeofday(&QccTimeStartTime, NULL))
    {
      QccErrorAddMessage("(QccTimeInit): Error getting time");
      QccExit;
    }

  QccTimeTableLength = 0;
  QccTimeTable = NULL;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccTimeMutex))
    {
      QccErrorAddMessage("(QccTimeInit): Error unlocking mutex");
      QccExit;
    }
#endif

}


static double QccTimeToSeconds(const struct timeval *time)
{
  if (time == NULL)
    return((double)0);

  return(((double)time->tv_sec * 1000000 + time->tv_usec) / 1000000);
}


static void QccTimeDifference(struct timeval *stop_time,
                              struct timeval *start_time,
                              struct timeval *difference)
{
  difference->tv_sec = stop_time->tv_sec - start_time->tv_sec;
  difference->tv_usec = stop_time->tv_usec - start_time->tv_usec;
  if (difference->tv_usec < 0)
    {
      difference->tv_sec--;
      difference->tv_usec += 1000000;
    }
}


int QccTimeTic()
{
  struct timeval current_time;

  if (gettimeofday(&current_time, NULL))
    {
      QccErrorAddMessage("(QccTimeTic): Error getting time");
      return(1);
    }

#ifndef QCC_USE_PTHREADS
  if (!QccTimeTableLength)
    {
      QccTimeTableLength = 1;
      if ((QccTimeTable =
           (QccTimeTableEntry *)malloc(sizeof(QccTimeTableEntry))) == NULL)
        {
          QccErrorAddMessage("(QccTimeTic): Error allocating memory");
          return(1);
        }
      QccTimeTable[0].time = current_time;
    }
#else
  if (pthread_mutex_lock(&QccTimeMutex))
    {
      QccErrorAddMessage("(QccTimeTic): Error locking mutex");
      return(1);
    }
  if (!QccTimeTableLength)
    {
      if ((QccTimeTable =
           (QccTimeTableEntry *)malloc(sizeof(QccTimeTableEntry))) == NULL)
        {
          QccErrorAddMessage("(QccTimeTic): Error allocating memory");
          return(1);
        }
      QccTimeTableLength++;
      QccTimeTable[0].thread_id = pthread_self();
      QccTimeTable[0].time = current_time;
    }
  else
    {
      pthread_t thread_id = pthread_self();
      int entry;
      int found = 0;

      for (entry = 0; entry < QccTimeTableLength; entry++)
        if (thread_id == QccTimeTable[entry].thread_id)
          {
            found = 1;
            break;
          }

      if (found)
        QccTimeTable[entry].time = current_time;
      else
        {
          QccTimeTableLength++;
          if ((QccTimeTable =
               (QccTimeTableEntry *)realloc((void *)QccTimeTable,
                                            sizeof(QccTimeTableEntry) *
                                            QccTimeTableLength)) == NULL)
            {
              QccErrorAddMessage("(QccTimeTic): Error reallocating memory");
              return(1);
            }
          QccTimeTable[QccTimeTableLength - 1].thread_id = thread_id;
          QccTimeTable[QccTimeTableLength - 1].time = current_time;
        }
    }
  if (pthread_mutex_unlock(&QccTimeMutex))
    {
      QccErrorAddMessage("(QccTimeTic): Error unlocking mutex");
      return(1);
    }
#endif

  return(0);
}


int QccTimeToc(double *time)
{
  struct timeval current_time;
  struct timeval start_time;
  struct timeval difference;

  if (gettimeofday(&current_time, NULL))
    {
      QccErrorAddMessage("(QccTimeToc): Error getting time");
      return(1);
    }

#ifndef QCC_USE_PTHREADS
  if (!QccTimeTableLength)
    start_time = QccTimeStartTime;
  else
    start_time = QccTimeTable[0].time;
#else
  if (pthread_mutex_lock(&QccTimeMutex))
    {
      QccErrorAddMessage("(QccTimeToc): Error locking mutex");
      return(1);
    }
  if (!QccTimeTableLength)
    start_time = QccTimeStartTime;
  else
    {
      pthread_t thread_id = pthread_self();
      int entry;
      int found = 0;

      for (entry = 0; entry < QccTimeTableLength; entry++)
        if (thread_id == QccTimeTable[entry].thread_id)
          {
            found = 1;
            break;
          }

      if (found)
        start_time = QccTimeTable[entry].time;
      else
        start_time = QccTimeStartTime;
    }
  if (pthread_mutex_unlock(&QccTimeMutex))
    {
      QccErrorAddMessage("(QccTimeToc): Error unlocking mutex");
      return(1);
    }
#endif

  QccTimeDifference(&current_time, &start_time, &difference);

  if (time != NULL)
    *time = QccTimeToSeconds(&difference);
  
  return(0);
}
