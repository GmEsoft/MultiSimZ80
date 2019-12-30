#include "ResDir.h"
#include "Res.h"

#if USE_RES

#include <cstdarg>

static const char *NO_ERROR = "";
static const char *ERROR_INVALID = "Invalid Function";

static ResSeq_t::const_iterator itDir = Res::getSeq().end();

ResDir::ResDir()
{
    itDir = Res::getSeq().begin();
}

DirEntry ResDir::next()
{
    DirEntry entry;
    if ( itDir != Res::getSeq().end() )
    {
        entry.name = itDir->filename;
        entry.size = itDir->res->len();
        ++itDir;
    }
    return entry;
}

#endif

