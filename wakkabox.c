/*
  This file is part of Wakkabox - A block puzzle game
  Copyright (C) 2002 Free Software Foundation, Inc
  
  Wakkabox is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  Wakkabox is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Wakkabox; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdlib.h>
#include <math.h>
#include "SDL.h"

// these checks are mostly for win32 compatibility, since the code
// isn't compiled with the -D stuff
#ifdef __BEOS__
#define CONFDIR "config"
#define DATADIR "data"
#else
#ifndef CONFDIR
#define CONFDIR "."
#endif
#ifndef DATADIR
#define DATADIR "."
#endif
#endif

#define NUMBLOCKS 10
#define NUMROWS 4
#define NUMCOLS 5
#define COLWIDTH 64
#define ROWHEIGHT 64

#define FINISH_X 3
#define FINISH_Y 1

enum overlapBitField_ { OVERLAP_NONE=0x0, OVERLAP_X=0x1, OVERLAP_Y=0x2, OVERLAP_BOTH=0x3 };
typedef int overlapBitField;

// again, windows compatibilty - msvc++, being a c++ compiler defines these
#ifndef TRUE
enum bool_ { FALSE=0, TRUE=1 };
#endif
typedef int bool;




bool bigbool;

typedef struct wakkablock_ {
	SDL_Rect *rect;
	SDL_Surface *img;
} wakkablock;

typedef struct wakkabox_ {
	SDL_Surface *screen;
	int pixperX;
	int pixperY;
} wakkabox;


wakkablock *blocks;
wakkabox *mainbox;
wakkablock *currentblock = NULL;
SDL_Rect prevRect;
unsigned int moveCount = 0;

char* savedir;

void initWakkabox( int width, int height ) {
	mainbox = malloc( sizeof( wakkabox ) );
	mainbox->screen = NULL;

	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError() );
		exit(1);
	}

	atexit(SDL_Quit);

	// remove this once we have more skills
	width = NUMCOLS * COLWIDTH;
	height = NUMROWS * ROWHEIGHT;
	//
	mainbox->screen = SDL_SetVideoMode( width, height, 8, SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF);
	if( mainbox->screen == NULL ) {
		fprintf(stderr, "Could not initialize %i x %i video mode: %s\n", width, height, SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption( "Wakkabox", NULL );

	mainbox->pixperX = mainbox->screen->w / 5;
	mainbox->pixperY = mainbox->screen->h / 4;

#ifdef __BEOS__
	savedir = CONFDIR;
#else
	savedir = getenv("HOME");
#endif
}

SDL_Surface* loadBMP( char *filename ) {
	SDL_Surface* image = NULL;

	image = SDL_LoadBMP( filename );
	if( image == NULL ) {
		fprintf(stderr, "Could not load image from file %s: %s\n", filename, SDL_GetError());
		return ( NULL );		
	}		
	
	return( image );
}

void saveConfigFile( char* filename ) {
	int i;
	char* outfilename;
	FILE *outfile = NULL;

	if( filename == NULL ) {
		outfilename = (char*)malloc( 20 + strlen(savedir) + 1);
		sprintf( outfilename, "%s/.wakkabox", savedir );
	}
	else {
		outfilename = (char*)malloc( strlen(filename) + 1 );
		strcpy(outfilename, filename); // use strcpy so we can free(outfilename) no matter what
	}		
	outfile = fopen( outfilename, "w" );

	printf("Saving block config to %s\n", outfilename );
	
	if( outfile == NULL ) {
		fprintf(stderr, "Could not open file %s", filename );
		exit(1);
	};

	for( i=0; i<NUMBLOCKS; i++ )
		fprintf(outfile, "%i %i\n", blocks[i].rect->x / mainbox->pixperX, blocks[i].rect->y / mainbox->pixperY);

	fprintf(outfile, "%i\n", moveCount );
	
	fclose( outfile );
	free( outfilename );
}

void loadConfigFile( char* filename ) {
	int i, x, y;
	char* infilename;
	FILE *infile = NULL;


	if( filename == NULL ) {
		printf("Checking for saved game ");
		infilename = (char*)malloc( strlen("/.wakkabox") + strlen(savedir)+1);
		sprintf( infilename, "%s/.wakkabox", savedir );
		printf("in file %s\n", infilename);
		
		infile = fopen( infilename, "r" );
		printf("huzzah\n");
		if( infile == NULL ) {
			printf("No saved game, using startconfig\n");
			free(infilename);
			infilename = (char*)malloc( strlen("/startconfig") + strlen(CONFDIR) + 1);
			sprintf( infilename, "%s/startconfig", CONFDIR );
			infile = fopen( infilename, "r" );
			if ( infile == NULL )
			{
				printf("startconfig not installed, checking ./\n");
				free(infilename);
				infilename = (char*)malloc( strlen("/startconfig") + strlen(".") + 1);
				sprintf( infilename, "%s/startconfig", "." );
				infile = fopen( infilename, "r" );
			}
		}
	}
	else {
		printf("using supplied filename");
		infilename = (char*)malloc( strlen(filename) + 1);
		strcpy(infilename, filename);
		infile = fopen( infilename, "r" );
	}
	printf("Loading block config from %s\n", infilename);	
	
	if( infile==NULL ) {
		fprintf(stderr, "Could not open file %s\n", filename );
		exit(1);
	};

	for( i=0; i < NUMBLOCKS; i++ ) {
		fprintf(stderr, "loading block #%i\n", i );

		if(  fscanf(infile, "%d", &x )==EOF ) {
			fprintf(stderr, "Error in config file: %s\n", filename );
			exit(1);
		}
		
		if(  fscanf(infile, "%d", &y )==EOF ) {
			fprintf(stderr, "Error in config file: %s\n", filename );
			exit(1);
		}
		
		fprintf(stderr, "block coords: %d %d\n", x, y );

		blocks[i].rect->x = x * mainbox->pixperX;
		blocks[i].rect->y = y * mainbox->pixperY;
	}	
	
	if ( fscanf( infile, "%d", &moveCount ) == EOF )
	{
		fprintf(stderr, "no movecount found, starting at 0\n");
		moveCount = 0;
	}
	else
	{
		fprintf(stderr, "starting at %i moves\n", moveCount);
	}

	free( infilename );

}	


void initBlocks() {
	char *smallblockFname, *medblockhFname, *medblockvFname, *bigblockFname;
	char *smallblockLocalName, *medblockhLocalName;
	char *medblockvLocalName, *bigblockLocalName;

	// adding a declaration here is easier than editing all the references
	// to screen not thru mainbox
	SDL_Surface *screen;

	SDL_Surface *smallblockImg, *medblockhImg, *medblockvImg, *bigblockImg;
	int i;
	SDL_Rect rect;

	screen = mainbox->screen;

	printf("Loading bmp files from %s...", DATADIR);
	
	smallblockFname = (char*)malloc( strlen("smallblock.bmp") 
									 + strlen(DATADIR) );
	medblockhFname = (char*)malloc( strlen("medblockh.bmp") 
									+ strlen(DATADIR) );
	medblockvFname = (char*)malloc( strlen("medblockv.bmp") 
									+ strlen(DATADIR) );
	bigblockFname = (char*)malloc( strlen("bigblock.bmp") 
								   + strlen(DATADIR) );

	smallblockLocalName = (char*)malloc( strlen("smallblock.bmp") 
										 + strlen(".") );
	medblockhLocalName = (char*)malloc( strlen("medblockh.bmp") 
										+ strlen(".") );
	medblockvLocalName = (char*)malloc( strlen("medblockv.bmp") 
										+ strlen(".") );
	bigblockLocalName = (char*)malloc( strlen("bigblock.bmp") 
									   + strlen(".") );
	

	sprintf( smallblockFname, "%s/smallblock.bmp", DATADIR );
	sprintf( medblockvFname, "%s/medblockv.bmp", DATADIR );
	sprintf( medblockhFname, "%s/medblockh.bmp", DATADIR );
	sprintf( bigblockFname, "%s/bigblock.bmp", DATADIR );
	
	// this is weak but i just copy/pasted
	sprintf( smallblockLocalName, "%s/smallblock.bmp", "." );
	sprintf( medblockvLocalName, "%s/medblockv.bmp", "." );
	sprintf( medblockhLocalName, "%s/medblockh.bmp", "." );
	sprintf( bigblockLocalName, "%s/bigblock.bmp", "." );
	
	smallblockImg = loadBMP( smallblockFname ),
	medblockhImg = loadBMP( medblockhFname ),
	medblockvImg = loadBMP( medblockvFname ),
	bigblockImg = loadBMP( bigblockFname );

	if (smallblockImg == NULL) smallblockImg = loadBMP( smallblockLocalName );
	if (medblockhImg == NULL ) medblockhImg = loadBMP( medblockhLocalName );
	if (medblockvImg == NULL ) medblockvImg = loadBMP( medblockvLocalName );
	if (bigblockImg == NULL ) bigblockImg = loadBMP( bigblockLocalName );
	
	if (smallblockImg == NULL) 
	{
		smallblockImg = SDL_CreateRGBSurface( SDL_HWSURFACE,
											 mainbox->pixperX,
											 mainbox->pixperY,
											 screen->format->BitsPerPixel,
											 screen->format->Rmask,
											 screen->format->Gmask,
											 screen->format->Bmask,
											 screen->format->Amask );

		SDL_FillRect( smallblockImg, NULL, SDL_MapRGB( screen->format,
													   0, 0, 0 ) );
		rect.x = 1; rect.y = 1;
		rect.w = smallblockImg->w - 2;
		rect.h = smallblockImg->h - 2;

		SDL_FillRect( smallblockImg, &rect, SDL_MapRGB( screen->format,
														255, 255, 0 ) );
	}

	if ( medblockhImg == NULL )
	{
		medblockhImg = SDL_CreateRGBSurface( SDL_HWSURFACE,
											mainbox->pixperX*2,
											mainbox->pixperY,
											screen->format->BitsPerPixel,
											screen->format->Rmask,
											screen->format->Gmask,
											screen->format->Bmask,
											screen->format->Amask );
		SDL_FillRect( medblockhImg, NULL, SDL_MapRGB( screen->format,
													  0, 0, 0 ) );
		rect.x = 1; rect.y = 1;
		rect.w = medblockhImg->w - 2;
		rect.h = medblockhImg->h - 2;

		SDL_FillRect( medblockhImg, &rect, SDL_MapRGB( screen->format,
													  0, 0, 255 ) );

	}
	
	if ( medblockvImg == NULL )
	{
		medblockvImg = SDL_CreateRGBSurface( SDL_HWSURFACE,
											mainbox->pixperX,
											mainbox->pixperY*2,
											screen->format->BitsPerPixel,
											screen->format->Rmask,
											screen->format->Gmask,
											screen->format->Bmask,
											screen->format->Amask );
		SDL_FillRect( medblockvImg, NULL, SDL_MapRGB( screen->format,
													  0, 0, 0 ) );
		rect.x = 1; rect.y = 1;
		rect.w = medblockvImg->w - 2;
		rect.h = medblockvImg->h - 2;

		SDL_FillRect( medblockvImg, &rect, SDL_MapRGB( screen->format,
													  0, 0, 255 ) );
	}
	
	if (bigblockImg == NULL) 
	{
		bigblockImg = SDL_CreateRGBSurface( SDL_HWSURFACE,
										   mainbox->pixperX*2,
										   mainbox->pixperY*2,
										   screen->format->BitsPerPixel,
										   screen->format->Rmask,
										   screen->format->Gmask,
										   screen->format->Bmask,
										   screen->format->Amask );
		SDL_FillRect( bigblockImg, NULL, SDL_MapRGB( screen->format,
													   0, 0, 0 ) );
		rect.x = 1; rect.y = 1;
		rect.w = bigblockImg->w - 2;
		rect.h = bigblockImg->h - 2;

		SDL_FillRect( bigblockImg, &rect, SDL_MapRGB( screen->format,
													 255, 0, 0 ) );
	}

	blocks = malloc( 10 * sizeof( wakkablock ) );
	
	for( i = 0; i < NUMBLOCKS; i++ ) {
		blocks[i].rect = malloc( sizeof( SDL_Rect ) );
	}

	/* init big block */
	/* SDL_DisplayFormat converts the image to a format compatible with the screen */
	blocks[0].rect->x = 0;
	blocks[0].rect->y = mainbox->pixperY;
	blocks[0].rect->w = mainbox->pixperX * 2;
	blocks[0].rect->h = mainbox->pixperY * 2;
	blocks[0].img = SDL_DisplayFormat( bigblockImg );

	/* init horizontal medium blocks */
	blocks[1].rect->x = 0;
	blocks[1].rect->y = 0;
	blocks[1].rect->w = mainbox->pixperX * 2;
	blocks[1].rect->h = mainbox->pixperY;
	blocks[1].img = SDL_DisplayFormat( medblockhImg );

	blocks[2].rect->x = 0;
	blocks[2].rect->y = mainbox->pixperY * 3;
	blocks[2].rect->w = mainbox->pixperX * 2;
	blocks[2].rect->h = mainbox->pixperY;
	blocks[2].img = SDL_DisplayFormat( medblockhImg );

	blocks[3].rect->x = mainbox->pixperX * 3;
	blocks[3].rect->y = 0;
	blocks[3].rect->w = mainbox->pixperX * 2;
	blocks[3].rect->h = mainbox->pixperY;
	blocks[3].img = SDL_DisplayFormat( medblockhImg );

	blocks[4].rect->x = mainbox->pixperX * 3;
	blocks[4].rect->y = mainbox->pixperY * 3;
	blocks[4].rect->w = mainbox->pixperX * 2;
	blocks[4].rect->h = mainbox->pixperY;
	blocks[4].img = SDL_DisplayFormat( medblockhImg );

	/* init vertical medium block */
	blocks[5].rect->x = mainbox->pixperX * 2;
	blocks[5].rect->y = mainbox->pixperY;
	blocks[5].rect->w = mainbox->pixperX;
	blocks[5].rect->h = mainbox->pixperY * 2;
	blocks[5].img = SDL_DisplayFormat( medblockvImg );

	/* init small blocks */
	blocks[6].rect->x = mainbox->pixperX * 3;
	blocks[6].rect->y = mainbox->pixperY;
	blocks[6].rect->w = mainbox->pixperX;
	blocks[6].rect->h = mainbox->pixperY;
	blocks[6].img = SDL_DisplayFormat( smallblockImg );

	blocks[7].rect->x = mainbox->pixperX * 4;
	blocks[7].rect->y = mainbox->pixperY;
	blocks[7].rect->w = mainbox->pixperX;
	blocks[7].rect->h = mainbox->pixperY;
	blocks[7].img = SDL_DisplayFormat( smallblockImg );

	blocks[8].rect->x = mainbox->pixperX * 3;
	blocks[8].rect->y = mainbox->pixperY * 2;
	blocks[8].rect->w = mainbox->pixperX;
	blocks[8].rect->h = mainbox->pixperY;
	blocks[8].img = SDL_DisplayFormat( smallblockImg );

	blocks[9].rect->x = mainbox->pixperX * 4;
	blocks[9].rect->y = mainbox->pixperY * 2;
	blocks[9].rect->w = mainbox->pixperX;
	blocks[9].rect->h = mainbox->pixperY;
	blocks[9].img = SDL_DisplayFormat( smallblockImg );

	free( bigblockImg );
	free( medblockvImg );
	free( medblockhImg );
	free( smallblockImg );

	free( smallblockFname );
	free( medblockvFname );
	free( medblockhFname );
	free( bigblockFname );

	free( smallblockLocalName );
	free( medblockhLocalName );
	free( medblockvLocalName );
	free( bigblockLocalName );

	printf("Done\n");
}	

