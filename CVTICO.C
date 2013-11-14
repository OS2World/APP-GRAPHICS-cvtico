#include <stdio.h>
#include <stdlib.h>

#define  INCL_GPIBITMAPS
#include <os2.h>
#include <pmbitmap.h>

typedef  unsigned int        WORD;
typedef  unsigned long       DWORD;

typedef struct winicohead
{
   WORD                      icoReserved;
   WORD                      icoResourceType;
   WORD                      icoResourceCount;
}  WinBitMapFileHeader;

typedef struct winbitmapinfo
{
   BYTE                      Width;
   BYTE                      Height;
   BYTE                      ColorCount;
   BYTE                      res1;
   WORD                      res2;
   WORD                      res3;
   DWORD                     icoDIBSize;
   DWORD                     icoDIBOffset;
}  WinBitMapInfo;

typedef struct windib
{
   DWORD                     size;
   DWORD                     width;
   DWORD                     height;
   WORD                      planes;
   WORD                      bitcount;
   DWORD                     compress;
   DWORD                     sizeimage;
   DWORD                     xpelspermeter;
   DWORD                     ypelspermeter;
   DWORD                     colorused;
   DWORD                     clrimportant;
}  WinDIBHead;

typedef struct winrgb
{
   BYTE                      blue;
   BYTE                      green;
   BYTE                      red;
   BYTE                      res;
}  wRGB;

void  disp_wbmfh ( WinBitMapFileHeader * );
void  disp_wbmi  ( WinBitMapInfo * );
void  disp_wdib  ( WinDIBHead * );
void  disp_rgb   ( wRGB * );
void  disp_colormap ( BYTE *, WinBitMapInfo * );
void  disp_monomap  ( BYTE *, WinBitMapInfo * );
void  disp_byte ( BYTE );
int   bitsperpel ( BYTE );
void  write_mono_rgb ( FILE * );
void  write_os2_rgb ( FILE *, wRGB *, int );

/* OS/2 structures */
BITMAPARRAYFILEHEADER        bafh;
BITMAPFILEHEADER             bfh;
RGB                          rgb;


void  main 
(
   int                       argc,
   char                      *argv []
)

