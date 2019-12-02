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

#define QCCFILE_READ 1
#define QCCFILE_WRITE 2
#define QCCFILE_PIPETRUE 1
#define QCCFILE_PIPEFALSE 2

typedef struct
{
  FILE *fileptr;
  int access_mode;
  int pipe_type;
} QccFileEntry;

static int QccFileTableLen = 0;
static QccFileEntry *QccFileTable;
#ifdef QCC_USE_PTHREADS
static pthread_mutex_t QccFileTableMutex = PTHREAD_MUTEX_INITIALIZER;
#endif


void QccFileInit(void)
{
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileInit): Error locking mutex");
      QccExit;
    }
#endif
  QccFileTableLen = 0;
  QccFileTable = NULL;
#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileInit): Error unlocking mutex");
      QccExit;
    }
#endif
}


int QccFileExists(const QccString filename)
{
  struct stat statbuf;

  return(!stat(filename, &statbuf));
}


int QccFileGetExtension(const QccString filename, QccString extension)
{
  char *ptr;
  QccString filename2;

  QccStringCopy(filename2, filename);

  if ((ptr = strrchr(filename2, '.')) == NULL)
    {
      QccStringMakeNull(extension);
      return(1);
    }

  if (!strcmp(ptr + 1, "gz"))
    {
      *ptr = '\0';

      if ((ptr = strrchr(filename2, '.')) == NULL)
        {
          QccStringMakeNull(extension);
          return(1);
        }
    }

  QccStringCopy(extension, ptr + 1);

  return(0);
}


static int QccFileTableAddEntry(FILE *fileptr,
                                const int access_mode,
                                const int pipe_flag)
{
  int return_value = 0;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileTableAddEntry): Error locking mutex");
      QccExit;
    }
#endif

  if (QccFileTable == NULL)
    {
      if ((QccFileTable = 
           (QccFileEntry *)malloc(sizeof(QccFileEntry))) == NULL)
        {
          QccErrorAddMessage("(QccFileTableAddEntry): Error allocating memory");
          goto Error;
        }
      QccFileTableLen = 1;
    }
  else
    {
      QccFileTableLen++;
      if ((QccFileTable = 
           (QccFileEntry *)realloc((void *)QccFileTable,
                                   sizeof(QccFileEntry)*QccFileTableLen)) ==
          NULL)
        {
          QccErrorAddMessage("(QccFileTableAddEntry): Error reallocating memory");
          goto Error;
        }
    }

  QccFileTable[QccFileTableLen - 1].fileptr = fileptr;
  QccFileTable[QccFileTableLen - 1].access_mode = access_mode;
  QccFileTable[QccFileTableLen - 1].pipe_type = pipe_flag;

  return_value = 0;
  goto Return;

 Error:
  return_value = 1;

 Return:

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileTableAddEntry): Error unlocking mutex");
      QccExit;
    }
#endif

  return(return_value);
}


static int QccFileGetPipeType(FILE *fileptr)
{
  int file_cnt;
  int return_value = 0;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileGetPipeType): Error locking mutex");
      QccExit;
    }
#endif

  for (file_cnt = 0; file_cnt < QccFileTableLen; file_cnt++)
    if (QccFileTable[file_cnt].fileptr == fileptr)
      {
        return_value = QccFileTable[file_cnt].pipe_type;
        break;
      }

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileGetPipeType): Error unlocking mutex");
      QccExit;
    }
#endif

  return(return_value);
}


static int QccFileTableRemoveEntry(FILE *fileptr)
{
  int file_cnt, file_cnt2, file_cnt3;
  QccFileEntry *tmp_table;
  int return_value = 0;

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_lock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileTableRemoveEntry): Error locking mutex");
      QccExit;
    }