void drawBox() {
	int i;

	// BAD!!
	SDL_FillRect( mainbox->screen, NULL, SDL_MapRGB(mainbox->screen->format, 0,0,0) );

	// also bad, but not that bad
	for( i = 0; i<NUMBLOCKS; i++ ) {
		/* NULL param means to copy the entire block image */
		SDL_BlitSurface( blocks[i].img, NULL, mainbox->screen, blocks[i].rect );
	}

	SDL_Flip(mainbox->screen);
}

bool pointInRect( int x, int y, SDL_Rect *rect ) {
	if( x < rect->x ) return( FALSE );
	if( y < rect->y ) return( FALSE );
	
	if( x > (rect->x + rect->w - 1) ) return( FALSE );
	if( y > (rect->y + rect->h - 1) ) return( FALSE );
	
	return( TRUE );
}

bool overlapBoth( int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2 ) {
	if( x1 > x2 + w2 - 1) return FALSE;
	if( x1 + w1 - 1 < x2 ) return FALSE;

	if( y1 > y2 + h2 - 1) return FALSE;
	if( y1 + h1 - 1 < y2 ) return FALSE;

	return TRUE;
}

bool rectsCollide( SDL_Rect *rect1, SDL_Rect *rect2 ) {

	return overlapBoth( rect1->x, rect1->y, rect1->w, rect1->h,
						rect2->x, rect2->y, rect2->w, rect2->h );
}

