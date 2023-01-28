#include <string.h>
#include <dir.h>
#include <allegro.h>

extern volatile char key[128];


void * trata_fich (char *ext)
{
     struct ffblk f;
     int ini=10;
     int iniold=0;
     int d;
     int aa=0;
     int done ;
     int files=10; // empieza en el 10 para dar huecos libres al menu de flechas
     char dir[1024][13];

     for (done=0;done<1024;done++) { dir[done][0]=0x20; dir[done][1]=0x0; }

     done = findfirst(ext, &f, FA_ARCH|FA_RDONLY);
     while (!done)
     {
      /*
       // pruebas
       printf("%d %10u %2u:%02u:%02u %2u/%02u/%4u %s\n", files,
         f.ff_fsize,
         (f.ff_ftime >> 11) & 0x1f,
         (f.ff_ftime >>  5) & 0x3f,
         (f.ff_ftime & 0x1f) * 2,
         (f.ff_fdate >>  5) & 0x0f,
         (f.ff_fdate & 0x1f),
         ((f.ff_fdate >> 9) & 0x7f) + 1980,
         f.ff_name);
      */
         strcpy(dir[files++],f.ff_name);

       done = findnext(&f);
     }

  // if (files==10) files=20;

  gotoxy(12,11); printf(" ÚÄÄÄÄÄÄÄÄÄÄÄÄ¿ \n");
  for (d=12;d<22;d++)
  { gotoxy(12,d ); printf(" ³            ³ \n"); }
  gotoxy(12,22); printf(" ÀÄÄÄÄÄÄÄÄÄÄÄÄÙ \n");
  gotoxy(12,17); printf("->            <-\n");

  do
   {
    if (ini!=iniold)
    {
      iniold=ini;
      for (d=ini;d<(ini+10);d++)
        {
          gotoxy(14,(d-ini)+12);
          printf("%12s\n",dir[d]);
        }
    }

    if (key[KEY_UP  ]) ini-=1;
    if (key[KEY_DOWN]) ini+=1;
    if (key[KEY_ENTER]) return dir[ini+5];

      for (d=0;d<65535;d++) for(done=0;done<4000;done++); //Pausa kk


    if (ini<5) ini=5;
    if (ini>(files-6)) ini=files-6;

   } while (!key[KEY_ESC]);


}