#endif

  for (file_cnt = 0; file_cnt < QccFileTableLen; file_cnt++)
    if (QccFileTable[file_cnt].fileptr == fileptr)
      break;

  if (file_cnt >= QccFileTableLen)
    {
      QccErrorAddMessage("(QccFileTableRemoveEntry): fileptr is not in table");
      goto Error;
    }

  if (QccFileTableLen == 1)
    {
      QccFree(QccFileTable);
      QccFileTableLen = 0;
      QccFileTable = NULL;
      goto Return;
    }

  if ((tmp_table = 
       (QccFileEntry *)malloc(sizeof(QccFileEntry)*(QccFileTableLen - 1))) == 
      NULL)
    {
      QccErrorAddMessage("(QccFileTableRemoveEntry): Error allocating memory");
      goto Error;
    }

  for (file_cnt2 = 0, file_cnt3 = 0; file_cnt2 < QccFileTableLen;
       file_cnt2++)
    if (file_cnt2 != file_cnt)
      tmp_table[file_cnt3++] = QccFileTable[file_cnt2];

  QccFree(QccFileTable);
  QccFileTable = tmp_table;
  QccFileTableLen--;

  return_value = 0;
  goto Return;

 Error:
  return_value = 1;

 Return:

#ifdef QCC_USE_PTHREADS
  if (pthread_mutex_unlock(&QccFileTableMutex))
    {
      QccErrorAddMessage("(QccFileTableRemoveEntry): Error unlocking mutex");
      QccExit;
    }
#endif

  return(return_value);
}


static FILE *QccFileOpenRead(const QccString filename, int *pipe_flag)
{
  int len;
  QccString cmd;
  FILE *infile = NULL;

  *pipe_flag = QCCFILE_PIPEFALSE;
  if (!QccFileExists(filename))
    {
      QccErrorAddMessage("(QccFileOpenRead): file %s not found",
                         filename);
      return(NULL);
    }

  if ((infile = fopen(filename, "rb")) == NULL)
    {
      QccErrorAddMessage("(QccFileOpenRead): Unable to open %s for reading",
                         filename);
      return(NULL);
    }
  len = strlen(filename);

  if ((!strcmp(&filename[len-2], ".Z")) ||
      (!strcmp(&filename[len-3], ".gz")))
    {
      fclose(infile);
      QccStringSprintf(cmd, "%s -q -c %s",
                       QCCMAKESTRING(QCCUNCOMPRESS),
                       filename);
      if ((infile = popen(cmd, "r")) == NULL)
        QccErrorAddMessage("(QccFileOpenRead): unable to open pipe for decompression");
      *pipe_flag = QCCFILE_PIPETRUE;
    }
  
  return(infile);
}


static FILE *QccFileOpenWrite(const QccString filename, int *pipe_flag)
{
  int len;
  QccString cmd;
  FILE *outfile = NULL;

  len = strlen(filename);

  *pipe_flag = QCCFILE_PIPEFALSE;
  if ((!strcmp(&filename[len-2], ".Z")) ||
      (!strcmp(&filename[len-3], ".gz")))
    {
      QccStringSprintf(cmd, "%s > %s",
                       QCCMAKESTRING(QCCCOMPRESS),
                       filename);
      if ((outfile = popen(cmd, "w")) == NULL)
        {
          QccErrorAddMessage("(QccFileOpenWrite): unable to open pipe for compression");
        }
      *pipe_flag = QCCFILE_PIPETRUE;
    }
  else
    {
      if ((outfile = fopen(filename, "wb")) == NULL)
        QccErrorAddMessage("(QccFileOpenWrite): unable to open %s for writing",
                           filename);
    }
  
  return(outfile);
}


FILE *QccFileOpen(const QccString filename, const QccString mode)
{
  FILE *fileptr = NULL;
  int pipe_flag;
  int access_mode;

  if (mode == NULL)
    {
      QccErrorAddMessage("(QccFileOpen): Error invalid open mode");
      return(NULL);
    }

  if (mode[0] == 'r')
    {
      if ((fileptr = QccFileOpenRead(filename, &pipe_flag)) == NULL)
        {
          QccErrorAddMessage("(QccFileOpen): Error calling QccFileOpenRead()");
          return(NULL);
        }

      access_mode = QCCFILE_READ;
    }
  else
    if (mode[0] == 'w')
      {
        if ((fileptr = QccFileOpenWrite(filename, &pipe_flag)) == NULL)
          {
            QccErrorAddMessage("(QccFileOpen): Error calling QccFileOpenWrite()");
            return(NULL);
          }

        access_mode = QCCFILE_WRITE;
      }
    else
      {
        QccErrorAddMessage("(QccFileOpen): Open mode %s is invalid",
                           mode);
        return(NULL);
      }

  if (QccFileTableAddEntry(fileptr, access_mode, pipe_flag))
    {
      QccErrorAddMessage("(QccFileOpen): Error calling QccFileTableAddEntry()");
      return(NULL);
    }

  return(fileptr);
}


