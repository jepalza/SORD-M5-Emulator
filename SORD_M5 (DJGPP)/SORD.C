#include <unistd.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <stdio.h>
#include <pc.h>
#include <stdlib.h>
#include <sys/movedata.h>
#include "../z80/z80.h"
#include "tms9918.h"
#include <allegro.h>


#include <sys/farptr.h> //es solo para el farpokeb
#include <go32.h>       //es solo para el farpokeb: en concreto para el _dos_ds


// 0000h-3fffh 16k VRAM memoria de video  ???? quizas no empiece en la 0000h

// 0000h-1fffh 8k ROM interna del sistema                        (8k)
// 2000h-3fffh 8k ROM externa del cartucho de BASIC-I            (8k)
// 4000h-5fffh 8k ROM externa del cartucho de BASIC-G o de FALC  (16k)
// 6000h-6fffh 4k ROM externa del cartucho de BASIC-F            (20k)
// 7000h-7fffh 4k RAM interna

// 8000h-ffffh 4k RAM ampliacion del BASIC-G solo
//               o bien 24k RAM extras con cartucho ampliacion de RAM

    Z80_Regs CPU;  // registros de la CPU

byte *rom  ; // rom de 64k, incluida la RAM
byte *vram ; // ram de VRAM o video RAM (16k) desde la 0 a la 16383 (0x4000)
byte *ramp ; // para copiar a la VGA
byte *tape ; // buffer de cintas
TMS9918 *vdp; // registros del VDP (VRAM) del SORD

byte bits[10]; //para almacenar los 10 bits de cada dato de cinta: un0+8bits+un1

 long Z80_IPeriod;
 long Z80_ICount ;
 int Z80_IRQ;

 int Z80_IRQ_CPU=0x6;
 long Z80_IPeriod=34000; // 4.3 Mhz ?? = 430000 ??? velocidad real ???

byte TEXTO=0;

byte port00=0;
word port01=0;
byte port02=0;
byte port03=0;

byte port10=0; // sacar o meter bytes de la VRAM segun direccion Port 0x11
byte port11=0; // activar direccion de lectura o escritura en la VRAM

byte port20=0; // generador de sonidos: por estudiar aun
byte port40=0;
int  port50=2; // contador de posiciones del buffer de cintas
byte load=0;

byte intI=6; // para iel contador de interrupciones

extern volatile char key[128];

void run_CPU(void);

byte initFM(void);
void resetFM(void);
void SN7649(byte val,byte chip);

void paleta(void);
void dibujar(void);
void cartucho (void);
int teclado(byte fila);
void saveram(char *fichero,char *zona,Z80_Regs *cpu);
void leerom(char *fichero,char *zona);

int saveyes=0; // si se pulsa F3 se guarda la RAM
int loadyes=0; // si se pulsa F4 se recupera la RAM

#include "fich.c"
void Z80_WRMEM(register dword a,register byte v)
{

  // es necesario que no se pueda escribir entre la 6000 y la 7000, para que el
  // test de ram minima se establezca en 7000 (como en el SORD real)
  if (a<0x7000) return;

  // y seguido que no se pueda mas alla de 8000 (entre 7000 y 8000 hay 4ks)
  // para que solo tenga 4k, como en el SORD real.
  // por ahora es para que sea igual al real, luego se puede quitar
  // para que tenga 32k de ram (de 7000 a ffff)
  if (a>0x7fff) return;

  rom[a]=v;
}

unsigned Z80_RDMEM(register dword a)
{
#ifdef DEBUG
  if (key[KEY_F12])
    {
     set_gfx_mode(GFX_TEXT,80,25,0,0);
     Z80_Trace=1; // entrar en modo debug
    }
#endif


  // if (a==0x16b6) Z80_Trace=1; /* set_gfx_mode(GFX_TEXT,80,25,0,0);*/
  // if (a==0x1659) port01=0; /* set_gfx_mode(GFX_TEXT,80,25,0,0);*/
  // if (a==0x1739) printf("%04x %04x\n",CPU.PC,CPU.AF);
  // if (a>0x2000 && a<0x4000) { Z80_Running=0; }

   //  anula las llamadas al CRCeo de las ROM, para poder trucar bytes de prueba
   // if (a==0xca) return 0xc9;
   // if (a==0xc7) return 0xc9;
   //  y seguido... bytes de prueba!!!
   // if (a==0x15b1) return 0x38;
   // if (a==0x15b6) return 0x0;

  return rom[a];
}