bool overlapX( int x1, int w1, int x2, int w2 ) {
	if( x1 > x2 + w2 - 1 ) return FALSE;
	if( x1 + w1 - 1 < x2 ) return FALSE;

	return( TRUE );
}

bool overlapY( int y1, int h1, int y2, int h2 ) {
	if( y1 > y2 + h2 - 1 ) return FALSE;
	if( y1 + h1 - 1 < y2 ) return FALSE;

	return( TRUE );
}

bool handleKeyDown( SDL_Event *event ) {
	fprintf(stderr, "Keydown event.\n");
	switch( event->key.keysym.sym ) {
	case SDLK_ESCAPE:
		return(TRUE);
		break;
	case SDLK_r:
		{
			// first remove save file
			char *filename;
			filename = (char*)malloc( 20 + strlen(savedir) + 1);
			sprintf( filename, "%s/save", savedir );
			remove( filename );
			// now loadconfig (which will not find save and use startconfig file)
			loadConfigFile(NULL);
			// redraw screen
			drawBox();
			break;
		}
	default: break;
		
	}
	return(FALSE);
}

bool handleKeyUp( SDL_Event *event ) {
	fprintf(stderr, "Keyup event.\n");
	return(FALSE);
}

bool handleMouseDown( SDL_Event *event ) {
	int i;
	
	for( i=0; (i<NUMBLOCKS)&&(currentblock==NULL); i++ ) {
		if( pointInRect( event->button.x, event->button.y, blocks[i].rect ) )
		{
			currentblock = &(blocks[i]);
			prevRect = *(currentblock->rect);
		}
	}
	fprintf(stderr, "MouseDown event.\n");
	return(FALSE);
}

