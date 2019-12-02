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


typedef glob_t QccFilePathGlobStruct;


static int QccFilePathGlob(QccString pattern,
                           int flag,
                           QccFilePathGlobStruct *glob_struct)
{
  if (pattern == NULL)
    return(0);
  if (glob_struct == NULL)
    return(0);
  if (QccStringNull(pattern))
    return(0);

  if (glob((char *)pattern,
           flag,
           NULL,
           (glob_t *)(glob_struct)))
    {
      QccErrorAddMessage("(QccFilePathGlob): Error calling glob()");
      return(1);
    }

  return(0);
}


static int QccFilePathMatch(const QccString filename,
                            const char *path)
{
  char *filename2;
  int match = 0;

  if ((filename2 = strrchr(path, (int)'/')) == NULL)
    match = !strncmp((char *)filename, path, QCCSTRINGLEN);
  else
    match = !strncmp((char *)filename, &filename2[1], QCCSTRINGLEN);

  return(match);
}


static void QccFilePathGlobFree(QccFilePathGlobStruct *glob_struct)
{
  if (glob_struct == NULL)
    return;

  globfree(glob_struct);
}


int QccFilePathSearch(const QccString pathlist,
                      const QccString filename,
                      QccString found_pathname)
{
  int return_value = 0;
  QccString pathlist2;
  char *current_path;
  char *end_of_path;
  int current_path_length;
  QccString search_path;
  int first_path = 1;
  QccFilePathGlobStruct glob_struct;
  unsigned int current_file;

  if (pathlist == NULL)
    return(0);
  if (filename == NULL)
    return(0);
  if (found_pathname == NULL)
    return(0);

  QccStringMakeNull(found_pathname);

  QccStringCopy(pathlist2, pathlist);
  end_of_path = &pathlist2[strlen(pathlist2)];

  current_path = (char *)pathlist2;

  while (current_path < end_of_path)
    {
      current_path_length = strcspn(current_path, ":");
      current_path[current_path_length] = '\0';
      QccStringCopy(search_path, current_path);
      current_path = &current_path[current_path_length + 1];

      if (strlen(search_path) > QCCSTRINGLEN - 2)
        {
          QccErrorAddMessage("(QccFilePathSearch): Path is too long");
          goto Error;
        }

      if (search_path[strlen(search_path) - 1] == '/')
        search_path[strlen(search_path) - 1] = '\0';
      if (QccFileExists(search_path))
        {
          strcat(search_path, "/*");
          
          if (QccFilePathGlob(search_path,
                              ((first_path) ? 0 : GLOB_APPEND),
                              &glob_struct) == 1)
            {
              QccErrorAddMessage("(QccFilePathSearch): Error calling QccFilePathGlob()");
              goto Error;
            }
          first_path = 0;
        }
    }
  
  if (strlen(filename) > QCCSTRINGLEN - 2)
    {
      QccErrorAddMessage("(QccFilePathSearch): Filename is too long");
      goto Error;
    }

  for (current_file = 0; current_file < glob_struct.gl_pathc;
       current_file++)
    if (QccFilePathMatch(filename,
                         glob_struct.gl_pathv[current_file]))
      {
        QccConvertToQccString(found_pathname,
                              glob_struct.gl_pathv[current_file]);
        goto Return;
      }

  QccErrorAddMessage("(QccFilePathSearch): file %s not found in path",
                     filename);
 Error:
  return_value = 1;
 Return:
  if (!first_path)
    QccFilePathGlobFree(&glob_struct);
  return(return_value);
}


FILE *QccFilePathSearchOpenRead(QccString filename,
                                const char *environment_path_variable,
                                const char *default_path_list)
{
  QccString pathlist;
  QccString found_pathname;
  FILE *infile = NULL;

  if (filename == NULL)
    return(NULL);

  if (filename[0] != '/')
    {
      if (environment_path_variable == NULL)
        {
          if (default_path_list == NULL)
            QccConvertToQccString(pathlist, ".");
          else
            QccConvertToQccString(pathlist, default_path_list);
        }
      else
        {
          if (QccGetEnv(environment_path_variable,
                        pathlist))
            {
              if (default_path_list == NULL)
                QccConvertToQccString(pathlist, ".");
              else
                QccConvertToQccString(pathlist, default_path_list);
            }
        }

      if (QccFilePathSearch(pathlist,
                            filename,
                            found_pathname))
        {
          QccErrorAddMessage("(QccFilePathSearchOpenRead): Error calling QccFilePathSearch()");
          return(NULL);
        }

      QccStringCopy(filename, found_pathname);
    }
  
  if ((infile = QccFileOpen(filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccFilePathSearchOpenRead): Error calling QccFileOpen()");
      return(NULL);
    }
  
  return(infile);
}