int Z80_Interrupt(void)
{

  intI+=2; if (intI==8) intI=0;

 return intI;
}

void leerom(char *fichero,char *zona)
{
      FILE *Fich;
      char *C=zona;

        if((Fich = fopen(fichero, "rb"))==NULL) {
          printf("Error cargando fichero.");
          exit(1); }

        while(!feof(Fich)) { *C++=getc(Fich);}
        if(fclose(Fich)!=0) {
          printf("No se puede cerrar el fichero.");
          exit(1);}
}

void saveram(char *fichero,char *zona,Z80_Regs *cpu)
{
      FILE *Fich;
      char *C;
      int a;

        if((Fich = fopen(fichero, "wb"))==NULL) {
          printf("Error abriendo fichero.");
          exit(1); }

        // guarda la RAM entre la 0x7000 y la 0x8000 (4k's)
        C=zona;
        a=0;
        while(a++<4096) { putc(*C++,Fich);}

        // guarda los registros del Z80
        C=(char *)cpu;
        a=0;
        while(a++<256) { putc(*C++,Fich);}

        if(fclose(Fich)!=0) {
          printf("No se puede cerrar el fichero.");
          exit(1);}
}



int main(int argc,char *argv[])
{

      if(!(rom=malloc(0x10000))) {
       printf("No hay suficiente memoria.");
       exit(1); }

      if(!(ramp=malloc(0x10000))) {
       printf("No hay suficiente memoria.");
       exit(1); }

      if(!(vram=malloc(0x10000))) {
       printf("No hay suficiente memoria.");
       exit(1); }

      if(!(tape=malloc(40000))) {
       printf("No hay suficiente memoria.");
       exit(1); }

       memset(ramp,13,0x10000);

       memset( rom,0,0x10000);
       memset(vram,0,0x10000);

       leerom("k.m5",tape);

       leerom("system.rom",rom);
       // leerom("basic-i.rom",rom+8192);
       // leerom("basic-f.rom",rom+8192);
       // leerom("basic-g.rom",rom+8192);
       // leerom("falc.rom",rom+8192);
       // leerom("designer.rom",rom+8192);

      allegro_init();
      if  (TEXTO) set_gfx_mode(GFX_TEXT,80,50,0,0);
      if (!TEXTO) set_gfx_mode(GFX_VGA,320,200,0,0);
      paleta();

      New9918(vdp,ramp,320,200);
      install_keyboard();
      initFM();
      cartucho();

      run_CPU();

      Trash9918(vdp);
      resetFM();
      free (rom);
      free (vram);
      free (ramp);
      remove_keyboard();
      set_gfx_mode(GFX_TEXT,80,25,0,0);
      printf("SORD-M5 emulator: Joseba Epalza   1-OCT-00   Version 1.1  <jepalza@teleline.es>\n");
      exit(1);
}

void paleta(void)
        {
        int a,c;

        char pal[]=
        {

  0x00,0x00,0x00, 0x00,0x00,0x00, 0x20,0xC0,0x20, 0x60,0xE0,0x60,
  0x20,0x20,0xE0, 0x40,0x60,0xE0, 0xA0,0x20,0x20, 0x40,0xC0,0xE0,
  0xE0,0x20,0x20, 0xE0,0x60,0x60, 0xC0,0xC0,0x20, 0xC0,0xC0,0x80,
  0x20,0x80,0x20, 0xC0,0x40,0xA0, 0xA0,0xA0,0xA0, 0xE0,0xE0,0xE0

        };

        // paleta de los graficos
        a=128;
        for (c=0;c<47;c+=3)
         {
          outp(0x3c8,a++);
          outp(0x3c9,pal[c+0] >> 2);
          outp(0x3c9,pal[c+1] >> 2);
          outp(0x3c9,pal[c+2] >> 2);

         }
}