bool handleMouseUp( SDL_Event *event ) {
	fprintf(stderr, "Mouseup event.\n");
	switch( event->button.button ) {
	case SDL_BUTTON_LEFT:
		if( currentblock != NULL ) {
/*			printf("floor(2.5) = %f\n", floor(2.5) );
			printf("released at %i, %i\n", 
			currentblock->rect->x, currentblock->rect->y );
			printf("turned to double %f, %f\n", 
			(double)currentblock->rect->x, (double)currentblock->rect->y );
			printf("divide to %f, %f\n", 
			((double)currentblock->rect->x) / mainbox->pixperX,
			((double)currentblock->rect->y) / mainbox->pixperY );
			printf("round to %f, %f\n", 
			floor( ((double)currentblock->rect->x) / mainbox->pixperX ),
			floor( ((double)currentblock->rect->y) / mainbox->pixperY ) );
*/
			currentblock->rect->x = (int) mainbox->pixperX 
				* floor( ((double)currentblock->rect->x) 
						 / mainbox->pixperX + 0.5 );
			currentblock->rect->y = (int) mainbox->pixperY 
				* floor( ((double)currentblock->rect->y) 
						 / mainbox->pixperY + 0.5 );
			
			if ( (currentblock->rect->x != prevRect.x)
				 || (currentblock->rect->y != prevRect.y) )
			{
				moveCount++;
				printf("%i moves\n", moveCount);
			}

			currentblock = NULL;
		}
		break;
	}

	/* Check winning conditions */
	
	if ( (blocks[0].rect->x / mainbox->pixperX == 3) 
		 && (blocks[0].rect->y / mainbox->pixperY == 1) )
	{
		printf("You solved the wakkabox in %i moves\n", moveCount);
		return(TRUE);
	}

	drawBox();
	return(FALSE);
}

