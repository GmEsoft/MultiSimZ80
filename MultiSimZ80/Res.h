#ifndef RES_H
#define RES_H

#include "TermCfg.h"

#if USE_RES

#include "runtime.h"
#include <vector>

class Res;

struct Res_t
{
    Res_t( const char *_filename, const Res *_res )
        : filename( _filename ), res( _res )
    {
    }
    const char *filename;
    const Res *res;
};

typedef std::vector< Res_t > ResSeq_t; 

class Res
{
public:
	Res( const uchar *res, const uint len ) 
		: res_( res ), len_( len )
	{
	}

	static void put( const char *filename, const Res *res );

	static const Res *get( const char *filename );

    static ResSeq_t &getSeq();

	const uchar *get() const
	{
		return res_;
	}

	uint len() const
	{
		return len_;
	}

private:
	const uchar	*res_;
	const uint	len_;
};

#endif

#endif