{
   FILE                      *fp;
   FILE                      *ofp;
   WinBitMapFileHeader       wbmfh;
   WinBitMapInfo             wbmi;
   WinDIBHead                dibhead;
   BYTE                      colormap [64 * 64];
   BYTE                      monomap  [64 * 64];
   wRGB                      rgbtable [64];
   int                       i;
   int                       j;
   int                       pels;
   fpos_t                    curpos;
   long                      last_bafh;

   if ( argc != 3 )
   {
      fprintf ( stderr, "The correct call is:\n   cvtico <infile> <outfile>" );
      exit ( 1 );
   }

   if ( NULL == (fp = fopen ( argv [1], "rb" )) )
   {
      perror ( argv [1] );
      exit ( 1 );
   }

   if ( NULL == (ofp = fopen ( argv [2], "w+b" )) )
   {
      perror ( argv [2]);
      exit ( 1 );
   }
   fseek ( ofp, 0L, SEEK_SET );

   fread ( &wbmfh, sizeof ( wbmfh ), 1, fp );
   #if defined (DEBUG)
      disp_wbmfh ( &wbmfh );
   #endif

   for ( i = 0; i < wbmfh. icoResourceCount; i++ )
   {
      fread ( &wbmi, sizeof ( wbmi ), 1, fp );
      #if defined (DEBUG)
         disp_wbmi ( &wbmi );
      #endif

      fgetpos ( fp, &curpos );

         fseek ( fp, wbmi.icoDIBOffset, SEEK_SET );
         pels                =  8 / bitsperpel ( wbmi. ColorCount );
         fread ( &dibhead, sizeof ( dibhead ), 1, fp );
         #if defined (DEBUG)
            disp_wdib ( &dibhead );
         #endif
         fread ( rgbtable, sizeof ( wRGB ), wbmi. ColorCount, fp );
         #if defined (DEBUG)
            for ( j = 0; j < wbmi. ColorCount; j++ )
               disp_rgb ( &rgbtable [j] );
         #endif

         fread ( colormap, sizeof ( BYTE ), wbmi.Width * wbmi.Height / pels, fp );
         #if defined (DEBUG)
            disp_colormap ( colormap, &wbmi );
         #endif
         fread ( monomap,  sizeof ( BYTE ), wbmi.Width * wbmi.Height / 8, fp );
         #if defined (DEBUG)
            disp_monomap ( monomap, &wbmi );
         #endif

         last_bafh           =  ftell ( ofp );

         /* Write it out now */
         bafh.usType         =  BFT_BITMAPARRAY;
         bafh.cbSize         =  sizeof ( bafh );
         bafh.offNext        =  0L;
         bafh.cxDisplay      =  0;
         bafh.cyDisplay      =  0;

         /* The first bfh is for the AndXor mask */
         bafh.bfh.usType     =  BFT_COLORICON;
         bafh.bfh.cbSize     =  sizeof ( bafh. bfh );
         bafh.bfh.xHotspot   =  0;
         bafh.bfh.yHotspot   =  0;
         bafh.bfh.offBits    =  last_bafh            + 
                                sizeof ( bafh )      +
                                2 * 3                +
                                sizeof ( bfh )       +
                                wbmi. ColorCount * 3;

         bafh.bfh.bmp.cbFix     =  sizeof ( bafh. bfh. bmp );
         bafh.bfh.bmp.cx        =  wbmi.Width;
         bafh.bfh.bmp.cy        =  wbmi.Height * 2;
         bafh.bfh.bmp.cPlanes   =  1;
         bafh.bfh.bmp.cBitCount =  1;

         /* The second bfh is for the Color mask */
         bfh.usType          =  BFT_COLORICON;
         bfh.cbSize          =  sizeof ( bafh. bfh );
         bfh.xHotspot        =  0;
         bfh.yHotspot        =  0;
         bfh.offBits         =  last_bafh               + 
                                sizeof ( bafh )         + 
                                2 * 3                   +
                                sizeof ( bfh )          +
                                wbmi. ColorCount * 3    +
                                wbmi.Width * wbmi.Height / 4;

         bfh.bmp.cbFix       =  sizeof ( bafh. bfh. bmp );
         bfh.bmp.cx          =  wbmi.Width;
         bfh.bmp.cy          =  wbmi.Height;
         bfh.bmp.cPlanes     =  1;
         bfh.bmp.cBitCount   =  bitsperpel ( wbmi. ColorCount );

         fwrite ( &bafh, sizeof ( bafh ), 1, ofp );
         write_mono_rgb ( ofp );
         fwrite ( &bfh,  sizeof ( bfh ),  1, ofp );
         write_os2_rgb ( ofp, rgbtable, wbmi. ColorCount );

         fwrite ( monomap,  sizeof ( BYTE ), wbmi.Width * wbmi.Height / 8, ofp );
         fwrite ( monomap,  sizeof ( BYTE ), wbmi.Width * wbmi.Height / 8, ofp );
         fwrite ( colormap, sizeof ( BYTE ), wbmi.Width * wbmi.Height / pels, ofp );

      fsetpos ( fp, &curpos );
   }

   fclose ( fp );
   fclose ( ofp );

   exit ( 0 );
}

/**/
/**************************************************************************************************/

void  disp_wbmfh 
( 
   WinBitMapFileHeader       *wbmfh
)
{
   printf ( "icoReserved      = %x\n", wbmfh -> icoReserved      );
   printf ( "icoResourceType  = %x\n", wbmfh -> icoResourceType  );
   printf ( "icoResourceCount = %x\n", wbmfh -> icoResourceCount );
}

/**/
/**************************************************************************************************/

void  disp_wbmi  
( 
   WinBitMapInfo             *wbmi
)
{
   printf ( "Width        = %x\n",  wbmi -> Width        );
   printf ( "Height       = %x\n",  wbmi -> Height       ); 
   printf ( "ColorCount   = %x\n",  wbmi -> ColorCount   ); 
   printf ( "res1         = %x\n",  wbmi -> res1         ); 
   printf ( "res2         = %x\n",  wbmi -> res2         ); 
   printf ( "res3         = %x\n",  wbmi -> res3         ); 
   printf ( "icoDIBSize   = %lx\n", wbmi -> icoDIBSize   ); 
   printf ( "icoDIBOffset = %lx\n", wbmi -> icoDIBOffset ); 
}

/**/
/**************************************************************************************************/

