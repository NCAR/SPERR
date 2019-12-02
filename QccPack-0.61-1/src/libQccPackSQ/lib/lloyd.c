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


/*
 *  Refer to:
 *    A. Gersho and R. M. Gray, "Vector Quantization and Signal Compression,"
 *      Kluwer Academic Publishers, Boston, 1992, pp. 187-194.
 */
static void QccSQLloydUpdateCentroids(QccSQScalarQuantizer *quantizer, 
                                      double max_value, double min_value,
                                      QccMathProbabilityDensity
                                      probability_density,
                                      double variance,
                                      double mean,
                                      int num_integration_intervals)
{
  double start, stop;
  int level;
  double delta_x;
  double x;
  int interval;
  double centroid_num;
  double centroid_den;
  double val;

  /*
   *  We integrate numerically using Simpson's Rule
   *    num_integration_intervals must be even for Simpson's Rule
   */

  if (num_integration_intervals & 1)
    num_integration_intervals++;

  start = min_value;
  for (level = 0; level < quantizer->num_levels; level++)
    {
      stop = (level < quantizer->num_levels - 1) ?
        quantizer->boundaries[level] : max_value;
      delta_x = (stop - start)/num_integration_intervals;
      val = probability_density(start, variance, mean);
      centroid_den = val;
      centroid_num = val * start;
      val = probability_density(stop, variance, mean);
      centroid_den += val;
      centroid_num += val * stop;
      for (interval = 1; interval <= num_integration_intervals - 1; 
           interval++)
        {
          x = start + interval * delta_x;
          val = ((interval & 1) ? 4 : 2) *
            probability_density(x, variance, mean);
          centroid_den += val;
          centroid_num += val * x;
        }
      if (centroid_den != 0.0)
        quantizer->levels[level] = centroid_num / centroid_den;
      else
        quantizer->levels[level] = (start + stop)/2;
      quantizer->probs[level] = centroid_den *
        (stop - start)/3/num_integration_intervals;

      start = stop;
    }

}


void QccSQLloydUpdateBoundaries(QccSQScalarQuantizer *quantizer)
{
  int level;

  for (level = 0; level < quantizer->num_levels - 1; level++)
    {
      quantizer->boundaries[level] =
        (quantizer->levels[level] + quantizer->levels[level + 1])/2;
    }
}


static double QccSQLloydCalcDistortion(QccSQScalarQuantizer *quantizer, 
                                       double max_value, double min_value,
                                       QccMathProbabilityDensity
                                       probability_density,
                                       double variance,
                                       double mean,
                                       int num_integration_intervals)
{
  double start, stop;
  int level;
  double delta_x;
  int interval;
  double distortion = 0.0;
  double val;
  double x;

  /*
   *  We integrate numerically using Simpson's Rule
   *    num_integration_intervals must be even for Simpson's Rule
   */

  if (num_integration_intervals & 1)
    num_integration_intervals++;

  start = min_value;
  for (level = 0; level < quantizer->num_levels; level++)
    {
      stop = (level < quantizer->num_levels - 1) ?
        quantizer->boundaries[level] : max_value;
      delta_x = (stop - start)/num_integration_intervals;
      val = probability_density(start, variance, mean) *
        (start - quantizer->levels[level]) *
        (start - quantizer->levels[level]) +
        probability_density(stop, variance, mean) *
        (stop - quantizer->levels[level]) *
        (stop - quantizer->levels[level]);
      for (interval = 1; interval <= num_integration_intervals - 1; 
           interval++)
        {
          x = start + interval*delta_x;
          val += ((interval & 1) ? 4 : 2) *
            probability_density(x, variance, mean) *
            (x - quantizer->levels[level]) *
            (x - quantizer->levels[level]);
        }
      distortion += val * (stop - start)/3/num_integration_intervals;
      start = stop;
    }

  return(distortion);
}


int QccSQLloydMakeQuantizer(QccSQScalarQuantizer *quantizer,
                            QccMathProbabilityDensity probability_density,
                            double variance,
                            double mean,
                            double max_value,
                            double min_value,
                            double stop_threshold,
                            int num_integration_intervals)
{
  double previous_distortion = MAXDOUBLE;
  double distortion;

  if (quantizer == NULL)
    return(0);
  if (probability_density == NULL)
    return(0);

  if (max_value <= min_value)
    {
      QccErrorAddMessage("(QccSQLloydMakeQuantizer): Max value (%f) must be larger than min value (%f)",
                         max_value, min_value);
      return(1);
    }

  if (num_integration_intervals < 2)
    {
      QccErrorAddMessage("(QccSQLloydMakeQuantizer): Number of integration intervals must be 2 or greater");
      return(1);
    }

  quantizer->type = QCCSQSCALARQUANTIZER_GENERAL;

  if (quantizer->num_levels < 1)
    return(0);

  if (QccSQUniformMakeQuantizer(quantizer,
                                max_value,
                                min_value, 
                                QCCSQ_NOOVERLOAD))
    {
      QccErrorAddMessage("(QccSQLloydMakeQuantizer): Error calling QccSQUniformMakeQuantizer()");
      return(1);
    }
  for (;;)
    {
      QccSQLloydUpdateCentroids(quantizer,
                                max_value,
                                min_value,
                                probability_density,
                                variance,
                                mean,
                                num_integration_intervals);
      QccSQLloydUpdateBoundaries(quantizer);
      
      distortion = QccSQLloydCalcDistortion(quantizer,
                                            max_value,
                                            min_value,
                                            probability_density,
                                            variance,
                                            mean,
                                            num_integration_intervals);
      if ((((previous_distortion - distortion) / previous_distortion) <
           stop_threshold) ||
          (distortion == 0.0))
        break;
      previous_distortion = distortion;
    }

  return(0);
}