FILE *QccFileDescriptorOpen(int file_descriptor, const QccString mode)
{
  FILE *fileptr = NULL;
  int pipe_flag;
  int access_mode;

  pipe_flag = QCCFILE_PIPEFALSE;

  if (mode == NULL)
    {
      QccErrorAddMessage("(QccFileDescriptorOpen): Error invalid open mode");
      return(NULL);
    }

  if (mode[0] == 'r')
    {
      if ((fileptr = fdopen(file_descriptor, "r")) == NULL)
        {
          QccErrorAddMessage("(QccFileDescriptorOpen): Error opening file descriptor for reading()");
          return(NULL);
        }

      access_mode = QCCFILE_READ;
    }
  else
    if (mode[0] == 'w')
      {
        if ((fileptr = fdopen(file_descriptor, "w")) == NULL)
          {
            QccErrorAddMessage("(QccFileDescriptorOpen): Error opening file descriptor for writing()");
            return(NULL);
          }

        access_mode = QCCFILE_WRITE;
      }
    else
      {
        QccErrorAddMessage("(QccFileDescriptorOpen): Open mode %s is invalid",
                           mode);
        return(NULL);
      }

  if (QccFileTableAddEntry(fileptr, access_mode, pipe_flag))
    {
      QccErrorAddMessage("(QccFileDescriptorOpen): Error calling QccFileTableAddEntry()");
      return(NULL);
    }

  return(fileptr);
}


int QccFileClose(FILE *fileptr)
{
  int pipe_flag;

  if (fileptr == NULL)
    return(0);

  if (!(pipe_flag = QccFileGetPipeType(fileptr)))
    {
      QccErrorAddMessage("(QccFileClose): fileptr is not in QccFileTable");
      return(1);
    }

  if (pipe_flag == QCCFILE_PIPETRUE)
    pclose(fileptr);
  else
    fclose(fileptr);

  if (QccFileTableRemoveEntry(fileptr))
    {
      QccErrorAddMessage("(QccFileClose): Error calling QccFileTableRemoveEntry()");
      return(1);
    }

  return(0);
}


int QccFileSeekable(FILE *fileptr)
{
  if (ftell(fileptr) == -1)
    return(0);
  return(1);
}


int QccFileFlush(FILE *fileptr)
{
  if (fflush(fileptr))
    {
      QccErrorAddMessage("(QccFileFlush): Error flushing file");
      return(1);
    }

  return(0);
}


int QccFileRemove(const QccString filename)
{
  if (filename == NULL)
    return(0);

  if (remove(filename) < 0)
    {
      QccErrorAddMessage("(QccFileRemove): Error removing %s", filename);
      return(1);
    }

  return(0);
}


int QccFileGetSize(const QccString filename,
                   FILE *fileptr,
                   long *filesize)
{
  struct stat statbuf;

  if ((filename == NULL) && (fileptr == NULL))
    return(0);
  if (filesize == NULL)
    return(0);

  if (filename == NULL)
    {
      if (QccFileGetPipeType(fileptr) == QCCFILE_PIPETRUE)
        {
          QccErrorAddMessage("(QccFileGetSize): cannot determine size of compressed file from fileptr");
          return(1);
        }
      if (fstat(fileno(fileptr), &statbuf))
        {
          QccErrorAddMessage("(QccFileGetSize): file not found");
          return(1);
        }
    }
  else
    if (stat(filename, &statbuf))
      {
        QccErrorAddMessage("(QccFileGetSize): file %s not found", filename);
        return(1);
      }
  
  *filesize = statbuf.st_size;

  return(0);
}


