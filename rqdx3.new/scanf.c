#include "defs.h"

byte linebuff[133];
word linesize;
word linechar;

scanf( format, args )
byte *format;
word args;
    {
    bool negative;
    byte **ptr, c, *s, getchar( );
    word size;
    long result;

    linesize = 0;
    ptr = &args;
    while( true )
	{
	if( ( c = *format++ ) != '%' )
	    return;
	c = *format++;
	switch( c )
	    {
	    case 'b':
		size = 1;
		c = *format++;
		break;
	    case 'l':
		size = 4;
		c = *format++;
		break;
	    case 'w':
		size = 2;
		c = *format++;
		break;
	    default:
		size = 2;
		break;
	    }
	switch( c )
	    {
	    case 'c':
		result = getchar( );
		break;
	    case 'd':
		result = 0;
		c = getchar( );
		while( c == ' ' )
		    c = getchar( );
		if( c == 0 )
		    size = 0;
		if( c == '+' )
		    {
		    negative = false;
		    c = getchar( );
		    }
		else if( c == '-' )
		    {
		    negative = true;
		    c = getchar( );
		    }
		else
		    negative = false;
		while( ( c >= '0' ) && ( c <= '9' ) )
		    {
		    result = result * 10 + c - '0';
		    c = getchar( );
		    }
		if( negative )
		    result = -result;
		break;
	    case 'o':
		result = 0;
		c = getchar( );
		while( c == ' ' )
		    c = getchar( );
		if( c == 0 )
		    size = 0;
		while( ( c >= '0' ) && ( c <= '7' ) )
		    {
		    result = result * 8 + c - '0';
		    c = getchar( );
		    }
		break;
	    case 's':
		s = *ptr++;
		do
		    {
		    c = getchar( );
		    *s++ = c;
		    }
		while( c != 0 );
		size = 0;
		break;
	    case 'x':
		result = 0;
		c = getchar( );
		while( c == ' ' )
		    c = getchar( );
		if( c == 0 )
		    size = 0;
		while( ( ( c >= '0' ) && ( c <= '9' ) )
			|| ( ( c >= 'a' ) && ( c <= 'f' ) ) )
		    {
		    result = result * 16 + c - '0';
		    if( ( c >= 'a' ) && ( c <= 'f' ) )
			result += 10 + '0' - 'a';
		    c = getchar( );
		    }
		break;
	    default:
		return;
	    }
	switch( size )
	    {
	    case 1:
		*( byte * ) ( *ptr++ ) = result;
		break;
	    case 2:
		*( word * ) ( *ptr++ ) = result;
		break;
	    case 4:
		*( long * ) ( *ptr++ ) = result;
		break;
	    }
	}
    }

byte getchar( )
    {
    if( --linesize < 0 )
	{
	getline( );
	linechar = 0;
	}
    return( linebuff[linechar++] );
    }

getline( )
    {
    byte c, $$getc( );

    linechar = 0;
    while( ( c = $$getc( ) ) != 13 )
	{
	if( c < 32 )
	    if( c == 21 )
		while( linechar > 0 )
		    {
		    printf( "\010 \010" );
		    --linechar;
		    }
	    else
		printf( "\007" );
	else if( c == 127 )
	    {
	    printf( "\010 \010" );
	    --linechar;
	    }
	else
	    {
	    linebuff[linechar++] = c;
	    printf( "%c", c );
	    }
	}
    linesize = linechar;
    linebuff[linesize] = 0;
    }