void dibujar(void)
{
  register word x,y;
  register char *pp;
  static pan=0x7000;

if (TEXTO)
{
        if (key[KEY_F1]) pan+=128;
        if (key[KEY_F2]) pan-=128;
        if (key[KEY_F3]) pan+=32;
        if (key[KEY_F4]) pan-=32;
        if (key[KEY_F5]) pan+=1;
        if (key[KEY_F6]) pan-=1;

    gotoxy(50,1); printf("%04X\n",pan);
    pp=rom+pan; // pruebas
    for (x=0;x<50;x++)
     {
      for (y=0;y<64;y+=2)
        _farpokeb(_dos_ds,0xb8000+((x*160)+y),*pp++);
     }

 return;
}

     if (key[KEY_F1])
       {

        set_gfx_mode(GFX_VGA,320,200,0,0);

        printf("F1: THIS HELP\n    ESTA AYUDA\n\n");
        printf("F2: LOAD ROM CART\n    CARGAR CARTUCHO DE ROM\n\n");
        printf("F3: SAVE RAM FILE\n    GRABAR LA RAM EN UN FICHERO\n\n");
        printf("F4: LOAD RAM FILE\n    CARGAR UN FICHERO DE RAM\n\n");
        printf("F5: SAVE PCX FILE\n    GRABAR IMAGEN PCX\n\n");
        printf("\n\n\n");
        printf("ESC: EXIT\n    SALIR\n");


          do {} while (key[KEY_F1]);
          do {if (key[KEY_ESC]) break;} while (!key[KEY_F1]);

        set_gfx_mode(GFX_VGA,320,200,0,0);
        paleta();

       }

     if (key[KEY_F2])
       {
         cartucho();
       }

     if (key[KEY_F3])
       {
         saveyes=1;
       }

     if (key[KEY_F4])
       {
         // loadyes=1;
         leerom("k.m5",rom+0x7383);
       }

     if (key[KEY_F5])
       {

         BITMAP *bmp;
         PALETTE pal;

         paleta();
         get_palette(pal);
         bmp = create_sub_bitmap(screen, 0, 0, SCREEN_W, SCREEN_H);
         save_bitmap("pantalla.pcx", bmp, pal);
         destroy_bitmap(bmp);

       }

     dosmemput (ramp,64000,0xa0000);

}

void cartucho (void)
{
        set_gfx_mode(GFX_VGA,320,200,0,0);

        printf("Leer un cartucho de ROM\n");
        printf("Read a ROM cartridge\n");
        printf("\n\n\n");

        {
          char *fichrom;

          fichrom=trata_fich("*.rom");
          leerom(fichrom,rom+8192);

        }

        set_gfx_mode(GFX_VGA,320,200,0,0);
        paleta();
}

