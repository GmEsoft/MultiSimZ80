#ifndef SDDIR_H
#define SDDIR_H

#include "Dir_I.h"

#include "FS.h"
#include "stdio_undefs.h"

class SdDir :
	public Dir_I
{
public:
	SdDir( File &dir ) : dir_( dir )
    {        
    }

	virtual DirEntry next();

	virtual int close()
	{
        dir_.close();
	}

	virtual const char *error() const
	{
		return "";
	}

	virtual operator bool() const
	{
		return bool( dir_ );
	}
private:
    File dir_;
};

#endif
