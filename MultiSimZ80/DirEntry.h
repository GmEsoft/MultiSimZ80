#ifndef DIRENTRY_H
#define DIRENTRY_H

class DirEntry
{
public:
	
	DirEntry() : name( 0 ), size( 0 ), isDir( false )
	{
	}

	const char *name;

	int size;

	bool isDir;

	operator bool()
	{
		return !!name;
	}
};

#endif
