#ifndef DO_H
#define DO_H

#include "Memory_I.h"
#include "Mode.h"
#include "TermProxy.h"

class DO :
	public Memory_I,
	public TermProxy
{
public:
	DO(void);
	~DO(void);

	// getch() hook
	virtual int getch()
	{
		updateDisplay();
		return TermProxy::getch();
	}

	// kbhit() hook
	virtual int kbhit()
	{
		updateDisplay();
		return TermProxy::kbhit();
	}

	// write char
	virtual uchar write( ushort addr, uchar data );

	// read char
	virtual uchar read( ushort addr );

	// set mode handler
	virtual void setMode( Mode *pMode )
	{
		pMode_ = pMode;
	}

	// get mode
	virtual ExecMode getMode()
	{
		return pMode_ ? pMode_->getMode() : MODE_STOP;
	}

	// API
	int updateDisplay( void );

	void setDisplayUpdated( char p_DisplayUpdated );

	// ===== INTERNAL =====
	void selectFont( char p_Font );

	void putVideo( int x, int y, const char* text );

	void init();

	void setColorMode( char p_ColorMode );

	char getColorMode( );

	void setFont( char p_Font );

	void setVideo4( char p_Video4 );

	void setGraphOverlay( char p_GraphOvl );

	void setInverse( char p_Inverse );

	void setModel( char p_Model );

	void setBigDisplay( int p_BigDisplay );

	int getBigDisplay( );

	void setFullScreen( int fs );

	void setTrueColorMode( int color );
private:
	static const int	numcolormodes = 6;
	uchar		        video[0x1000];
	bool				initialized;
	bool		        displayupdated;
	int					font;
	int					fontbanks;
	bool				video4;
	bool				inverse;
	int					colormode;
	char				trs80model;
	bool				bigDisplay;
	char				graphOverlay;
	bool				trueColorMode;
	Mode				*pMode_;

};

#endif