bool handleMouseMotion( SDL_Event *event ) {
	SDL_Rect newrect;
	int i;
	int newx;
	int newy;

	if( currentblock == NULL ) return( FALSE );

	newx = currentblock->rect->x + event->motion.xrel;
	newy = currentblock->rect->y + event->motion.yrel;
	newrect.x = newx;
	newrect.y = newy;
	newrect.w = currentblock->rect->w;
	newrect.h = currentblock->rect->h;

	if( newrect.x < 0 )
		newx = 0;
	else if( newrect.x + newrect.w > mainbox->screen->w )
		newx = mainbox->screen->w - newrect.w;
	newrect.x = newx;
	
	if( newrect.y < 0 )
		newy = 0;
	else if( newrect.y + newrect.h > mainbox->screen->h )
		newy = mainbox->screen->h - newrect.h;
	newrect.y = newy;


	for( i=0; (i<NUMBLOCKS); i++ ) {
		if( &(blocks[i]) != currentblock ) {
			if( rectsCollide( &newrect, blocks[i].rect ) ) {
				fprintf(stderr, "collision with block %i -- ", i);
				fprintf(stderr, "(%i, %i) : %i x %i >< (%i, %i) : %i x %i\n",
						newrect.x, newrect.y, newrect.w, newrect.h,
						blocks[i].rect->x, blocks[i].rect->y, 
						blocks[i].rect->w, blocks[i].rect->h );
				newrect.x = newx;
				newrect.y = currentblock->rect->y;
				if( rectsCollide( &newrect, blocks[i].rect ) ) {
					newx = currentblock->rect->x;
					newrect.x = newx;
				}
		
				newrect.y = newy;
				if( rectsCollide( &newrect, blocks[i].rect ) ) {
					newy = currentblock->rect->y;
					newrect.y = newy;
				}
			}
		}	
	}

	currentblock->rect->x = newrect.x;
	currentblock->rect->y = newrect.y;

	drawBox();

	fprintf(stderr, "mouse motion event\n");
	return(FALSE);
}

