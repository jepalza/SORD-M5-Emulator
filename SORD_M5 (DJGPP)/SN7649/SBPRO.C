#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned int word;
typedef unsigned char byte;

#define p_base 0x220

void volumen(byte canal, byte volumen);

byte dir_can[]={ 0x000,0x001,0x002,
                 0x008,0x009,0x00a,
                 0x010,0x011,0x012 };

// para el SN7649
word Freq[16];                   /* Channel frequencies, Hz          */
byte vol;                     /* Channel volumes, linear 0..255   */
byte NoiseMode[3];

static word Freqs[] =
{
#include "FMFreqs.h"
};


// activa registro
void sbproL(byte reg, byte val)
{
  byte c;
  outp(p_base+0,reg); for(c=1;c<7 ;c+=1) inp(p_base+0);  // selecciona registro
  outp(p_base+1,val); for(c=1;c<36;c+=1) inp(p_base+0);  // pasa valor a registro
}

void sbproR(byte reg, byte val)
{
  byte c;
  outp(p_base+2,reg); for(c=1;c<7 ;c+=1) inp(p_base+2);  // selecciona registro
  outp(p_base+3,val); for(c=1;c<36;c+=1) inp(p_base+2);  // pasa valor a registro
}

byte initFM(void)
{
  byte a,b,c;

  for (c=0;c<225;c++) sbproL(c,0); // resetea todos los registros
  for (c=0;c<225;c++) sbproR(c,0); // resetea todos los registros

  sbproR(5,1); //Pone el OPL3 en modo 18 canales (9+9 estereo)

// crea 9+9 canales (aunque solo se usan 8+8) de audio FM estandard
  for(c=0;c<9;c++)
  {
    sbproL(0x20+dir_can[c],0x01);
    sbproL(0x23+dir_can[c],0x01);
    sbproL(0x40+dir_can[c],0x1f);
    sbproL(0x43+dir_can[c],0x00);
    sbproL(0x60+dir_can[c],0xf0);
    sbproL(0x63+dir_can[c],0xf0);
    sbproL(0x80+dir_can[c],0x14);
    sbproL(0x83+dir_can[c],0x13);

    sbproR(0x20+dir_can[c],0x01);
    sbproR(0x23+dir_can[c],0x01);
    sbproR(0x40+dir_can[c],0x1f);
    sbproR(0x43+dir_can[c],0x00);
    sbproR(0x60+dir_can[c],0xf0);
    sbproR(0x63+dir_can[c],0xf0);
    sbproR(0x80+dir_can[c],0x14);
    sbproR(0x83+dir_can[c],0x13);

    sbproR(0xC0+c,0x11); //desvia el canal derecho al derecho
    sbproL(0xC0+c,0x11); // y el izquierdo al izquierdo
  }

 sbproR(0xC0,0x10+1); //desvia el canal derecho al derecho
 sbproL(0xC0,0x20+1); // y el izquierdo al izquierdo

  for (c=0;c<9;c++) sbproL(0xB0+c,0); // apaga las voces
  for (c=0;c<9;c++) sbproR(0xB0+c,0); // de todos los canales

  for (c=0;c<18;c++) volumen(c,63);   // pone volumen de cada canal, a tope (63)
}

void volumen(byte canal, byte volumen)
{
     if (canal<9)
       {
        sbproL(0x43+dir_can[canal],63-volumen);
        if (volumen==0) sbproL(0xB0+canal,0);
       }
     else
       {
        sbproR(0x43+dir_can[canal-9],63-volumen);
        if (volumen==0) sbproR(0xB0+(canal-9),0);
       }
}

void sound(byte canal, word freq)
{
   if (freq/5>=sizeof(Freqs)) freq=0;
   freq/=5;
  if (canal<9)
   {
    sbproL(0xA0+canal,Freqs[freq]&0xFF);
    sbproL(0xB0+canal,Freqs[freq]>>8);
    //sbproL(0xA0+canal,freq&0xFF);
    //sbproL(0xB0+canal,(freq>>8)+0x3c);
   }
  else
   {
    sbproR(0xA0+(canal-9),Freqs[freq]&0xFF);
    sbproR(0xB0+(canal-9),Freqs[freq]>>8);
    //sbproR(0xA0+(canal-9),freq&0xFF);
    //sbproR(0xB0+(canal-9),(freq>>8)+0x3c);
   }
}

void resetFM(void)
{
  byte c;
  for (c=0;c<9;c++) sbproL(0xB0+c,0); // apaga las voces
  for (c=0;c<9;c++) sbproR(0xB0+c,0); // de todos los canales
}

void SN7649(register byte val, register byte chip)
{
  static byte Buf;
  register byte N;
  register long J;

  chip=chip<<2;

         switch(val&0xF0)
         {
           case 0xE0:
             NoiseMode[chip>>2]=val&0x03;
             switch(val&0x03)
            {
               case 0: Freq[3+chip]=20000;break;
               case 1: Freq[3+chip]=10000;break;
               case 2: Freq[3+chip]=5000 ;break;
               case 3: Freq[3+chip]=Freq[2+chip];
             }
             sound(3+chip,Freq[3+chip]);
       //_settextposition(3+chip+1,1);
       //printf("NOISE %d: Freq=%dHz    \n",chip>>2,Freq[3+chip]);
             break;
           case 0x80: case 0xA0: case 0xC0:
             Buf=val;break;
           case 0x90: case 0xB0: case 0xD0: case 0xF0:
             N=((val-0x90)>>5)+chip;
             vol=((~val&0x0F)<<4)/4;
             volumen(N,vol);
       //_settextposition(N+1,25);
       //printf("Canal %d: Vol=%d    \n",N,Volume[N]);
             break;
           default:
             if(Buf&0xC0)
             {
              N=((Buf-0x80)>>5)+chip;
               J=(val&0x3F)*16+(Buf&0x0F)+1;
               J=131072L/J;
               Freq[N]=J<65535? J:0;
               //Freq[N]=J;
               if(((N-chip)==2)&&((NoiseMode[chip>>2])==3)) sound(3+chip,Freq[3+chip]=Freq[2+chip]);
               sound(N,Freq[N]);
       //_settextposition(N+1,1);
       //printf("Canal %d: Freq=%ldHz    \n",N,Freq[N]);
             }
         }
       }

