/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        TMS9918.h                        **/
/**                                                         **/
/** This file contains emulation for the TMS9918 video chip **/
/** produced by Texas Instruments.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#ifndef TMS9918_H
#define TMS9918_H

#define VDP_UPERIOD     2   /* Default screen update period  */
#define VDP_MAXSPRITES  4   /* Default max number of sprites */

#define BigSprites(VDP)   (VDP->R[1]&0x01) /* Zoomed sprites */
#define Sprites16x16(VDP) (VDP->R[1]&0x02) /* Big sprites    */
#define ScreenON(VDP)     (VDP->R[1]&0x40) /* Display on     */
#define VBlankON(VDP)     (VDP->R[1]&0x20) /* VBlanks on     */

#ifndef byte
 typedef unsigned char byte;
#endif

/** TMS9918 **************************************************/
/** This data structure stores VDP state, screen buffer,    **/
/** VRAM, and other parameters.                             **/
/*************************************************************/
typedef struct
{
  /* User-Configurable Parameters */
  byte  UPeriod;     /* Screen update period    */
  byte  MaxSprites;  /* Number of sprites/line  */

  /* VDP State */
  byte  *VRAM;       /* 16kB of VRAM            */
  byte  R[16];       /* VDP control registers   */
  byte  Status;      /* VDP status register     */
  byte  VKey;        /* 1: Control byte latched */
  byte  WKey;        /* 1: VRAM in WRITE mode   */
  byte  Mode;        /* Current screen mode     */
  byte  Line;        /* Current scanline        */
  byte  CLatch;      /* Control register latch  */
  byte  DLatch;      /* Data register latch     */
  short VAddr;       /* VRAM access address     */

  /* VRAM Tables */
  byte  *ChrTab;     /* Character Name Table    */
  byte  *ChrGen;     /* Character Pattern Table */
  byte  *SprTab;     /* Sprite Attribute Table  */
  byte  *SprGen;     /* Sprite Pattern Table    */
  byte  *ColTab;     /* Color Table             */

  /* Picture Rendering */
  void  *XBuf;       /* Buffer where picture is formed     */
  int   XPal[16];    /* Colors obtained with SetColor()    */
  int   FGColor;     /* Foreground color (Rg#7)            */
  int   BGColor;     /* Background color (Rg#7)            */
  byte  UCount;      /* Screen update counter              */
  int   UOffset;     /* Current offset in XBuf             */
  byte  OwnXBuf;     /* 1: XBuf was allocated in New9918() */
  int   Width;       /* XBuf width in pixels >=256         */
  int   Height;      /* XBuf height in pixels >=192        */
} TMS9918;

/** Palette9918[] ********************************************/
/** 16 standard colors used by TMS9918/TMS9928 VDP chips.   **/
/*************************************************************/
// extern struct { byte R,G,B; } Palette9918[];

/** New9918() ************************************************/
/** Create a new VDP context. The user can either provide   **/
/** his own screen buffer by pointing Buffer to it, or ask  **/
/** New9918() to allocate a buffer by setting Buffer to 0.  **/
/** Width and Height must always specify screen buffer      **/
/** dimensions. New9918() pointer to the screen buffer on   **/
/** success, 0 otherwise.                                   **/
/*************************************************************/
void *New9918(TMS9918 *VDP,byte *Buffer,int Width,int Height);

/** Reset9918() **********************************************/
/** Reset the VDP. The user can provide a new screen buffer **/
/** by pointing Buffer to it and setting Width and Height.  **/
/** Set Buffer to 0 to use the existing screen buffer.      **/
/*************************************************************/
void Reset9918(TMS9918 *VDP,byte *Buffer,int Width,int Height);

/** Trash9918() **********************************************/
/** Free all buffers associated with VDP and invalidate VDP **/
/** context. Use this to shut down VDP.                     **/
/*************************************************************/
void Trash9918(TMS9918 *VDP);

/** WrCtrl9918() *********************************************/
/** Write a value V to the VDP Control Port.                **/
/*************************************************************/
void WrCtrl9918(TMS9918 *VDP,byte V);

/** RdCtrl9918() *********************************************/
/** Read a value from the VDP Control Port.                 **/
/*************************************************************/
byte RdCtrl9918(TMS9918 *VDP);

/** WrData9918() *********************************************/
/** Write a value V to the VDP Data Port.                   **/
/*************************************************************/
void WrData9918(TMS9918 *VDP,byte V);

/** RdData9918() *********************************************/
/** Read a value from the VDP Data Port.                    **/
/*************************************************************/
byte RdData9918(TMS9918 *VDP);

/** Loop9918() ***********************************************/
/** Call this routine on every scanline to update the       **/
/** screen buffer. Loop9918() returns 1 if an interrupt is  **/
/** to be generated, 0 otherwise.                           **/
/*************************************************************/
byte Loop9918(TMS9918 *VDP);

/** Write9918() **********************************************/
/** This is a convinience function provided for the user to **/
/** write into VDP control registers. This can also be done **/
/** by two consequent WrCtrl9918() calls.                   **/
/*************************************************************/
void Write9918(TMS9918 *VDP,byte R,byte V);

/** RefreshLine#() *******************************************/
/** Screen handlers refreshing current scanline in a screen **/
/** buffer. These functions are used from TMS9918.c and     **/
/** should not be called by the user.                       **/
/*************************************************************/
void RefreshLine0(TMS9918 *VDP);
void RefreshLine1(TMS9918 *VDP);
void RefreshLine2(TMS9918 *VDP);
void RefreshLine3(TMS9918 *VDP);

#endif /* TMS9918_H */
