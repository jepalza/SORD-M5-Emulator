/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        TMS9918.c                        **/
/**                                                         **/
/** This file contains emulation for the TMS9918 video chip **/
/** produced by Texas Instruments.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "TMS9918.h"

/** Palette9918[] ********************************************/
/** 16 standard colors used by TMS9918/TMS9928 VDP chips.   **/
/*************************************************************/
// struct { byte R,G,B; } Palette9918[16] =
byte Palette9918[16][3] =
{
  {0x00,0x00,0x00},{0xf0,0xf0,0x00},{0x20,0xC0,0x20},{0x60,0xE0,0x60},
  {0x20,0x20,0xE0},{0x40,0x60,0xE0},{0xA0,0x20,0x20},{0x40,0xC0,0xE0},
  {0xE0,0x20,0x20},{0xE0,0x60,0x60},{0xC0,0xC0,0x20},{0xC0,0xC0,0x80},
  {0x20,0x80,0x20},{0xC0,0x40,0xA0},{0xA0,0xA0,0xA0},{0xE0,0xE0,0xE0}
};

/** Screen[] *************************************************/
/** Pointer to the scanline refresh handlers and VDP table  **/
/** address masks for the standard TMS9918 screen modes.    **/
/*************************************************************/
static struct
{
  void (*LineHandler)(TMS9918 *VDP);
  byte R2,R3,R4,R5,R6;
} Screen[4] =
{
  { RefreshLine0,0x7F,0x00,0x3F,0x00,0x3F },/* SCREEN 0:TEXT 40x24    */
  { RefreshLine1,0x7F,0xFF,0x3F,0xFF,0x3F },/* SCREEN 1:TEXT 32x24    */
  { RefreshLine2,0x7F,0x80,0x3C,0xFF,0x3F },/* SCREEN 2:BLOCK 256x192 */
  { RefreshLine3,0x7F,0x00,0x3F,0xFF,0x3F },/* SCREEN 3:GFX 64x48x16  */
};

/** Static Functions *****************************************/
/** Functions used internally by the TMS9918 emulation.     **/
/*************************************************************/
static byte CheckSprites(TMS9918 *VDP);

/** New9918() ************************************************/
/** Create a new VDP context. The user can either provide   **/
/** his own screen buffer by pointing Buffer to it, or ask  **/
/** New9918() to allocate a buffer by setting Buffer to 0.  **/
/** Width and Height must always specify screen buffer      **/
/** dimensions. New9918() pointer to the screen buffer on   **/
/** success, 0 otherwise.                                   **/
/*************************************************************/
void *New9918(TMS9918 *VDP,byte *Buffer,int Width,int Height)
{
  byte j;

  /* Allocate memory for VRAM */
  VDP->VRAM=(byte *)malloc(0x4000);
  if(!VDP->VRAM) return(0);

  /* Reset VDP */
  VDP->UPeriod=VDP_UPERIOD;
  VDP->MaxSprites=VDP_MAXSPRITES;
  VDP->OwnXBuf=0;
  Reset9918(VDP,Buffer,Width,Height);

  /* If needed, allocate screen buffer */
  if(!Buffer)
  {
    Buffer=(void *)malloc(Width*Height);
    if(Buffer) { VDP->XBuf=Buffer;VDP->OwnXBuf=1; }
    else       { free(VDP->VRAM);return(0); }
  }


  // genera la paleta de colores (por Joseba Epalza 24-8-00)
  for (j=0;j<16;j++)
      VDP->XPal[j]=j+128;

  /* Done */
  return(VDP->XBuf);
}

/** Reset9918() **********************************************/
/** Reset the VDP. The user can provide a new screen buffer **/
/** by pointing Buffer to it and setting Width and Height.  **/
/** Set Buffer to 0 to use the existing screen buffer.      **/
/*************************************************************/
void Reset9918(TMS9918 *VDP,byte *Buffer,int Width,int Height)
{
  /* If new screen buffer is passed, set it */
  if(Buffer)
  {
    if(VDP->OwnXBuf&&VDP->XBuf) free(VDP->XBuf);
    VDP->XBuf=Buffer;
    VDP->Width=Width;
    VDP->Height=Height;
    VDP->OwnXBuf=0;
  }

  VDP->UCount=0;
  VDP->VAddr=0x0000;
  VDP->Status=0x00;
  VDP->VKey=1;
  VDP->WKey=1;
  VDP->Mode=0;
  VDP->Line=0;

  VDP->ChrTab=VDP->VRAM;
  VDP->ChrGen=VDP->VRAM;
  VDP->ColTab=VDP->VRAM;
  VDP->SprTab=VDP->VRAM;
  VDP->SprGen=VDP->VRAM;
}

