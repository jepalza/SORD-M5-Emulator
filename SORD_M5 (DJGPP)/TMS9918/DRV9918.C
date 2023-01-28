/** EMULib Emulation Library *********************************/
/**                                                         **/
/**                        DRV9918.c                        **/
/**                                                         **/
/** This file contains emulation for the TMS9918 video chip **/
/** produced by Texas Instruments.                          **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996-1999                 **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/
#include <stdio.h>
#include "TMS9918.h"

#ifndef pixel
typedef byte pixel;
#endif

/** Static Functions *****************************************/
/** Functions used internally by the TMS9918 drivers.       **/
/*************************************************************/
static void RefreshSprites(TMS9918 *VDP);
static void RefreshBorder(TMS9918 *VDP);

/** RefreshBorder() ******************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** the screen border.                                      **/
/*************************************************************/
void RefreshBorder(register TMS9918 *VDP)
{
  register pixel *P,BC;
  register int J,N;
  register byte Y;

  /* Border color */
  BC=VDP->BGColor;

  /* Screen buffer */
  P=(pixel *)(VDP->XBuf);
  Y=VDP->Line;
  J=VDP->Width*(Y+(VDP->Height-192)/2);

  /* For the first line, refresh top border */
  if(Y) P+=J;
  else for(;J;J--) *P++=BC;

  /* Calculate number of pixels */
  N=(VDP->Width-(VDP->Mode? 256:240))/2; 

  /* Refresh left border */
  for(J=N;J;J--) *P++=BC;

  /* Refresh right border */
  P+=VDP->Width-(N<<1);
  for(J=N;J;J--) *P++=BC;

  /* For the last line, refresh bottom border */
  if(Y==191)
    for(J=VDP->Width*(VDP->Height-192)/2;J;J--) *P++=BC;
}

/** RefreshSprites() *****************************************/
/** This function is called from RefreshLine#() to refresh  **/
/** sprites.                                                **/
/*************************************************************/
void RefreshSprites(register TMS9918 *VDP)
{
  register byte Y,H,*PT,*AT;
  register pixel *P,*T,C;
  register int L,K;
  register unsigned int M;

  Y=VDP->Line;
  T=(pixel *)(VDP->XBuf)+VDP->UOffset;
  H=Sprites16x16(VDP)? 16:8;
  AT=VDP->SprTab-4;
  C=VDP->MaxSprites;
  M=0;L=0;

  do
  {
    M<<=1;AT+=4;L++;    /* Iterate through SprTab */
    K=AT[0];            /* K = sprite Y coordinate */
    if(K==208) break;   /* Iteration terminates if Y=208 */
    if(K>256-H) K-=256; /* Y coordinate may be negative */

    /* Mark all valid sprites with 1s, break at MaxSprites */
    if((Y>K)&&(Y<=K+H)) { M|=1;if(!--C) break; }

  }
  while(L<32);

  for(;M;M>>=1,AT-=4)
    if(M&1)
    {
      C=AT[3];                  /* C = sprite attributes */
      L=C&0x80? AT[1]-32:AT[1]; /* Sprite may be shifted left by 32 */
      C&=0x0F;                  /* C = sprite color */

      if((L<256)&&(L>-H)&&C)
      {
        K=AT[0];                /* K = sprite Y coordinate */
        if(K>256-H) K-=256;     /* Y coordinate may be negative */

        PT=VDP->SprGen+((int)(H>8? AT[2]&0xFC:AT[2])<<3)+Y-K-1;
        C=VDP->XPal[C];
        P=T+L;

        /* Mask 1: clip left sprite boundary */
        K=L>=0? 0x0FFFF:(0x10000>>-L)-1;

        /* Mask 2: clip right sprite boundary */
        if(L>256-H) K^=((0x00200>>(H-8))<<(L-257+H))-1;

        /* Get and clip the sprite data */
        K&=((int)PT[0]<<8)|(H>8? PT[16]:0x00);

        /* Draw left 8 pixels of the sprite */
        if(K&0xFF00)
        {
          if(K&0x8000) P[0]=C;
          if(K&0x4000) P[1]=C;
          if(K&0x2000) P[2]=C;
          if(K&0x1000) P[3]=C;
          if(K&0x0800) P[4]=C;
          if(K&0x0400) P[5]=C;
          if(K&0x0200) P[6]=C;
          if(K&0x0100) P[7]=C;
        }

        /* Draw right 8 pixels of the sprite */
        if(K&0x00FF)
        {
          if(K&0x0080) P[8]=C;
          if(K&0x0040) P[9]=C;
          if(K&0x0020) P[10]=C;
          if(K&0x0010) P[11]=C;
          if(K&0x0008) P[12]=C;
          if(K&0x0004) P[13]=C;
          if(K&0x0002) P[14]=C;
          if(K&0x0001) P[15]=C;
        }
      }
    }
}

