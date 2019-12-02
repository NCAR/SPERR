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

static QccErrorStruct QccError;
#ifdef QCC_USE_PTHREADS
static pthread_mutex_t QccErrorMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


void QccErrorInit(void)
{
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorInit): Error locking mutex\n");
      exit(1);
    }
#endif

  QccError.current_message[0] = '\0';
  QccError.num_messages = 0;
  QccError.error_messages = NULL;
  QccError.errcode = 0;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorInit): Error unlocking mutex\n");
      exit(1);
    }
#endif

}


static void QccErrorPrint(QccString message)
{
  QccString message_line;
  QccString tmp;
  int line_length;
  int message_line_length = 0;
  char *ptr;
  char *current_message_line;
  int first_line;

  line_length = QCCERROR_OUTPUTLINELENGTH;
  first_line = 1;
  QccStringSprintf(message_line, "%s", (char *)message);
  current_message_line = (char *)message_line;
  if ((int)strlen(current_message_line) > line_length)
    do
      {
        QccStringCopy(tmp, current_message_line);
        do
          {
            ptr = strrchr((char *)tmp, ' ');
            if (ptr == NULL)
              {
                if (first_line)
                  {
                    fprintf(stderr, "%s\n", (char *)current_message_line);
                    first_line = 0;
                    line_length -= QCCERROR_OUTPUTTABLENGTH;
                  }
                else
                  fprintf(stderr, "\t%s\n", (char *)current_message_line);
                current_message_line = 
                  (&current_message_line[strlen(current_message_line)]);
                break;
              }
            message_line_length = 
              (int)(ptr - (char *)tmp) + 1;
            ptr[0] = '\0';
          }
        while (message_line_length >= line_length);
        if (strlen(tmp))
          {
            if (first_line)
              {
                fprintf(stderr, "%s\n", tmp);
                first_line = 0;
                line_length -= QCCERROR_OUTPUTTABLENGTH;
              }
            else
              fprintf(stderr, "\t%s\n", tmp);
            current_message_line = 
              (char *)(&current_message_line[message_line_length]);
          }
      }
    while ((int)strlen(current_message_line) > line_length);
  if (strlen(current_message_line))
    {
      if (first_line)
        {
          fprintf(stderr, "%s\n", (char *)current_message_line);
          first_line = 0;
          line_length -= QCCERROR_OUTPUTTABLENGTH;
        }
      else
        fprintf(stderr, "\t%s\n", (char *)current_message_line);
    }
}


void QccErrorAddMessageVA(const char *format, va_list ap)
{
  QccString error_message;

#ifdef QCC_NO_SNPRINTF
  vsprintf(error_message, format, ap);
#else
  vsnprintf(error_message, QCCSTRINGLEN, format, ap);
#endif

  error_message[QCCSTRINGLEN] = '\0';

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorAddMessageVA): Error locking mutex\n");
      exit(1);
    }
#endif

  QccStringSprintf(QccError.current_message, "*ERROR* %s",
                   error_message);
  QccError.current_message[QCCSTRINGLEN] = '\0';

  if (QccError.num_messages == 0)
    {
      if ((QccError.error_messages = 
           (QccString *)malloc(sizeof(QccString))) == NULL)
        {
          fprintf(stderr, "(QccErrorAddMessageVA): Error allocating memory while processing message:\n");
          fprintf(stderr, "'%s'\n", QccError.current_message);
          goto Error;
        }
      QccStringCopy(QccError.error_messages[0], QccError.current_message);
      QccError.num_messages = 1;
    }
  else
    {
      QccError.num_messages++;
      if ((QccError.error_messages =
           (QccString *)realloc(QccError.error_messages,
                                sizeof(QccString)*QccError.num_messages)) == NULL)
        {
          fprintf(stderr, "(QccErrorAddMessageVA): Error reallocating memory while processing message:\n");
          fprintf(stderr, "'%s'\n", QccError.current_message);
          goto Error;
        }
      QccStringCopy(QccError.error_messages[QccError.num_messages-1], 
             QccError.current_message);
    }

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorAddMessageVA): Error unlocking mutex\n");
      exit(1);
    }
#endif
  return;

Error:
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorAddMessageVA): Error unlocking mutex\n");
      exit(1);
    }
#endif
  exit(1);
}


void QccErrorAddMessage(const char *format, ...)
{
  va_list ap;

  va_start(ap, format);

  QccErrorAddMessageVA(format, ap);

  va_end(ap);
}


void QccErrorWarningVA(const char *format, va_list ap)
{
  QccString warning_message;
  QccString warning_message2;

#ifdef QCC_NO_SNPRINTF
  vsprintf(warning_message, format, ap);
#else
  vsnprintf(warning_message, QCCSTRINGLEN, format, ap);
#endif

  warning_message[QCCSTRINGLEN] = '\0';

  QccStringSprintf(warning_message2, "*WARNING* %s", warning_message);
  warning_message2[QCCSTRINGLEN] = '\0';

  QccErrorPrint(warning_message2);

}


void QccErrorWarning(const char *format, ...)
{
  va_list ap;

  va_start(ap, format);

  QccErrorWarningVA(format, ap);

  va_end(ap);
}


void QccErrorPrintMessages(void)
{
  int error;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorPrintMessages): Error locking mutex\n");
      exit(1);
    }
#endif

  for (error = 0; error < QccError.num_messages; error++)
    QccErrorPrint(QccError.error_messages[error]);

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorPrintMessages): Error unlocking mutex\n");
      exit(1);
    }
#endif

  QccErrorClearMessages();
}


void QccErrorClearMessages(void)
{
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorClearMessages): Error locking mutex\n");
      exit(1);
    }
#endif

  if (QccError.error_messages != NULL)
    QccFree(QccError.error_messages);

  QccError.current_message[0] = '\0';
  QccError.num_messages = 0;
  QccError.error_messages = NULL;
  QccError.errcode = 0;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccErrorMutex))
    {
      fprintf(stderr, "(QccErrorClearMessages): Error unlocking mutex\n");
      exit(1);
    }
#endif

}


void QccErrorExit(void)
{
  QccErrorPrintMessages();
  exit(QCCEXIT_ERROR);
}