/** Trash9918() **********************************************/
/** Free all buffers associated with VDP and invalidate VDP **/
/** context. Use this to shut down VDP.                     **/
/*************************************************************/
void Trash9918(TMS9918 *VDP)
{
  /* Free all allocated memory */
  if(VDP->VRAM)               free(VDP->VRAM);
  if(VDP->XBuf&&VDP->OwnXBuf) free(VDP->XBuf);

  VDP->VRAM=0;
  VDP->XBuf=0;
  VDP->OwnXBuf=0;
}

/** Write9918() **********************************************/
/** This is a convinience function provided for the user to **/
/** write into VDP control registers. This can also be done **/
/** by two consequent WrCtrl9918() calls.                   **/
/*************************************************************/
void Write9918(TMS9918 *VDP,byte R,byte V)
{
  register byte J;

  /* Store value into a register */
  VDP->R[R]=V;

  /* Depending on the register, do... */
  switch(R)
  {
    case 0:
    case 1:
      /* Figure out new screen mode number */
      switch(((VDP->R[0]&0x0E)>>1)|(VDP->R[1]&0x18))
      {
        case 0x10: V=0;break;
        case 0x00: V=1;break;
        case 0x01: V=2;break;
        case 0x08: V=3;break;
        default:   V=VDP->Mode;
      }

      /* If mode was changed, recompute table addresses */
      if(V!=VDP->Mode)
      {
        VDP->ChrTab=VDP->VRAM+((int)(VDP->R[2]&Screen[V].R2)<<10);
        VDP->ColTab=VDP->VRAM+((int)(VDP->R[3]&Screen[V].R3)<<6);
        VDP->ChrGen=VDP->VRAM+((int)(VDP->R[4]&Screen[V].R4)<<11);
        VDP->SprTab=VDP->VRAM+((int)(VDP->R[5]&Screen[V].R5)<<7);
        VDP->SprGen=VDP->VRAM+((int)(VDP->R[6]&Screen[V].R6)<<11);
        VDP->Mode=V;
      }
      break;

    case  2: VDP->ChrTab=VDP->VRAM+((int)(V&Screen[VDP->Mode].R2)<<10);break;
    case  3: VDP->ColTab=VDP->VRAM+((int)(V&Screen[VDP->Mode].R3)<<6);break;
    case  4: VDP->ChrGen=VDP->VRAM+((int)(V&Screen[VDP->Mode].R4)<<11);break;
    case  5: VDP->SprTab=VDP->VRAM+((int)(V&Screen[VDP->Mode].R5)<<7);break;
    case  6: VDP->SprGen=VDP->VRAM+((int)(V&Screen[VDP->Mode].R6)<<11);break;
    case  7: VDP->FGColor=VDP->XPal[V>>4];
             V&=0x0F;
             VDP->XPal[0]=VDP->XPal[V? V:1];
             VDP->BGColor=VDP->XPal[V];
             break;
  }
}

/** Loop9918() ***********************************************/
/** Call this routine on every scanline to update the       **/
/** screen buffer. Loop9918() returns 1 if an interrupt is  **/
/** to be generated, 0 otherwise.                           **/
/*************************************************************/
byte Loop9918(TMS9918 *VDP)
{
  register byte J;

  /* If refreshing screen... */
  if(VDP->Line<192)
  {
    /* Decrement/reset update counter */
    if(!VDP->Line)
    {
      J=VDP->UCount;
      J=(J? J:VDP->UPeriod)-1;
      if(!J) VDP->UOffset=VDP->Width*(VDP->Height-192)/2+VDP->Width/2-128;
      VDP->UCount=J;
    }

    /* Refresh a scanline if needed */
    if(!VDP->UCount)
    {
      Screen[VDP->Mode].LineHandler(VDP);
      VDP->UOffset+=VDP->Width;
    }

    /* Go to the next line */
    VDP->Line++;

    /* No interrupt */
    return(0);
  }

  /* Time for VBlank... */

  /* Reset scanline */
  VDP->Line=0;

  /* Set VBlank status flag */
  VDP->Status|=0x80;

  /* Set Sprite Collision status flag */
  if(!(VDP->Status&0x20))
    if(CheckSprites(VDP)) VDP->Status|=0x20;

  /* Generate VDP interrupt */
  return(VDP->VKey&&VBlankON(VDP));
}

