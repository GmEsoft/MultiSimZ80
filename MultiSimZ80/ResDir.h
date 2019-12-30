#ifndef RESDIR_H
#define RESDIR_H

#include "TermCfg.h"

#if USE_RES

#include "Dir_I.h"

class ResDir :
	public Dir_I
{
public:
	ResDir();

	virtual DirEntry next();

	virtual int close()
	{
		return 0;
	}

	virtual const char *error() const
	{
		return "";
	}

	virtual operator bool() const
	{
		return true;
	}
};

#endif

#endif
