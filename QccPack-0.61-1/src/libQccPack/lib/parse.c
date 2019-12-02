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

#define QCCPARSEMAXNUMARGUMENTS 1024
#define QCCPARSESTRINGLEN QCCSTRINGLEN


static void QccParseCatChar(char *str, char ch)
{
  int length;

  length = strlen(str);

  if (length > (QCCPARSESTRINGLEN - 1))
    return;

  str[length] = ch;
  str[length + 1] = '\0';
}


static const char *QccParseGetFormat(char *fmt, const char *pnt,
                                     int *multiple_arguments)
{
  int i = 0;

  *multiple_arguments = 0;
  while ((*pnt != '\0') && (*pnt != ':') && (*pnt != ']') && (*pnt != '[') && 
         (*pnt != ' ') && (*pnt != '\n') && (*pnt != '\t') && (*pnt != '\r'))
    {
      if (*pnt == '*')
        *multiple_arguments = 1;
      else
        fmt[i++] = *pnt;
      pnt = &(pnt[1]);
    }
  fmt[i] = '\0';

  return(pnt);
}


static char QccParseGetType(const char *pnt, int *multiple_arguments)
{
  char type = 'd';

  *multiple_arguments = 0;

  while ((*pnt != '\0') && (*pnt != ':') && (*pnt != ']') && (*pnt != '[') && 
         (*pnt != ' ') && (*pnt != '\n') && (*pnt != '\t') && (*pnt != '\r'))
    {
      if (pnt[0] == '*')
        *multiple_arguments = 1;
      else
        type = pnt[0];
      pnt = &(pnt[1]);
    }

  switch (type)
    {
      /*  Unsigned integer  */
    case 'u':
      type = 'u';
      break;

      /*  Floating point  */
    case 'e':
    case 'g':
    case 'f':
      type = 'f';
      break;

      /*  Character string  */
    case 's':
      type = 's';
      break;

      /*  Integer  */
    default:
      type = 'd';
      break;
    }

  if (*multiple_arguments)
    {
      if (strchr(pnt, ']') != NULL)
        {
          QccErrorAddMessage("(QccParseParameters): Multiple argument designation can only be used for non-optional parameters");
          return(0);
        }
      if (strchr(pnt, ' ') != NULL)
        {
          QccErrorAddMessage("(QccParseParameters): Multiple argument designation must be last argument");
          return(0);
        }
    }

  return (type);
}


static char *QccParseFindPosition(const char *format, char *arg, int *pos)
{
  const char *format_orig;
  char *pnt;
  const char *tmp;
  char sw[QCCPARSESTRINGLEN + 1];
  int i, done = 0;;

  format_orig = format;

  do
    {
      sw[0] = '\0';
      /*  Find location of switch  */
      pnt = strstr(format, arg);
      if (pnt != NULL)
        {
          i = 0;
          /*  Extract full switch from format prototype  */
          while ((pnt[i] != '\0') && (pnt[i] != ':') && 
                 (pnt[i] != ']') && (pnt[i] != '[') && (pnt[i] != ' ') && 
                 (pnt[i] != '\n') && (pnt[i] != '\t') && (pnt[i] != '\r'))
            QccParseCatChar(sw, pnt[i++]);
          if (pnt[i] == '\0')
            pnt = NULL;
          /*  Make sure switches match exactly  */
          if (!strcmp(sw, arg))
            done = 1;
          else
            format = &pnt[i];
        }
    }
  while ((pnt != NULL) && (!done));

  /*  Count number of pointer references in prototype to current pos  */
  *pos = 0;
  tmp = format_orig;
  if (pnt != NULL)
    while (tmp != pnt)
      {
        if (tmp[0] == '%')
          {
            (*pos)++;
          }
        tmp = &tmp[1];
      }
  return(pnt);
}