int QccFileGetModTime(const QccString filename,
                      FILE *fileptr,
                      long *modtime)
{
  struct stat statbuf;

  if ((filename == NULL) && (fileptr == NULL))
    return(0);
  if (modtime == NULL)
    return(0);

  if (filename == NULL)
    {
      if (QccFileGetPipeType(fileptr) == QCCFILE_PIPETRUE)
        {
          QccErrorAddMessage("(QccFileGetModTime): cannot determine modtime of compressed file from fileptr");
          return(1);
        }
      if (fstat(fileno(fileptr), &statbuf))
        {
          QccErrorAddMessage("(QccFileGetModTime): file not found");
          return(1);
        }
    }
  else
    if (stat(filename, &statbuf))
      {
        QccErrorAddMessage("(QccFileGetModTime): file %s not found", filename);
        return(1);
      }
  
  *modtime = statbuf.st_mtime;

  return(0);
}


int QccFileGetRealPath(const QccString filename,
                       QccString path)
{
  char *path2 = NULL;
  int return_value = 0;
  
  if (filename == NULL)
    return(0);

  if ((path2 = realpath(filename, NULL)) == NULL)
    {
      QccErrorAddMessage("(QccFileGetRealPath): Error calling realpath()");
      goto Error;
    }       

  if (strlen(path2) > (QCCSTRINGLEN - 1))
    {
      QccErrorAddMessage("(QccFileGetRealPath): Path too long");
      goto Error;
    }       

  QccConvertToQccString(path, path2);
  
  return_value = 0;
  goto Return;
 Error:
  return_value = 1;
 Return:
  if (path2 != NULL)
    free(path2);
  return(return_value);
}


int QccFileGetCurrentPosition(FILE *infile, long *current_position)
{
  if (infile == NULL)
    return(0);
  if (current_position == NULL)
    return(0);

  if (QccFileGetPipeType(infile) == QCCFILE_PIPETRUE)
    {
      QccErrorAddMessage("(QccFileGetCurrentPosition): cannot determine position of compressed file");
      return(1);
    }

  *current_position = ftell(infile);
  if (ferror(infile))
    {
      QccErrorAddMessage("(QccFileGetCurrentPosition): error accessing file");
      return(1);
    }

  return(0);
}


int QccFileRewind(FILE *infile)
{
  if (infile == NULL)
    return(0);

  if (QccFileGetPipeType(infile) == QCCFILE_PIPETRUE)
    {
      QccErrorAddMessage("(QccFileRewind): cannot rewind a compressed file");
      return(1);
    }

  rewind(infile);
  if (ferror(infile))
    {
      QccErrorAddMessage("(QccFileRewind): error accessing file");
      return(1);
    }

  return(0);
}


int QccFileReadChar(FILE *infile, char *val)
{
  int read_val;

  if (infile == NULL)
    return(0);

  read_val = fgetc(infile);
  if (val != NULL)
    *val = (char)read_val;

  return(ferror(infile) || feof(infile));
}


int QccFileWriteChar(FILE *outfile, char val)
{
  if (outfile == NULL)
    return(0);

  fputc((int)val, outfile);
  return(ferror(outfile));
}


int QccFileReadInt(FILE *infile, int *val)
{
  unsigned char ch[QCC_INT_SIZE];

  if (infile == NULL)
    return(0);

  fread(ch, 1, QCC_INT_SIZE, infile);
  if (ferror(infile) || feof(infile))
    return(1);

  if (val != NULL)
    QccBinaryCharToInt(ch, (unsigned int *)val);

  return(0);
}


int QccFileWriteInt(FILE *outfile, int val)
{
  unsigned char ch[QCC_INT_SIZE];

  if (outfile == NULL)
    return(0);

  QccBinaryIntToChar((unsigned int)val, ch);

  fwrite(ch, 1, QCC_INT_SIZE, outfile);

  return(ferror(outfile));
}


int QccFileReadDouble(FILE *infile, double *val)
{
  unsigned char ch[QCC_INT_SIZE];
  float tmp;

  if (infile == NULL)
    return(0);

  fread(ch, 1, QCC_INT_SIZE, infile);
  if (ferror(infile) || feof(infile))
    return(1);

  if (val != NULL)
    {
      QccBinaryCharToFloat(ch, &tmp);
      
      if (val != NULL)
        *val = tmp;
    }

  return(0);
}


int QccFileWriteDouble(FILE *outfile, double val)
{
  unsigned char ch[QCC_INT_SIZE];
  float tmp;

  if (outfile == NULL)
    return(0);

  tmp = val;
  QccBinaryFloatToChar(tmp, ch);
  fwrite(ch, 1, QCC_INT_SIZE, outfile);
  return(ferror(outfile));

}