int teclado (byte fila)
{
  int tec=0;
  static byte teclaborrar=0;
  static byte teclaleft=0;
  static byte teclaright=0;
  static byte teclaup=0;
  static byte tecladown=0;



    // TECLAS ESPECIALES CURRADAS POR MI, COMO CURSORES O DELETE

   if (key[KEY_BACKSPACE]) teclaborrar=1;
   if (key[KEY_LEFT ]) teclaleft=1;
   if (key[KEY_RIGHT]) teclaright=1;
   if (key[KEY_UP   ]) teclaup=1;
   if (key[KEY_DOWN ]) tecladown=1;



  if (fila==0)
    {

     if (key[KEY_LCONTROL]) tec^=0x01; // IDEM: CONTROL DEL SORD
     if (key[KEY_RCONTROL]) tec^=0x01; //       CONTROL DEL SORD
     if (key[KEY_ALT])      tec^=0x02; // TECLA FN: TECLA ALT IZQ.
     if (key[KEY_LSHIFT])   tec^=0x04; // SHIFT IZQUIERDO
     if (key[KEY_RSHIFT])   tec^=0x08; // SHIFT DERECHO
     // if (key[KEY_])        tec^=0x10; // NO HACE NADA ?????
     // if (key[KEY_])        tec^=0x20; // NO HACE NADA ?????
     if (key[KEY_SPACE])    tec^=0x40; // ESPACIO
     if (key[KEY_ENTER])    tec^=0x80; // enter

     if (teclaborrar)  tec=0x01;   // retorna CONTROL
     if (teclaleft)    tec=0x01;
     if (teclaright)   tec=0x01;
     if (teclaup)      tec=0x01;
     if (tecladown)    tec=0x01;
    }
  if (fila==1)
    {
     if (key[KEY_1]) tec^=0x01;
     if (key[KEY_2]) tec^=0x02;
     if (key[KEY_3]) tec^=0x04;
     if (key[KEY_4]) tec^=0x08;
     if (key[KEY_5]) tec^=0x10;
     if (key[KEY_6]) tec^=0x20;
     if (key[KEY_7]) tec^=0x40;
     if (key[KEY_8]) tec^=0x80;
    }
  if (fila==2)
    {
     if (key[KEY_Q]) tec^=0x01;
     if (key[KEY_W]) tec^=0x02;
     if (key[KEY_E]) tec^=0x04;
     if (key[KEY_R]) tec^=0x08;
     if (key[KEY_T]) tec^=0x10;
     if (key[KEY_Y]) tec^=0x20;
     if (key[KEY_U]) tec^=0x40;
     if (key[KEY_I]) tec^=0x80;
    }
  if (fila==3)
    {
     if (key[KEY_A]) tec^=0x01;
     if (key[KEY_S]) tec^=0x02;
     if (key[KEY_D]) tec^=0x04;
     if (key[KEY_F]) tec^=0x08;
     if (key[KEY_G]) tec^=0x10;
     if (key[KEY_H]) tec^=0x20;
     if (key[KEY_J]) tec^=0x40;
     if (key[KEY_K]) tec^=0x80;
    }
  if (fila==4)
    {
     if (key[KEY_Z]) tec^=0x01;
     if (key[KEY_X]) tec^=0x02;
     if (key[KEY_C]) tec^=0x04;
     if (key[KEY_V]) tec^=0x08;
     if (key[KEY_B]) tec^=0x10;
     if (key[KEY_N]) tec^=0x20;
     if (key[KEY_M]) tec^=0x40;
     if (key[KEY_COMMA]) tec^=0x80;
    }
  if (fila==5)
    {
     if (key[KEY_9]) tec^=0x01;
     if (key[KEY_0]) tec^=0x02;
     if (key[KEY_SLASH]) tec^=0x04; // MENOS - (OJO: SEGUN TECLADO US)
     // if (key[KEY_]) tec^=0x08; // simbolo ^
     if (key[KEY_STOP]) tec^=0x10; // punto
     // if (key[KEY_]) tec^=0x20; // SLASH (TECLA DEL 7)
     // if (key[KEY_]) tec^=0x40; // subrayado (underscore)
     if (key[KEY_TILDE]) tec^=0x80; // BACKSLASH

     if (teclaborrar) { tec=0x80; teclaborrar=0; }
     if (teclaup)     { tec=0x08; teclaup=0; }
     if (tecladown)   { tec=0x20; tecladown=0; }

    }
  if (fila==6)
    {
     if (key[KEY_O]) tec^=0x01;
     if (key[KEY_P]) tec^=0x02;
     // if (key[KEY_]) tec^=0x04; // arroba @
     if (key[KEY_OPENBRACE] ) tec^=0x08; // ABRIR  CORCHETE Y LLAVE
     if (key[KEY_L]) tec^=0x10;
     if (key[KEY_COLON]) tec^=0x20; // PUNTO Y COMA (¥ EN EL SP)
     if (key[KEY_QUOTE]) tec^=0x40; // DOS PUNTOS   (ABRIR LLAVE EN EL SP)
     if (key[KEY_CLOSEBRACE]) tec^=0x80; // CERRAR CORCHETE Y LLAVE

     if (teclaleft)  { tec=0x20; teclaleft=0; }
     if (teclaright) { tec=0x40; teclaright=0; }

    }

  if (fila==7)
    {
      // JOYSTICKS SIN IMPLEMENTAR AUN
    }
  return tec;
}

void run_CPU(void)
{
    Z80_Reset();
    Z80_GetRegs(&CPU);

 do
  {

   // para ejecutar la CPU
   Z80_IRQ=Z80_IRQ_CPU;
   Z80_IPeriod=Z80_IPeriod;
   Z80_ICount =Z80_IPeriod;
   Z80_SetRegs(&CPU);
   Z80_Execute();
   Z80_GetRegs(&CPU);
  if (saveyes)
    {
        saveram("DUMP.RAM",rom+0x7000,&CPU);
        saveyes=0;
    }
  if (loadyes)
    {
        int a;
        char *c;

        leerom("DUMP.RAM",rom+0x7000);
        c=&CPU;
        for (a=0;a<256;a++) c[a]=rom[0x8000+a];
        loadyes=0;
        Z80_SetRegs(&CPU);
        Z80_Reset();
    }
  if (key[KEY_ESC]) Z80_Running=0; //salir

  } while (Z80_Running);
}

