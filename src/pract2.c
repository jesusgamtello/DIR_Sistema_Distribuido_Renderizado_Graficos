/* Pract2  RAP 09/10    Javier Ayllon*/

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h> 
#include <assert.h>   
#include <unistd.h>   
#define NIL (0)    

#define N 10
#define FOTO "include/foto.dat"
#define SIZE 400
#define MODE 2


/*Variables Globales */

XColor colorX;
Colormap mapacolor;
char cadenaColor[]="#000000";
Display *dpy;
Window w;
GC gc;

/*Funciones auxiliares */

void initX() {

      dpy = XOpenDisplay(NIL);
      assert(dpy);

      int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
      int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

      w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                     400, 400, 0, blackColor, blackColor);
      XSelectInput(dpy, w, StructureNotifyMask);
      XMapWindow(dpy, w);
      gc = XCreateGC(dpy, w, 0, NIL);
      XSetForeground(dpy, gc, whiteColor);
      for(;;) {
            XEvent e;
            XNextEvent(dpy, &e);
            if (e.type == MapNotify)
                  break;
      }


      mapacolor = DefaultColormap(dpy, 0);

}

void dibujaPunto(int x,int y, int r, int g, int b) {

        sprintf(cadenaColor,"#%.2X%.2X%.2X",r,g,b);
        XParseColor(dpy, mapacolor, cadenaColor, &colorX);
        XAllocColor(dpy, mapacolor, &colorX);
        XSetForeground(dpy, gc, colorX.pixel);
        XDrawPoint(dpy, w, gc,x,y);
        XFlush(dpy);

}

void puntos(MPI_Comm commPadre){
      MPI_Status status;

      int buff[5];
      for (int i=0;i<(SIZE*SIZE);i++){
            MPI_Recv(&buff,5,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,commPadre,&status);
            dibujaPunto(buff[0],buff[1],buff[2],buff[3],buff[4]);
      }

}



/* Programa principal */

int main (int argc, char *argv[]) {

  int rank,size;
  MPI_Comm commPadre;
  int tag;
  MPI_Status status;
  int buf[5];
  int array_of_errcodes[5];


  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_get_parent( &commPadre );
  if ( (commPadre==MPI_COMM_NULL) && (rank==0) )  {

      initX();

      /* Codigo del maestro */
      MPI_Comm_spawn("pract2",MPI_ARGV_NULL,N,MPI_INFO_NULL,0,MPI_COMM_WORLD,&commPadre,array_of_errcodes);
      /*En algun momento dibujamos puntos en la ventana algo como
      dibujaPunto(x,y,r,g,b);  */
      puntos(commPadre);

      sleep(5);

      }

  

 	
  else {
    /* Codigo de todos los trabajadores */
    /* El archivo sobre el que debemos trabajar es foto.dat */
      MPI_File f;

      int filas,tamaniobloque,inicio,fin;

      filas=SIZE/N;
      tamaniobloque=filas*SIZE*3*sizeof(unsigned char);
      inicio=rank*filas;
      fin=((rank+1)*filas);
      if (rank==N-1){
            fin=SIZE;
      }
      MPI_File_open(MPI_COMM_WORLD,FOTO,MPI_MODE_RDONLY,MPI_INFO_NULL,&f);
      MPI_File_set_view(f,rank*tamaniobloque,MPI_UNSIGNED_CHAR,MPI_UNSIGNED_CHAR,"native",MPI_INFO_NULL);

      unsigned char colores[3];
      int i,j;

      for(i=inicio;i<fin;i++){
            for(j=0;j<SIZE;j++){

                  MPI_File_read(f,colores,3,MPI_UNSIGNED_CHAR,&status);
                  buf[0]=j;
                  buf[1]=i;

                  switch (MODE)
                  {
                  case 0:
                        buf[2]=(int) colores[0];
                        buf[3]=(int) colores[1];
                        buf[4]=(int) colores[2];
                        break;
                  
                  case 1://B&N
                        buf[2]=((int) colores[0]+(int) colores[1]+(int) colores[2])/3;
                        buf[3]=((int) colores[0]+(int) colores[1]+(int) colores[2])/3;
                        buf[4]=((int) colores[0]+(int) colores[1]+(int) colores[2])/3;
                        break;

                  case 2://sepia
                        buf[2]=((int) colores[0]*0.393+(int) colores[1]*0.769+(int) colores[2]*0.189);
                        buf[3]=((int) colores[0]*0.349+(int) colores[1]*0.686+(int) colores[2]*0.168);
                        buf[4]=((int) colores[0]*0.272+(int) colores[1]*0.534+(int) colores[2]*0.131);
                        break;

                  case 3://invertimos color
                        buf[2]=255-(int) colores[0];
                        buf[3]=255-(int) colores[1];
                        buf[4]=255-(int) colores[2];
                        break;
                  
                  default:
                        buf[2]=(int) colores[0];
                        buf[3]=(int) colores[1];
                        buf[4]=(int) colores[2];
                        break;
                  }
                  
                  MPI_Bsend(&buf,5,MPI_INT,0,1,commPadre);

            }
      }

     
      
      MPI_File_close(&f);
  }

  MPI_Finalize();
  return EXIT_SUCCESS;

}