int QccFileReadString(FILE *infile, QccString s)
{
  QccString cmd;

  if (infile == NULL)
    return(0);

  if (s == NULL)
    return(0);

  QccStringSprintf(cmd, "%%%ds", QCCSTRINGLEN);
  fscanf(infile, cmd, s);

  return(ferror(infile) || feof(infile));
}


int QccFileWriteString(FILE *outfile, const QccString s)
{
  if (outfile == NULL)
    return(0);

  if (s == NULL)
    return(0);

  fprintf(outfile, "%s", s);

  return(ferror(outfile));
}


int QccFileReadLine(FILE *infile, QccString s)
{
  char ch;
  int len;

  for (len = 0; len < QCCSTRINGLEN; len++)
    {
      if (QccFileReadChar(infile, &ch))
        {
          QccErrorAddMessage("(QccFileReadLine): Error calling QccFileReadChar()");
          return(1);
        }
      if (ch != '\n')
        s[len] = ch;
      else
        break;
    }

  s[len] = '\0';
  
  if (len >= QCCSTRINGLEN)
    do
      {
        if (QccFileReadChar(infile, &ch))
          {
            QccErrorAddMessage("(QccFileReadLine): Error calling QccFileReadChar()");
            return(1);
          }
      }
    while (ch != '\n');
  
  return(0);
}

static int QccFileSkipComment(FILE *infile)
{
  unsigned char ch;

  if (infile == NULL)
    return(0);

  do
    {
      ch = fgetc(infile);
      if (ferror(infile))
        return(1);
    }
  while (ch != '\n');

  ungetc(ch, infile);

  return(0);
}


int QccFileSkipWhiteSpace(FILE *infile, int skip_comments_flag)
{
  unsigned char ch;

  if (infile == NULL)
    return(0);

  do
    {
      ch = fgetc(infile);
      if (ferror(infile))
        return(1);

      if (((ch == '#') || (ch == '*') || (ch == '/'))
          && skip_comments_flag)
        {
          if (QccFileSkipComment(infile))
            return(1);
          ch = fgetc(infile);
          if (ferror(infile))
            return(1);
        }
    }
  while ((ch == ' ') || (ch == '\n') || (ch == '\t') || (ch == '\r'));

  ungetc(ch, infile);
  return(0);

}


int QccFileReadMagicNumber(FILE *infile, QccString magic_num,
                           int *major_version_number,
                           int *minor_version_number)
{
  QccString header_value;
  int major, minor;

  if (infile == NULL)
    return(0);

  if (magic_num == NULL)
    return(1);

  if (QccFileReadString(infile, header_value))
    return(1);
  if (QccFileSkipWhiteSpace(infile, 1))
    return(1);

  sscanf(header_value, "%[^0-9]%d.%d", magic_num, &major, &minor);

  if (major_version_number != NULL)
    *major_version_number = major;
  if (minor_version_number != NULL)
    *minor_version_number = minor;

  return(0);
}


int QccFileWriteMagicNumber(FILE *outfile, const QccString magic_num)
{
  int major;
  int minor;

  if (outfile == NULL)
    return(0);

  QccGetQccPackVersion(&major, &minor, NULL);

  if (QccFileWriteMagicNumberVersion(outfile,
                                     magic_num,
                                     major,
                                     minor))
    {
      QccErrorAddMessage("(QccFileWriteMagicNumber): Error calling QccFileWriteMagicNumberVersion()");
      goto QccErr;
    }

  return(0);

 QccErr:
  return(1);
}


int QccFileWriteMagicNumberVersion(FILE *outfile,
                                   const QccString magic_num,
                                   int major_version_number,
                                   int minor_version_number)
{
  QccString header_value;

  if (outfile == NULL)
    return(0);

  QccStringSprintf(header_value, "%s%d.%d\n",
                   magic_num, major_version_number, minor_version_number);

  if (QccFileWriteString(outfile, header_value))
    {
      QccErrorAddMessage("(QccFileWriteMagicNumberVersion): Error writing magic number");
      goto QccErr;
    }

  return(0);

 QccErr:
  return(1);
}