void Z80_Reti(void) { }
void Z80_Retn(void) { }
byte Z80_In(register byte Port)
 {
   port01-=1;


   // if (Port==0x00) return port00-=1;
   if (Port==0x01) return port01;

   if (Port==0x50)
     {
       static byte c=0;
       static byte e=0;
       byte f,g,h;

       if (!c)
        {
          c=10;
//          printf("\n");
          f=tape[port01++]; h=f;
          for (g=9;g>1;g--) { bits[g]=f&1; f=f>>1; }
          bits[1]=1; bits[10]=0; // bits de sinc. inic. y final (se lee desde el ultimo al primero)
//          gotoxy(1,1); printf("%04x %02x ",port01,h);
//          for (g=1;g<11;g++) printf ("%d",bits[g]);
        }

       if (!e)
        {
          e=1;
//          printf(".");
          return 255;
        }
        else
        {
          e=0;
          f=bits[c--];
          if (!f)
            {
//              printf("0");
              return 253;
            }
           else
            {
//              printf("1");
              return 254;
            }
        }

     }

   // if (Port==0x02) return port02-=1;
   // if (Port==0x03) return port03-=1;


   // if (Port==0x20) Z80_Running=0; // estado de los sonidos

   // if (Port==0x40) return 0;

//   if (Port==0x50)
//      {
//       if (port50) return port50=0; else return port50=1;
//      }

   if (Port==0x10) return RdData9918(vdp);
   if (Port==0x11) return RdCtrl9918(vdp);

   // lectura de las diversas filas de teclas
   if (Port==0x30) return teclado(0);
   if (Port==0x31) return teclado(1);
   if (Port==0x32) return teclado(2);
   if (Port==0x33) return teclado(3);
   if (Port==0x34) return teclado(4);
   if (Port==0x35) return teclado(5);
   if (Port==0x36) return teclado(6);

   if (Port==0x37) return teclado(7); // JOYSTICKS

   // printf("        INPUT %02X \n",Port);
   return 0;
 }
void Z80_Out(register byte Port,register byte Value)
 {

      if (!vdp->Line) dibujar();
      Loop9918(vdp);
/*
   if (Port==0x00) printf("00: %02x\n",Value);
   if (Port==0x01) printf("01: %02x\n",Value);
   if (Port==0x02) printf("02: %02x\n",Value);
   if (Port==0x03) printf("03: %02x\n",Value);
*/
//   if (Port==0x00) { port00=Value; return; }
   if (Port==0x01) { port01=255; return; }
//   if (Port==0x02) { port02=Value; return; }
//   if (Port==0x03) { port03=Value; return; }


   if (Port==0x20)
    {
      port20=Value;  // generador de sonidos: por estudiar
      SN7649(Value,0);
      SN7649(Value,1);
      return;
      // (a==0xa400) { SN7649(v,1); return;}
      // (a==0xa800) { SN7649(v,2); return;}
      // (a==0xac00) { SN7649(v,3); return;}
    }

   // if (Port==0x40) { port40=Value; return; }
   // if (Port==0x50) if (Value==2) { load=1; return; }; // comando "SAVE"

   if (Port==0x10) { WrData9918(vdp,Value); return; }
   if (Port==0x11) { WrCtrl9918(vdp,Value); return; }


   //if (Port==0x20 && Value) printf("OUT %02X %02X\n",Port,Value);
   //if (Port==0x50 && Value) printf("OUT %02X %02X\n",Port,Value);
   //if (Port==0x40 && Value) printf("OUT %02X %02X\n",Port,Value);

 }


void Z80_Patch(Z80_Regs *R){}

// exclusivos del DEBUG
void SetGraphMode(void)
{
 //set_gfx_mode(GFX_TEXT,80,50,0,0);
 set_gfx_mode(GFX_VGA,320,200,0,0);
 paleta();
}
void Process_Screen(void)
{
 set_gfx_mode(GFX_VGA,320,200,0,0);
 paleta();
}

