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


#include "seqdist.h"

#define USG_STRING "[-vo %:] [-sf %: %d:startframe] [-ef %: %d:endframe] [-mse %:] [-snr %:] [-psnr %:] [-mae %:] [-w %:] %s:seqfile1 %s:seqfile2"

#define ALL 0
#define MSE 1
#define SNR 2
#define PSNR 3
#define MAE 4

int StartFrameNumSpecified = 0;
int StartFrameNum;
int EndFrameNumSpecified = 0;
int EndFrameNum;

int value_only_flag = 0;
int mse_specified = 0;
int snr_specified = 0;
int psnr_specified = 0;
int mae_specified = 0;

int whole_sequence_only = 0;

int DistortionMethod = ALL;

int ImageType;
int MaxVal = 255;

int NumCols1, NumCols2;
int NumRows1, NumRows2;

int NumFrames;

QccIMGImageSequence ImageSequence1;
QccIMGImageSequence ImageSequence2;


int main(int argc, char *argv[])
{
  double value = 0;
  double mse = -1.0;
  double snr = -1.0;
  double psnr = -1.0;
  double mae = -1.0;
  double signal_power;
  QccIMGImageComponent *image_component1 = NULL;
  QccIMGImageComponent *image_component2 = NULL;
  int component, num_components;
  QccString component_name;
  int num_rows1_Y, num_cols1_Y;
  int num_rows1_U, num_cols1_U;
  int num_rows1_V, num_cols1_V;
  int num_rows2_Y, num_cols2_Y;
  int num_rows2_U, num_cols2_U;
  int num_rows2_V, num_cols2_V;
  int current_frame;
  double total_mse[3] = { 0.0, 0.0, 0.0 };
  double total_mae[3] = { 0.0, 0.0, 0.0 };
  double total_signal_power[3] = { 0.0, 0.0, 0.0 };
  double total_snr = 0.0;
  double total_psnr = 0.0;

  QccInit(argc, argv);

  QccIMGImageSequenceInitialize(&ImageSequence1);
  QccIMGImageSequenceInitialize(&ImageSequence2);
  
  if (QccParseParameters(argc, argv,
                         USG_STRING,
                         &value_only_flag,
                         &StartFrameNumSpecified,
			 &StartFrameNum,
                         &EndFrameNumSpecified,
			 &EndFrameNum,
                         &mse_specified,
                         &snr_specified,
                         &psnr_specified,
                         &mae_specified,
                         &whole_sequence_only,
                         ImageSequence1.filename, ImageSequence2.filename))
    QccErrorExit();

  if (mse_specified + snr_specified + psnr_specified + mae_specified > 1)
    {
      QccErrorAddMessage("%s: Only one of -mse, -snr, -psnr, and -mae can be given",
                         argv[0]);
      QccErrorExit();
    }

  if (mse_specified)
    DistortionMethod = MSE;
  else if (snr_specified)
    DistortionMethod = SNR;
  else if (psnr_specified)
    DistortionMethod = PSNR;
  else if (mae_specified)
    DistortionMethod = MAE;
  else
    DistortionMethod = ALL;

  if (value_only_flag &&
      (!mse_specified && !snr_specified && !psnr_specified && !mae_specified))
    {
      QccErrorAddMessage("%s: One of -mse, -snr, -psnr, and -mae must be used with -vo",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceFindFrameNums(&ImageSequence1))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceFindFrameNums()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageSequenceFindFrameNums(&ImageSequence2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceFindFrameNums()",
                         argv[0]);
      QccErrorExit();
    }

  if (!StartFrameNumSpecified)
    StartFrameNum = ImageSequence1.start_frame_num;

  if ((StartFrameNum < ImageSequence1.start_frame_num) ||
      (StartFrameNum < ImageSequence2.start_frame_num))
    {
      QccErrorAddMessage("%s: Start frame %d is out of the range of available frames which start at %d",
                         argv[0],
                         StartFrameNum,
                         QccMathMax(ImageSequence1.start_frame_num,
                                    ImageSequence2.start_frame_num));
      QccErrorExit();
    }
  ImageSequence1.start_frame_num = StartFrameNum;
  ImageSequence2.start_frame_num = StartFrameNum;

  if (!EndFrameNumSpecified)
    EndFrameNum = ImageSequence1.end_frame_num;

  if ((EndFrameNum > ImageSequence1.end_frame_num) ||
      (EndFrameNum > ImageSequence2.end_frame_num))
    {
      QccErrorAddMessage("%s: End frame %d is out of the range of available frames which end at %d",
                         argv[0],
                         EndFrameNum,
                         QccMathMin(ImageSequence1.end_frame_num,
                                    ImageSequence2.end_frame_num));
      QccErrorExit();
    }
  ImageSequence1.end_frame_num = EndFrameNum;
  ImageSequence2.end_frame_num = EndFrameNum;

  NumFrames = QccIMGImageSequenceLength(&ImageSequence1);
  if (NumFrames !=
      QccIMGImageSequenceLength(&ImageSequence2))
    {
      QccErrorAddMessage("%s: Image sequences are of different lengths",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageSequenceStartRead(&ImageSequence1))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  ImageType = ImageSequence1.current_frame.image_type;

  if (QccIMGImageSequenceStartRead(&ImageSequence2))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageSequenceStartRead()",
                         argv[0]);
      QccErrorExit();
    }
  if (ImageSequence2.current_frame.image_type != ImageType)
    {
      QccErrorAddMessage("%s: Image sequences must both be color (or grayscale)",
                         argv[0]);
      QccErrorExit();
    }

  if (QccIMGImageGetSizeYUV(&(ImageSequence1.current_frame),
                            &num_rows1_Y, &num_cols1_Y,
                            &num_rows1_U, &num_cols1_U,
                            &num_rows1_V, &num_cols1_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }
  if (QccIMGImageGetSizeYUV(&(ImageSequence2.current_frame),
                            &num_rows2_Y, &num_cols2_Y,
                            &num_rows2_U, &num_cols2_U,
                            &num_rows2_V, &num_cols2_V))
    {
      QccErrorAddMessage("%s: Error calling QccIMGImageGetSizeYUV()",
                         argv[0]);
      QccErrorExit();
    }

  num_components = QccIMGImageColor(&(ImageSequence1.current_frame)) ? 3 : 1;

  if (!value_only_flag)
    printf("Distortion between sequences\n   %s\n         and\n   %s\nis:\n",
           ImageSequence1.filename, ImageSequence2.filename);

  
  ImageSequence1.current_frame_num = ImageSequence1.start_frame_num - 1;
  ImageSequence2.current_frame_num = ImageSequence2.start_frame_num - 1;

  for (current_frame = ImageSequence1.start_frame_num;
       current_frame <= ImageSequence1.end_frame_num;
       current_frame++)
    {
      if (QccIMGImageSequenceIncrementFrameNum(&ImageSequence1))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSequenceIncrementFrameNum()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageSequenceReadFrame(&ImageSequence1))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSequenceReadFrame()",
                             argv[0]);
          QccErrorExit();
        }

      if (QccIMGImageSequenceIncrementFrameNum(&ImageSequence2))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSequenceIncrementFrameNum()",
                             argv[0]);
          QccErrorExit();
        }
      if (QccIMGImageSequenceReadFrame(&ImageSequence2))
        {
          QccErrorAddMessage("%s: Error calling QccIMGImageSequenceReadFrame()",
                             argv[0]);
          QccErrorExit();
        }

      for (component = 0; component < num_components; component++)
        {
          switch (component)
            {
            case 0:
              image_component1 = &(ImageSequence1.current_frame.Y);
              image_component2 = &(ImageSequence2.current_frame.Y);
              QccStringCopy(component_name, "Y component");
              NumRows1 = num_rows1_Y;
              NumCols1 = num_cols1_Y;
              NumRows2 = num_rows2_Y;
              NumCols2 = num_cols2_Y;
              break;
            case 1:
              image_component1 = &(ImageSequence1.current_frame.U);
              image_component2 = &(ImageSequence2.current_frame.U);
              QccStringCopy(component_name, "U component");
              NumRows1 = num_rows1_U;
              NumCols1 = num_cols1_U;
              NumRows2 = num_rows2_U;
              NumCols2 = num_cols2_U;
              break;
            case 2:
              image_component1 = &(ImageSequence1.current_frame.V);
              image_component2 = &(ImageSequence2.current_frame.V);
              QccStringCopy(component_name, "V component");
              NumRows1 = num_rows1_V;
              NumCols1 = num_cols1_V;
              NumRows2 = num_rows2_V;
              NumCols2 = num_cols2_V;
              break;
            }          
          
          if ((NumCols1 != NumCols2) || (NumRows1 != NumRows2))
            {
              QccErrorAddMessage("%s: Size of %s (%d x %d) does not equal that of %s (%d x %d) for image component %d",
                                 argv[0],
                                 ImageSequence1.filename, NumCols1, NumRows1,
                                 ImageSequence2.filename, NumCols2, NumRows2,
                                 component);
              QccErrorExit();
            }
          
          mse = QccIMGImageComponentMSE(image_component1, image_component2);
          mae = QccIMGImageComponentMAE(image_component1, image_component2);
          total_mse[component] += mse/NumFrames;
          total_mae[component] += mae/NumFrames;
          signal_power = QccIMGImageComponentVariance(image_component1);
          total_signal_power[component] += signal_power/NumFrames;

          if (mse != 0.0)
            {
              psnr = 10.0*log10((double)MaxVal*MaxVal/mse);
              snr = 10.0*log10(signal_power/mse);
            }
          
          if (!whole_sequence_only)
            {
              if (!value_only_flag)
                {
                  printf("  Frame #%d:\n", current_frame);
                  if (num_components != 1)
                    printf("   %s:\n", component_name);
                  switch (DistortionMethod)
                    {
                    case MSE:
                      printf("%16.6f (MSE)\n", mse);
                      break;
                    case SNR:
                      printf("%16.6f dB (SNR)\n", snr);
                      break;
                    case PSNR:
                      printf("%16.6f dB (PSNR)\n", psnr);
                      break;
                    case MAE:
                      printf("%16.6f (MAE)\n", mae);
                      break;
                    case ALL:
                    default:
                      printf("%16.6f\t(MSE)\n%16.6f dB\t(SNR)\n%16.6f dB\t(PSNR)\n%16.6f\t(MAE)\n",
                             mse, snr, psnr, mae);
                      break;
                    }
                }
              else
                {
                  switch (DistortionMethod)
                    {
                    case MSE:
                      value = mse;
                      break;
                    case SNR:
                      value = snr;
                      break;
                    case PSNR:
                      value = psnr;
                      break;
                    case MAE:
                      value = mae;
                      break;
                    }
                  printf("%f\n", value);
                }
            }
        }
    }

  if (whole_sequence_only || !value_only_flag)
    {
      if (!value_only_flag)
        {
          if (!whole_sequence_only)
            printf("===========================================================\n");
          printf("Over whole sequence:\n");
        }

      for (component = 0; component < num_components; component++)
        {
          if (total_mse[component] != 0.0)
            {
              total_psnr = 
                10.0*log10((double)MaxVal*MaxVal/total_mse[component]);
              total_snr = 
                10.0*log10(total_signal_power[component]/total_mse[component]);
            }
          if (!value_only_flag)
            {
              if (num_components != 1)
                printf("   %s:\n", component_name);
              switch (DistortionMethod)
                {
                case MSE:
                  printf("%16.6f (MSE)\n", total_mse[component]);
                  break;
                case SNR:
                  printf("%16.6f dB (SNR)\n", total_snr);
                  break;
                case PSNR:
                  printf("%16.6f dB (PSNR)\n", total_psnr);
                  break;
                case MAE:
                  printf("%16.6f (MAE)\n", total_mae[component]);
                  break;
                case ALL:
                default:
                  printf("%16.6f\t(MSE)\n%16.6f dB\t(SNR)\n%16.6f dB\t(PSNR)\n%16.6f\t(MAE)\n",
                         total_mse[component], total_snr, total_psnr,
                         total_mae[component]);
                  break;
                }
            }
          else
            {
              switch (DistortionMethod)
                {
                case MSE:
                  value = total_mse[component];
                  break;
                case SNR:
                  value = total_snr;
                  break;
                case PSNR:
                  value = total_psnr;
                  break;
                case MAE:
                  value = total_mae[component];
                  break;
                }
              printf("%f\n", value);
            }
        }
    }

  QccIMGImageSequenceFree(&ImageSequence1);
  QccIMGImageSequenceFree(&ImageSequence1);

  QccExit;
}
