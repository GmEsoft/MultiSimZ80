#include "Res.h"

#if USE_RES

#include <cstring>

static ResSeq_t resSeq;

const Res *Res::get(const char *filename)
{
    for (   ResSeq_t::const_iterator it = resSeq.begin();
            it != resSeq.end();
            ++it )
    {
        if ( !strcmp( filename, it->filename ) )
            return it->res;
    }
    return 0;
}

void Res::put( const char *filename, const Res *res )
{
	resSeq.push_back( Res_t( filename, res ) );
}

ResSeq_t &Res::getSeq()
{
    return resSeq;
}

#endif