static int QccParseReadParameter(int narg,
                                 char *arg[], 
                                 char *fmt,
                                 int pos,
                                 int *cindex,
                                 int multiple_arguments,
                                 void **parse_pointer,
                                 char *parse_type)
{
  int val = 0;
  int *num_args;
  int **d_ptr;
  unsigned int **u_ptr;
  float **f_ptr;
  QccString **s_ptr;

  if (!multiple_arguments)
    /*  Use type to cast the pointer appropriately  */
    switch (parse_type[pos])
      {
      case 'f':
        val = sscanf(arg[*cindex], fmt, (float *) parse_pointer[pos]);
        break;
      case 'u':
        val = sscanf(arg[*cindex], fmt, (unsigned int *) parse_pointer[pos]);
        break;
      case 's':
        QccConvertToQccString((char *)(parse_pointer[pos]),
                              arg[*cindex]);
        val = strlen((char *) parse_pointer[pos]);
        break;
      default:
        val = sscanf(arg[*cindex], fmt, (int *) parse_pointer[pos]);
        break;
      }
  else
    {
      num_args = (int *)parse_pointer[pos];
      *num_args = 1;
      for ( ; *cindex < narg; (*cindex)++, 
              *num_args += 1)
        switch (parse_type[pos])
          {
          case 'f':
            f_ptr = (float **)parse_pointer[pos + 1];
            if (*num_args == 1)
              {
                if ((*f_ptr = (float *)malloc(sizeof(float))) == NULL)
                  return(0);
              }
            else
              if ((*f_ptr = 
                   (float *)realloc(*f_ptr, sizeof(float)*(*num_args))) == 
                  NULL)
                return(0);
            val = sscanf(arg[*cindex], fmt, &(*f_ptr)[*num_args - 1]);
            if (val != 1)
              return(val);
            break;
          case 'u':
            u_ptr = (unsigned int **)parse_pointer[pos + 1];
            if (*num_args == 1)
              {
                if ((*u_ptr = 
                     (unsigned int *)malloc(sizeof(unsigned int))) == NULL)
                  return(0);
              }
            else
              if ((*u_ptr =
                   (unsigned int *)realloc(*u_ptr, 
                                           sizeof(unsigned int)*(*num_args))) 
                  == NULL)
                return(0);
            val = sscanf(arg[*cindex], fmt, &(*u_ptr)[*num_args - 1]);
            if (val != 1)
              return(val);
            break;
          case 's':
            s_ptr = (QccString **)parse_pointer[pos + 1];
            if (*num_args == 1)
              {
                if ((*s_ptr = (QccString *)malloc(sizeof(QccString))) == NULL)
                  return(0);
              }
            else
              if ((*s_ptr = 
                   (QccString *)realloc(*(s_ptr),
                                        sizeof(QccString) * (*num_args))) ==
                  NULL)
                return(0);
            QccConvertToQccString((*s_ptr)[*num_args - 1], arg[*cindex]);
            val = strlen(arg[*cindex]);
            if (val < 1)
              return(val);
            break;
          case 'd':
            d_ptr = (int **)parse_pointer[pos + 1];
            if (*num_args == 1)
              {
                if ((*d_ptr = (int *)malloc(sizeof(int))) == NULL)
                  return(0);
              }
            else
              if ((*d_ptr =
                   (int *)realloc(*d_ptr, sizeof(int)*(*num_args))) == NULL)
                return(0);
            val = sscanf(arg[*cindex], fmt, &((*d_ptr)[*num_args - 1]));
            if (val != 1)
              return(val);
            break;
          }
      *num_args -= 1;
    }

  return(val);
}


