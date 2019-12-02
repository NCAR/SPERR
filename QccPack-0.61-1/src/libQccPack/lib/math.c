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


static void QccMathRandInitialize(void)
{
  srandom((unsigned int)time(NULL));
}


double QccMathRand(void)
{
  int r;
  double rnum;
  static pthread_once_t first_time_called = PTHREAD_ONCE_INIT;
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

  if (pthread_once(&first_time_called, QccMathRandInitialize))
    {
      QccErrorAddMessage("(QccMathRand): Error calling random-number initialization");
      QccExit;
    }

  if (pthread_mutex_lock(&mutex))
    {
      QccErrorAddMessage("(QccMathRand): Error locking mutex");
      QccExit;
    }

  r = random();

  if (pthread_mutex_unlock(&mutex))
    {
      QccErrorAddMessage("(QccMathRand): Error unlocking mutex");
      QccExit;
    }

  /*  Uniform in range [0.0, 1.0]  */
  rnum = (double)r/(MAXINT);
  
  return(rnum);
}


#else


double QccMathRand(void)
{
  static int first_time_called = 1;
  double rnum;
  
  if (first_time_called)
    {
      srandom((unsigned int)time(NULL));
      first_time_called = 0;
    }
  
  /*  Uniform in range [0.0, 1.0]  */
  rnum = (double)random()/(MAXINT);
  
  return(rnum);
}


#endif


/*
double QccMathRandNormal(void)
{
  double value;

  do
    {
      value = QccMathRand();
    }
  while (value == 0);

  return(sqrt(-2 * log(value)) * cos(2 * M_PI * QccMathRand()));
}
*/


double QccMathRandNormal(void)
{
#ifdef QCC_USE_PTHREADS
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
  static int previous_value_set = 0;
  static double previous_value;
  double current_value;
  double C, R;
  double value1, value2;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&mutex))
    {
      QccErrorAddMessage("(QccMathRandNormal): Error locking mutex");
      QccExit;
    }
#endif

  if (!previous_value_set)
    {
      do
        {
          value1 = 2 * QccMathRand() - 1;
          value2 = 2 * QccMathRand() - 1;
          R = value1 * value1 + value2 * value2;
        }
      while ((R >= 1) || (R == 0));

      C = sqrt(-2*log(R)/R);

      previous_value = value1 * C;
      previous_value_set = 1;

      current_value = value2 * C;
    }
  else
    {
      previous_value_set = 0;
      current_value = previous_value;
    }

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&mutex))
    {
      QccErrorAddMessage("(QccMathRandNormal): Error unlocking mutex");
      QccExit;
    }
#endif

  return(current_value);
}


double QccMathGaussianDensity(double x, double variance, double mean)
{
  return(exp((-(x - mean)*(x - mean))/2/variance)/sqrt(2*M_PI*variance));
}


double QccMathLaplacianDensity(double x, double variance, double mean)
{
  return(exp((-sqrt(2/variance))*fabs(x - mean))/sqrt(2*variance));
}
