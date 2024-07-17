#ifndef NACHOS_FILESYS_UNIQUEOPENFILE__HH
#define NACHOS_FILESYS_UNIQUEOPENFILE__HH

#include "filesys/file_header.hh"
#include "open_file.hh"

class UniqueOpenFile : public OpenFile {
   public:
    UniqueOpenFile(int _sector) : OpenFile(_sector, new FileHeader) { fileHeader->FetchFrom(_sector); }

    ~UniqueOpenFile() override { delete fileHeader; }
};

#endif