static const char *QccParseStartOfMandatoryParameters(const char *format,
                                                      int *pos)
{
  int i, done;
  const char *format2, *tmp;

  format2 = tmp = format;
  *pos = 0;

  /*  Find first ']' from end of string  */
  /*  This is last of optional parameters  */
  i = strlen(format2) - 1;
  done = 0;
  while ((i > 0) && (!done))
    {
      done = (format2[i] == ']');
      if (!done)
        i--;
    }

  /*  Scan until we find '%'  */
  /*  This is first mandatory parameter  */
  format2 = &format2[i];
  while((format2[0] != '\0') && (format2[0] != '%'))
    format2 = &format2[1];

  /*  Get count of the pointer reference  */
  while (tmp != format2)
    {
      if (tmp[0] == '%')
        (*pos)++;
      tmp = &tmp[1];
    }

  return(format2);
}


static void QccParsePrintUsage(const char *format)
{
  QccString program_name;
  QccString usage_string;

  QccStringMakeNull(program_name);
  QccStringMakeNull(usage_string);

  QccGetProgramName(program_name);

  QccPrintQccPackVersion(stderr);

  sprintf(usage_string, "Usage: %s ", program_name);

  while (format[0] !=  '\0')
    {
      /*  Skip to label  */
      if (format[0] == '%')
        {
          while ((format[0] != ':') && (format[0] != ']') && 
                 (format[0] != '[') && (format[0] != ' ') && 
                 (format[0] != '\n') && (format[0] != '\t') && 
                 (format[0] != '\r'))
            format = &format[1];

          if (format[0] == ':')
            format = &format[1];
        }
      /*  Print labels and switches  */
      if (format[0] != '\0')
        {
          QccParseCatChar(usage_string, format[0]);
          format = &format[1];
        }
    }

  QccErrorAddMessage(usage_string);
}


