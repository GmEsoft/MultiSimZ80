#ifndef DIR_I_H
#define DIR_I_H

#include "DirEntry.h"

class Dir_I
{
public:

	Dir_I(void)
	{
	}

	virtual ~Dir_I(void)
	{
	}

	virtual DirEntry next() = 0;

	virtual int close() = 0;

	virtual const char *error() const = 0;

	virtual operator bool() const = 0;

};

#endif