void  disp_wdib  
( 
   WinDIBHead                *dib
)
{
   printf ( "size          = %lx\n", dib -> size          );
   printf ( "width         = %lx\n", dib -> width         );
   printf ( "height        = %lx\n", dib -> height        );
   printf ( "planes        = %x\n",  dib -> planes        );
   printf ( "bitcount      = %x\n",  dib -> bitcount      );
   printf ( "compress      = %lx\n", dib -> compress      );
   printf ( "sizeimage     = %lx\n", dib -> sizeimage     );
   printf ( "xpelspermeter = %lx\n", dib -> xpelspermeter );
   printf ( "ypelspermeter = %lx\n", dib -> ypelspermeter );
   printf ( "colorused     = %lx\n", dib -> colorused     );
   printf ( "clrimportant  = %lx\n", dib -> clrimportant  );
}

/**/
/**************************************************************************************************/

void  disp_rgb  
( 
   wRGB                       *rgb
)
{
   printf ( "%02x %02x %02x %02x\n", rgb -> blue, rgb -> green, rgb -> red, rgb -> res );
}

/**/
/**************************************************************************************************/

int   bitsperpel 
( 
   BYTE                      colorcount
)
{
   if ( colorcount == 2 )
      return ( 1 );
   else if ( colorcount == 8 )
      return ( 3 );
   else
      return ( 4 );
}

/**/
/**************************************************************************************************/

void  disp_colormap 
( 
   BYTE                      *colormap,
   WinBitMapInfo             *wbmi
)
{
   int                       w;
   int                       h;
   int                       width;
   int                       height;
   int                       pels;

   width                     =  wbmi -> Width;
   height                    =  wbmi -> Height;
   pels                      =  8 / bitsperpel ( wbmi -> ColorCount );

   for ( h = 0; h < height; h++ )
   {
      for ( w = 0; w < width / pels; w++ )
         printf ( "%02x", colormap [ h * width / pels + w ] );
      printf ( "\n" );
   }
}

/**/
/**************************************************************************************************/

void  disp_monomap
( 
   BYTE                      *monomap,
   WinBitMapInfo             *wbmi
)
{
   int                       w;
   int                       h;
   int                       width;
   int                       height;

   width                     =  wbmi -> Width;
   height                    =  wbmi -> Height;

   for ( h = 0; h < height; h++ )
   {
      for ( w = 0; w < width / 8; w++ )
         disp_byte ( monomap [ h * width / 8 + w ] );
      printf ( "\n" );
   }
}

/**/
/**************************************************************************************************/

void  disp_byte 
( 
   BYTE                      b 
)
{
   int                       i;

   for ( i = 0; i < 8; i++ )
   {
      printf ( "%d", ( 0 == (b & 0x80) ) ? 0 : 1 );
      b                      =  b << 1;
   }
}

/**/
/**************************************************************************************************/

char  os2_rgb_defaults [] =
{
   0x00, 0x00, 0x00,
   0xff, 0xff, 0xff,
   0xff, 0x00, 0x00,
   0x00, 0x00, 0xff,
   0xff, 0x00, 0xff,
   0x00, 0xff, 0x00,
   0xff, 0xff, 0x00,
   0x00, 0xff, 0xff,
   0x40, 0x40, 0x40,
   0x80, 0x00, 0x00,
   0x00, 0x00, 0x80,
   0x80, 0x00, 0x80,
   0x00, 0x80, 0x00,
   0x80, 0x80, 0x00,
   0x00, 0x34, 0x78,
   0xc0, 0xc0, 0xc0
};
void  write_os2_rgb 
( 
   FILE                      *ofp,
   wRGB                      *rgbtable,
   int                       colorcount
)
{
   int                       i;

   for ( i = 0; i < colorcount; i++ )
   {
      fwrite ( &rgbtable [i].blue,  sizeof ( BYTE ), 1, ofp );
      fwrite ( &rgbtable [i].green, sizeof ( BYTE ), 1, ofp );
      fwrite ( &rgbtable [i].red,   sizeof ( BYTE ), 1, ofp );
   }

// fwrite ( os2_rgb_defaults, sizeof ( os2_rgb_defaults ), 1, ofp );
}

/**/
/**************************************************************************************************/

char  mono_rgb_defaults [] =
{
   0x00, 0x00, 0x00,
   0xff, 0xff, 0xff
};
void  write_mono_rgb 
( 
   FILE                      *ofp
)
{
   fwrite ( mono_rgb_defaults, sizeof ( mono_rgb_defaults ), 1, ofp );
}