int QccFileGetMagicNumber(const QccString filename, QccString magic_num)
{
  FILE *infile;

  if (filename == NULL)
    return(0);
  if (magic_num == NULL)
    return (0);

  if ((infile = QccFileOpen(filename, "r")) == NULL)
    {
      QccErrorAddMessage("(QccFileGetMagicNumber): Error opening %s for reading",
                         filename);
      return(1);
    }
  
  if (QccFileReadMagicNumber(infile, magic_num,
                             NULL, NULL))
    {
      QccErrorAddMessage("(QccFileGetMagicNumber): Error reading magic number of %s",
                         filename);
      return(1);
    }
  
  QccFileClose(infile);
  
  return(0);
}


int QccFilePrintFileInfo(const QccString filename,
                         const QccString magic_num,
                         int major_version,
                         int minor_version)
{
  long filesize, modtime;
  char *modtime_string;

  printf("---------------------------------------------------------------------------\n\n");

  if (filename != NULL)
    if (strlen(filename))
      printf("   File: %s\n", filename);

  if (magic_num != NULL)
    if (strlen(magic_num))
      {
        printf("   Format: %s (", magic_num);

        if (!strcmp(magic_num, QCCDATASET_MAGICNUM))
          printf("dataset");
        else if (!strcmp(magic_num, QCCCHANNEL_MAGICNUM))
          printf("channel");
        else if (!strcmp(magic_num, QCCSQSCALARQUANTIZER_MAGICNUM))
          printf("scalar quantizer");
        else if (!strcmp(magic_num, QCCVQCODEBOOK_MAGICNUM))
          printf("VQ codebook");
        else if (!strcmp(magic_num, QCCVQMULTISTAGECODEBOOK_MAGICNUM))
          printf("Multistage/Residual VQ codebook");
        else if (!strcmp(magic_num, QCCAVQSIDEINFO_MAGICNUM))
          printf("side information");
        else if (!strcmp(magic_num, QCCIMGIMAGECOMPONENT_MAGICNUM))
          printf("image component");
        else if (!strcmp(magic_num, QCCIMGIMAGECUBE_MAGICNUM))
          printf("3D image cube");
        else if (!strcmp(magic_num, QCCWAVFILTERBANK_MAGICNUM))
          printf("wavelet filter bank");
        else if (!strcmp(magic_num, QCCWAVVECTORFILTERBANK_MAGICNUM))
          printf("vector wavelet filter bank");
        else if (!strcmp(magic_num, QCCWAVLIFTINGSCHEME_MAGICNUM))
          printf("wavelet lifting scheme");
        else if (!strcmp(magic_num, QCCWAVPERCEPTUALWEIGHTS_MAGICNUM))
          printf("perceptual weights");
        else if (!strcmp(magic_num, QCCWAVSUBBANDPYRAMID_MAGICNUM))
          printf("subband pyramid");
        else if (!strcmp(magic_num, QCCWAVSUBBANDPYRAMIDINT_MAGICNUM))
          printf("integer-valued subband pyramid");
        else if (!strcmp(magic_num, QCCWAVSUBBANDPYRAMID3D_MAGICNUM))
          printf("3D subband pyramid");
        else if (!strcmp(magic_num, QCCWAVSUBBANDPYRAMID3DINT_MAGICNUM))
          printf("integer-valued 3D subband pyramid");
        else if (!strcmp(magic_num, QCCWAVZEROTREE_MAGICNUM))
          printf("zerotree");
        else if (!strcmp(magic_num, QCCHYPKLT_MAGICNUM))
          printf("KLT transform matrix");
        else
          printf("?");

        printf(")\n");
      }

  if ((major_version >= 0) && (minor_version >= 0))
    printf("   Created by: QccPack Version %d.%d\n",
           major_version, minor_version);

  if (QccFileExists(filename))
    {
      QccFileGetSize(filename, NULL, &filesize);
      QccFileGetModTime(filename, NULL, &modtime);
      modtime_string = ctime((time_t *)(&modtime));
      printf("   Time: %s", modtime_string);
      printf("   Size: %ld bytes\n", filesize);
    }

  printf("\n---------------------------------------------------------------------------\n\n");

  return(0);
}
