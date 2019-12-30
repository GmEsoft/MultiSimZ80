#include "SdDir.h"

#include <string.h>


DirEntry SdDir::next()
{
    DirEntry entry;
    if ( File file = dir_.openNextFile() )
    {
        entry.name = strrchr( file.name(), '/' ) + 1;
        entry.size = file.size();
        entry.isDir = file.isDirectory();
        file.close();
    }
    return entry;
}

