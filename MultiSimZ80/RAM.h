#ifndef RAM_H
#define RAM_H

// RAM ACCESSOR

#include "Memory_I.h"

class RAM : public Memory_I
{
public:
	RAM()
		: buffer_( 0 ), size_( 0 )
	{
	}

	void setBuffer( uchar *_buffer, ushort _size )
	{
		buffer_= _buffer; 
		size_ = _size;
	}

	uchar *getBuffer()
	{
		return buffer_;
	}
	
	ushort getSize()
	{
		return size_;
	}

	// write char
	virtual uchar write( ushort addr, uchar data )
	{
		if ( !size_ || addr < size_ )
			buffer_[addr] = data;
		return data;
	}

	// read char
	virtual uchar read( ushort addr )
	{
		if ( !size_ || addr < size_ )
			return buffer_[addr];
		return 0;
	}

private:
	uchar *buffer_;
	ushort size_;
};

#endif
