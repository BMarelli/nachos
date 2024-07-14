#ifndef NACHOS_FILESYS_FILE_PATH__HH
#define NACHOS_FILESYS_FILE_PATH__HH

class FilePath {
    FilePath(const char *path);

    const char *GetBaseName();
};

#endif