/** RefreshLine0() *******************************************/
/** Refresh line Y (0..191) of SCREEN0, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine0(register TMS9918 *VDP)
{
  register byte *T,X,K,Offset;
  register pixel *P,FC,BC;

  P=(pixel *)(VDP->XBuf)+VDP->UOffset+8;
  BC=VDP->BGColor;
  FC=VDP->FGColor;

  if(!ScreenON(VDP))
    for(X=0;X<240;X++) *P++=BC;
  else
  {
    K=VDP->Line;
    T=VDP->ChrTab+(K>>3)*40;
    Offset=K&0x07;

    for(X=0;X<40;X++)
    {
      K=VDP->ChrGen[((int)*T<<3)+Offset];
      P[0]=K&0x80? FC:BC;
      P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;
      P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;
      P[5]=K&0x04? FC:BC;
      P+=6;T++;
    }
  }

  /* Refresh screen border */
  RefreshBorder(VDP);
}

/** RefreshLine1() *******************************************/
/** Refresh line Y (0..191) of SCREEN1, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine1(register TMS9918 *VDP)
{
  register byte *T,X,K,Offset;
  register pixel *P,FC,BC;

  P=(pixel *)(VDP->XBuf)+VDP->UOffset;

  if(!ScreenON(VDP))
  {
    register int J;
    BC=VDP->BGColor;
    for(J=0;J<256;J++) *P++=BC;
  }
  else
  {
    K=VDP->Line;
    T=VDP->ChrTab+((int)(K&0xF8)<<2);
    Offset=K&0x07;

    for(X=0;X<32;X++)
    {
      K=*T;
      BC=VDP->ColTab[K>>3];
      FC=VDP->XPal[BC>>4];
      BC=VDP->XPal[BC&0x0F];
      K=VDP->ChrGen[((int)K<<3)+Offset];
      P[0]=K&0x80? FC:BC;
      P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;
      P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;
      P[5]=K&0x04? FC:BC; 
      P[6]=K&0x02? FC:BC;
      P[7]=K&0x01? FC:BC;
      P+=8;T++;
    }

    /* Refresh sprites */
    RefreshSprites(VDP);
  }

  /* Refresh screen border */
  RefreshBorder(VDP);
}

/** RefreshLine2() *******************************************/
/** Refresh line Y (0..191) of SCREEN2, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine2(register TMS9918 *VDP)
{
  register byte X,K,Offset;
  register byte *T,*PGT,*CLT;
  register pixel *P,FC,BC;
  register int J;

  P=(pixel *)(VDP->XBuf)+VDP->UOffset;

  if(!ScreenON(VDP))
  {
    BC=VDP->BGColor;
    for(J=0;J<256;J++) *P++=BC;
  }
  else
  {
    K=VDP->Line;
    J=(int)(K&0xC0)<<5;
    PGT=VDP->ChrGen+J;
    CLT=VDP->ColTab+J;
    T=VDP->ChrTab+((int)(K&0xF8)<<2);
    Offset=K&0x07;

    for(X=0;X<32;X++)
    {
      J=((int)*T<<3)+Offset;
      K=CLT[J];
      FC=VDP->XPal[K>>4];
      BC=VDP->XPal[K&0x0F];
      K=PGT[J];
      P[0]=K&0x80? FC:BC;
      P[1]=K&0x40? FC:BC;
      P[2]=K&0x20? FC:BC;
      P[3]=K&0x10? FC:BC;
      P[4]=K&0x08? FC:BC;
      P[5]=K&0x04? FC:BC;
      P[6]=K&0x02? FC:BC;
      P[7]=K&0x01? FC:BC;
      P+=8;T++;
    }

    /* Refresh sprites */
    RefreshSprites(VDP);
  }

  /* Refresh screen border */
  RefreshBorder(VDP);
}

/** RefreshLine3() *******************************************/
/** Refresh line Y (0..191) of SCREEN3, including sprites   **/
/** in this line.                                           **/
/*************************************************************/
void RefreshLine3(register TMS9918 *VDP)
{
  register byte *T,X,K,Offset;
  register pixel *P;

  P=(pixel *)(VDP->XBuf)+VDP->UOffset;

  if(!ScreenON(VDP))
  {
    register pixel BC;
    register int J;
    BC=VDP->BGColor;
    for(J=0;J<256;J++) *P++=BC;
  }
  else
  {
    K=VDP->Line;
    T=VDP->ChrTab+((int)(K&0xF8)<<2);
    Offset=(K&0x1C)>>2;

    for(X=0;X<32;X++)
    {
      K=VDP->ChrGen[((int)*T<<3)+Offset];
      P[0]=P[1]=P[2]=P[3]=VDP->XPal[K>>4];
      P[4]=P[5]=P[6]=P[7]=VDP->XPal[K&0x0F];
      P+=8;T++;
    }

    /* Refresh sprites */
    RefreshSprites(VDP);
  }

  /* Refresh screen border */
  RefreshBorder(VDP);
}