bool handleEvent( SDL_Event *event ) {
	switch( event->type ) {
	case SDL_KEYDOWN:
		return( handleKeyDown( event ) ); break;
	case SDL_KEYUP:
		return( handleKeyUp( event ) ); break;
	case SDL_MOUSEBUTTONDOWN:
		return( handleMouseDown( event ) ); break;
	case SDL_MOUSEBUTTONUP:
		return( handleMouseUp( event ) ); break;
	case SDL_MOUSEMOTION:
		return( handleMouseMotion( event ) ); break;
	case SDL_QUIT:
		return( TRUE ); break;
	}
	return( FALSE );
}

int main( int argc, char* argv[] ) {
	bool done = FALSE;
	SDL_Event *event;
	char* fileName=NULL;
	event = malloc( sizeof( SDL_Event ) );

	initWakkabox(400, 500);

	initBlocks();


	if ( argc > 1 )
		fileName = argv[1];

	loadConfigFile(fileName);

	drawBox();

	while( !done ) {
	//	printf("loop...%i\n", done);
		SDL_WaitEvent( event );
		done = handleEvent( event );
	//	printf("endloop...%i\n", done);
	}
	saveConfigFile(fileName);

	/// LOOK AT ME IM SO BAD I DONT DEALLOCATE ANYTHING

	return(0);
}