/** WrCtrl9918() *********************************************/
/** Write a value V to the VDP Control Port.                **/
/*************************************************************/
void WrCtrl9918(TMS9918 *VDP,byte V)
{
  if(VDP->VKey) { VDP->VKey=0;VDP->CLatch=V; }
  else
  {
    VDP->VKey=1;
    switch(V&0xC0)
    {
      case 0x00:
      case 0x40:
        VDP->VAddr=(VDP->CLatch|((int)V<<8))&0x3FFF;
        VDP->WKey=V&0x40;
/*
        if(!VDP->WKey)
        {
          VDP->DLatch=VDP->VRAM[VDP->VAddr];
          VDP->VAddr=(VDP->VAddr+1)&0x3FFF;
        }
*/
        break;
      case 0x80:
        Write9918(VDP,V&0x0F,VDP->CLatch);
        break;
    }
  }
}

/** RdCtrl9918() *********************************************/
/** Read a value from the VDP Control Port.                 **/
/*************************************************************/
byte RdCtrl9918(TMS9918 *VDP)
{
  register byte J;

  J=VDP->Status;
  VDP->Status&=0x5F;
  VDP->VKey=1;
  return(J);
}

/** WrData9918() *********************************************/
/** Write a value V to the VDP Data Port.                   **/
/*************************************************************/
void WrData9918(TMS9918 *VDP,byte V)
{
  if(VDP->WKey)
  {
    /* VDP in the proper WRITE mode */
    VDP->DLatch=VDP->VRAM[VDP->VAddr]=V;
    VDP->VAddr=(VDP->VAddr+1)&0x3FFF;
  }
  else
  {
    /* VDP in the READ mode */
    VDP->DLatch=VDP->VRAM[VDP->VAddr];
    VDP->VAddr=(VDP->VAddr+1)&0x3FFF;
    VDP->VRAM[VDP->VAddr]=V;
  }
}

/** RdData9918() *********************************************/
/** Read a value from the VDP Data Port.                    **/
/*************************************************************/
byte RdData9918(TMS9918 *VDP)
{
  register byte J;

/*
  J=VDP->DLatch;
  VDP->DLatch=VDP->VRAM[VDP->VAddr];
*/
  J=VDP->VRAM[VDP->VAddr];
  VDP->VAddr=(VDP->VAddr+1)&0x3FFF;
  return(J);
}

/** CheckSprites() *******************************************/
/** This function is periodically called to check for the   **/
/** sprite collisions and return 1 if collision has occured **/
/*************************************************************/
byte CheckSprites(TMS9918 *VDP)
{
  register int LS,LD;
  register byte DH,DV,*PS,*PD,*T;
  byte I,J,N,*S,*D;

  /* Find first sprite to display */
  for(N=0,S=VDP->SprTab;(N<32)&&(S[0]!=208);N++,S+=4);

  if(Sprites16x16(VDP))
  {
    for(J=0,S=VDP->SprTab;J<N;J++,S+=4)
      if(S[3]&0x0F)
        for(I=J+1,D=S+4;I<N;I++,D+=4)
          if(D[3]&0x0F)
          {
            DV=S[0]-D[0];
            if((DV<16)||(DV>240))
            {
              DH=S[1]-D[1];
              if((DH<16)||(DH>240))
              {
                PS=VDP->SprGen+((int)(S[2]&0xFC)<<3);
                PD=VDP->SprGen+((int)(D[2]&0xFC)<<3);
                if(DV<16) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>240) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while(DV<16)
                {
                  LS=((int)PS[0]<<8)+PS[16];
                  LD=((int)PD[0]<<8)+PD[16];
                  if(LD&(LS>>DH)) break;
                  else { DV++;PS++;PD++; }
                }
                if(DV<16) return(1);
              }
            }
          }
  }
  else
  {
    for(J=0,S=VDP->SprTab;J<N;J++,S+=4)
      if(S[3]&0x0F)
        for(I=J+1,D=S+4;I<N;I++,D+=4)
          if(D[3]&0x0F)
          {
            DV=S[0]-D[0];
            if((DV<8)||(DV>248))
            {
              DH=S[1]-D[1];
              if((DH<8)||(DH>248))
              {
                PS=VDP->SprGen+((int)S[2]<<3);
                PD=VDP->SprGen+((int)D[2]<<3);
                if(DV<8) PD+=DV; else { DV=256-DV;PS+=DV; }
                if(DH>248) { DH=256-DH;T=PS;PS=PD;PD=T; }
                while((DV<8)&&!(*PD&(*PS>>DH))) { DV++;PS++;PD++; }
                if(DV<8) return(1);
              }
            }
          }
  }

  /* No collision */
  return(0);
}
