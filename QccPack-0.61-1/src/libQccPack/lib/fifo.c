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


void QccFifoInitialize(QccFifo *fifo)
{
  if (fifo == NULL)
    return;
  
  QccBitBufferInitialize(&fifo->output_buffer);
  QccBitBufferInitialize(&fifo->input_buffer);
}


int QccFifoStart(QccFifo *fifo)
{
  int fd[2];
  
  if (fifo == NULL)
    return(0);
  
  if (pipe(fd))
    {
      QccErrorAddMessage("(QccFifoStart): Error creating pipe");
      return(1);
    }
  
  if (fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK))
    {
      QccErrorAddMessage("(QccFifoStart): Error changing pipe to nonblocking");
      return(1);
    }

  if (fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK))
    {
      QccErrorAddMessage("(QccFifoStart): Error changing pipe to nonblocking");
      return(1);
    }

  if ((fifo->output_buffer.fileptr =
       QccFileDescriptorOpen(fd[1], "w")) == NULL)
    {
      QccErrorAddMessage("(QccFifoStart): Error creating write stream for pipe");
      return(1);
    }
  
  if ((fifo->input_buffer.fileptr =
       QccFileDescriptorOpen(fd[0], "r")) == NULL)
    {
      QccErrorAddMessage("(QccFifoStart): Error creating read stream for pipe");
      return(1);
    }
  
  fifo->output_buffer.type = QCCBITBUFFER_OUTPUT;
  if (QccBitBufferStart(&fifo->output_buffer))
    {
      QccErrorAddMessage("(QccFifoStart): Error calling QccBitBufferStart()");
      return(1);
    }
  
  fifo->input_buffer.type = QCCBITBUFFER_INPUT;
  if (QccBitBufferStart(&fifo->input_buffer))
    {
      QccErrorAddMessage("(QccFifoStart): Error calling QccBitBufferStart()");
      return(1);
    }
  
  return(0);
}


int QccFifoEnd(QccFifo *fifo)
{
  if (fifo == NULL)
    return(0);

  if (QccBitBufferEnd(&fifo->output_buffer))
    {
      QccErrorAddMessage("(QccFifoEnd): Error calling QccBitBufferEnd()");
      return(1);
    }

  if (QccBitBufferEnd(&fifo->input_buffer))
    {
      QccErrorAddMessage("(QccFifoEnd): Error calling QccBitBufferEnd()");
      return(1);
    }

  return(0);
}


int QccFifoFlush(QccFifo *fifo)
{
  if (fifo == NULL)
    return(0);

  if (QccBitBufferFlush(&fifo->output_buffer))
    {
      QccErrorAddMessage("(QccFifoEnd): Error calling QccBitBufferFlush()");
      return(1);
    }

  if (QccBitBufferFlush(&fifo->input_buffer))
    {
      QccErrorAddMessage("(QccFifoEnd): Error calling QccBitBufferFlush()");
      return(1);
    }

  return(0);
}


int QccFifoRestart(QccFifo *fifo)
{
  if (fifo == NULL)
    return(0);

  if (QccFifoEnd(fifo))
    {
      QccErrorAddMessage("(QccFifoRestart): Error calling QccFifoEnd()");
      return(1);
    }

  QccFifoInitialize(fifo);

  if (QccFifoStart(fifo))
    {
      QccErrorAddMessage("(QccFifoRestart): Error calling QccFifoStart()");
      return(1);
    }

  return(0);
}
