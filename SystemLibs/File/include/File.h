   
#ifndef FILE_H
#define FILE_H

//#include <windows.h> // win32
#include "WindowsWrapper.h"

// Make the assumption of c-char strings, not UNICODE
// 32 bit files, not supporting 64 bits


enum class FileMode
{
   READ = 0x7A000000,
   WRITE,
   READ_WRITE
};

enum class FileSeek
{
   BEGIN = 0x7B000000,
   CURRENT,
   END
};

enum class FileError
{
   SUCCESS = 0x7C000000,
   OPEN_FAIL,
   CLOSE_FAIL,
   WRITE_FAIL,
   READ_FAIL,
   SEEK_FAIL,
   TELL_FAIL,
   FLUSH_FAIL
};

typedef HANDLE FileHandle;

class File
{
public:

   static FileError open( FileHandle &fh, const char * const fileName, FileMode mode );
   static FileError close( FileHandle fh );
   static FileError write( FileHandle fh, const void * const buffer, const size_t inSize );
   static FileError read( FileHandle fh, void * const _buffer, const size_t _size );
   static FileError seek( FileHandle fh, FileSeek seek, int offset );
   static FileError tell( FileHandle fh, unsigned int &offset );
   static FileError flush( FileHandle fh );


private:

   static DWORD privGetFileDesiredAccess( FileMode mode );
   static DWORD privGetSeek( FileSeek seek );
};

#endif