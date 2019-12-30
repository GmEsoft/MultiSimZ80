#ifndef FILEDESCRIPTOR_H
#define FILEDESCRIPTOR_H

#include <fcntl.h>
#include <sys/stat.h>

#ifndef O_ACCMODE
#define O_ACCMODE	( ! ( O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_BINARY ) )
#endif

#endif