int QccParseParametersVA(int argc, char *argv[], const char *format,
                         va_list ap)
{
  int done, switch_done;
  const char *format2;
  const char *pnt;
  char fmt[QCCPARSESTRINGLEN + 1];
  int cindex, i, pos;
  int multiple_arguments;
  int return_value = 0;
  void **parse_pointer = NULL;
  char *parse_type = NULL;

  if ((parse_pointer = 
       (void **)calloc(QCCPARSEMAXNUMARGUMENTS, sizeof(void *))) == NULL)
    {    
      QccErrorAddMessage("(QccParseParameters): Error allocating memory");
      goto Error;
    }

  if ((parse_type = 
       (char *)calloc(QCCPARSEMAXNUMARGUMENTS, sizeof(char))) == NULL)
    {    
      QccErrorAddMessage("(QccParseParameters): Error allocating memory");
      goto Error;
    }

  /*  Get pointers  */
  i = 0;
  pnt = &format[0];
  while ((i < QCCPARSEMAXNUMARGUMENTS) && (pnt[0] != '\0'))
    {
      if (pnt[0] == '%')
        {
          parse_type[i] = QccParseGetType(pnt, &multiple_arguments);
          if (multiple_arguments)
            switch (parse_type[i])
              {
              case 'f':
                parse_pointer[i++] = (void *) va_arg(ap, int *);
                parse_pointer[i] = (void *) va_arg(ap, float **);
                *((float **)parse_pointer[i++]) = NULL;
                break;
              case 'u':
                parse_pointer[i++] = (void *) va_arg(ap, int *);
                parse_pointer[i] = (void *) va_arg(ap, unsigned int **);
                *((unsigned int **)parse_pointer[i++]) = NULL;
                break;
              case 's':
                parse_pointer[i++] = (void *) va_arg(ap, int *);
                parse_pointer[i] = (void *) va_arg(ap, char ***);
                *((unsigned char ***)parse_pointer[i++]) = NULL;
                break;
              case 'd':
                parse_pointer[i++] = (void *) va_arg(ap, int *);
                parse_pointer[i] = (void *) va_arg(ap, int **);
                *((int **)parse_pointer[i++]) = NULL;
                break;
              }
          else
            switch (parse_type[i])
              {
              case 'f':
                parse_pointer[i++] = (void *) va_arg(ap, float *);
                break;
              case 'u':
                parse_pointer[i++] = (void *) va_arg(ap, unsigned int *);
                break;
              case 's':
                parse_pointer[i++] = (void *) va_arg(ap, char *);
                break;
              case 'd':
                parse_pointer[i++] = (void *) va_arg(ap, int *);
                break;
              default:
                goto QccParseErrorInUsageStringReturn;
              }
        }
      pnt = &pnt[1];
    }
  if (i >= QCCPARSEMAXNUMARGUMENTS)
    {
      QccErrorAddMessage("(QccParseParameters): Too many arguments");
      goto QccParseErrorInUsageStringReturn;
    }

  /*  Process all switches first  */
  cindex = 1;
  switch_done = 0;
  while ((cindex < argc) && !switch_done)
    {
      /*  If not a switch then done  */
      if (argv[cindex][0] != '-')
        switch_done = 1;
      else
        {
          /*  Find switch in prototype  */
          pnt = QccParseFindPosition(format, argv[cindex], &pos);
          if (pnt == NULL)
            goto QccParseParseErrorReturn;
          cindex++;
          done = 0;
          while(!done)
            {
              /*  Pointer reference  */
              if (pnt[0] == '%')
                {
                  /*  Pointer flag, no associated value  */
                  if (pnt[1] == ':')
                    {
                      /*  Set the flag  */
                      *(int *)parse_pointer[pos++] = 1;
                      pnt = &pnt[2];
                    }
                  else
                    {
                      if (cindex >= argc)
                        goto QccParseParseErrorReturn;
                      /*  Get format for argument  */
                      pnt = QccParseGetFormat(fmt, pnt, &multiple_arguments);
                      /*  Get value of argument  */
                      if (!QccParseReadParameter(argc, argv,
                                                 fmt, pos++, &cindex, 0,
                                                 parse_pointer,
                                                 parse_type))
                        goto QccParseParseErrorReturn;
                      cindex++;
                    }
                }
              else
                {
                  /*  Keep searching till end of switch's parameters  */
                  if ((pnt[0] == '\0')||(pnt[0] == ']')||(pnt[0] == '['))
                    done = 1;
                  else
                    pnt = &pnt[1];
                }
            }
        }
    }

  /*  Now do all mandatory parameters  */
  format2 = QccParseStartOfMandatoryParameters(format, &pos);

  /*  Loop till end of string or out of arguments  */
  while ((format2[0] != '\0') && (cindex < argc))
    {
      /*  Pointer reference  */
      if (format2[0] == '%')
        {
          /*  Get format for argument  */
          format2 = QccParseGetFormat(fmt, format2, &multiple_arguments);
          /*  Get value of argument  */
          if (!QccParseReadParameter(argc, argv,
                                     fmt, pos++, &cindex, multiple_arguments,
                                     parse_pointer, parse_type))
            goto QccParseParseErrorReturn;
        }
      
      cindex++;
      /*  Loop to next pointer reference  */
      while((format2[0] != '\0') && (format2[0] != '%'))
        format2 = &format2[1];
    }

  /*  Check to see if all mandatory paramters have been processed  */
  if ((format2[0] != '\0') || (cindex < argc))
    goto QccParseParseErrorReturn;

  /* Normal error-free return */
  return_value = 0;
  goto Return;
  
 QccParseParseErrorReturn:
  QccParsePrintUsage(format);
  goto Error;

 QccParseErrorInUsageStringReturn:
  QccErrorAddMessage("(QccParseParameters): Error in usage string");
  goto Error;

 Error:
  return_value = 1;

 Return:
  if (parse_pointer != NULL)
    QccFree(parse_pointer);
  if (parse_type != NULL)
    QccFree(parse_type);
  return(return_value);
}


int QccParseParameters(int argc, char *argv[], const char *format, ...)
{
  int return_value;
  va_list ap;
  
  va_start(ap, format);
  
  return_value = QccParseParametersVA(argc, argv, format, ap);

  va_end(ap);

  return(return_value);
}
