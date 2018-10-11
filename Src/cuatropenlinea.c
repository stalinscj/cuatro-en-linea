#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <time.h>
#include <gtk/gtk.h>

#define TAMPANTALLAX 1000
#define TAMPANTALLAY 485


//usleep(1000000); esperara 1 segundo


typedef struct CASILLA{
	
	int pidMarco;
	int pidBola;
	char color;
	bool estaOcupada;
	
}Casilla;

typedef struct NODO{
	int puntosActualesJ1;
	int puntosActualesJ2;
	int pidPuntosActuales;
	struct NODO *sig;
}Nodo;

typedef struct RECORDS{
	
	double horizontal;
	double vertical;
	double diagonalP;
	double diagonalS;

}Records;

typedef struct PUNTEROS{
	
	int *fila; 
	int *columna;
	Casilla (*tablero)[];
	key_t *claveMemoria;
	int *idMemoria;
	Casilla *buffer; 
	Records *mejorTiempo;
	struct timeval *inicio;
	struct timeval *fin; 
	Nodo *puntos;
	int *modo;
	int *colorJ1; 
	int *turno;
	GtkWidget* ventana;
	GtkWidget* contenedor;
	GtkWidget* (*G_tablero)[];
	pthread_t *hilo;
	bool *finalizarHilo;
	int *dificultad;
	
	
}Punteros;


bool establecerDimensiones(int argc, char **argv, int *fila, int *columna, GtkWidget *cboNroFilas){
	
	if(argc==1){
		*fila=6;
		*columna=7;
	}else{
		*fila = atoi(argv[1]);
		if((*fila%2==0 && ((*fila>5)&&(*fila<13)))){
			*columna = *fila+1;
		}else{
			printf("El numero de filas debe ser un numero par mayor a 5 y menor a 13\n");
			return false;
		}
	}
	
	gtk_combo_box_set_active(GTK_COMBO_BOX(cboNroFilas), (*fila-6)/2);
	
	return true;
}

void aislar(){
		
	//printf("soy %d hijo de %d y estoy aislado\n",getpid(),getppid());
	kill(getpid(),SIGSTOP);
	//printf("soy %d hijo de %d y YA NO estoy aislado\n",getpid(),getppid());
		

}

void inicializarTablero(int fila, int columna,Casilla tablero[fila][columna]){
	
	Casilla casillaNueva;
	
	casillaNueva.estaOcupada=0;
	casillaNueva.pidBola=-1;
	casillaNueva.pidMarco=-1;
	casillaNueva.color=' ';
	
	int i,j,pid;
	
	for(i=0;i<fila;i++){
		for(j=0;j<columna;j++){
			pid=fork();
			
			if(pid==0)
				break;
			casillaNueva.pidMarco=pid;
			tablero[i][j]=casillaNueva;
			
		}
		if(pid==0)
			break;
	}
	
	if(pid==0){	
		//printf("soy %d hijo de %d y sali del for\n",getpid(),getppid());
		aislar(getpid());
	}
	
	//printf("soy %d hijo de %d y no estoy aislado\n",getpid(),getppid());
	
	usleep(100);
}

void imprimirCasillas(int fila, int columna,Casilla tablero[fila][columna]){
	
	int i,j;
	for(i=0;i<fila;i++)
		for(j=0;j<columna;j++){
			printf("\nmarco[%d][%d]=%d\n",i,j,tablero[i][j].pidMarco);
			printf("bola[%d][%d]=%d\n",i,j,tablero[i][j].pidBola);
			printf("ocupada[%d][%d]=%d\n\n",i,j,tablero[i][j].estaOcupada);
			printf("color[%d][%d]=%c",i,j,tablero[i][j].color);
		}
	
}

void liberarBuffer(int idMemoria,Casilla *buffer){
		
	shmdt ((char *)buffer);
	shmctl (idMemoria, IPC_RMID, (struct shmid_ds *)NULL);

}

void agruparBolaYMarco(int pidMarco){
	
	key_t claveMemoria = ftok("/bin/ls" , 33);
	int idMemoria = shmget(claveMemoria,sizeof(Casilla), 0777 | IPC_CREAT);
	Casilla *buffer = shmat(idMemoria,0,0);
	
	buffer->pidMarco = pidMarco;
	buffer->estaOcupada = true;

}

void manejador(int numSignal){
		
	switch(numSignal){
		
		case SIGINT:{
			//g_print("\n\nSe quito el pause en manejador\n\n");
			
		}break;
		
		case SIGCONT:{
			
			agruparBolaYMarco(getpid());
			//printf("soy %d y estoy en SIGCONT\n",getpid());
			aislar();
		}break;
	
	}
}

int filaMasBajaLibre(int fila, int columna, Casilla tablero[fila][columna], int columnaDeseada){
	
	int i;
	
	if(columnaDeseada>=0 && columnaDeseada<columna)
		for(i=fila-1;i>=0;i--){
		
			if(!tablero[i][columnaDeseada].estaOcupada)
				return i;
		}
	
	return -1;

}

void actualizarTablero(int fila, int columna, Casilla tablero[fila][columna], int filaDeseada, int columnaDeseada, Casilla *buffer){
	
	while(!buffer->estaOcupada){
		usleep(100);
	}
	
	tablero[filaDeseada][columnaDeseada].pidMarco = buffer->pidMarco;
	tablero[filaDeseada][columnaDeseada].pidBola = buffer ->pidBola;
	tablero[filaDeseada][columnaDeseada].estaOcupada = buffer->estaOcupada;
	tablero[filaDeseada][columnaDeseada].color = buffer->color;
	
	buffer->pidMarco =-1;
	buffer->pidBola =-1;
	buffer->estaOcupada=false;
	buffer->color=' ';
	
}

void inicializarMemoriaCompartida(key_t *claveMemoria, int *idMemoria, Casilla **buffer){
	
	*claveMemoria = ftok("/bin/ls" , 33);
	*idMemoria = shmget(*claveMemoria,sizeof(Casilla), 0777 | IPC_CREAT);
	*buffer = shmat(*idMemoria,0,0);
	
}

void crearBola(int fila, int columna, Casilla tablero[fila][columna], int filaDeseada, int columnaDeseada, Casilla *buffer, char color ){
	
	int pid = fork();
	
	if(pid==0){	
		//printf("soy bola %d hijo de %d\n",getpid(),getppid());
		buffer->pidBola = getpid();
		buffer->color = color;
		kill(tablero[filaDeseada][columnaDeseada].pidMarco,SIGCONT);
		aislar();
	}else
		actualizarTablero(fila,columna,tablero,filaDeseada,columnaDeseada,buffer);
}

void clrscr(){
	write(1,"\E[H\E[2J",7);
}

void gotoxy(int x,int y){
	printf("%c[%d;%df",0x1B,y,x);
}

void visualizarTablero(int fila, int columna, Casilla tablero[fila][columna]){
	
	clrscr();
	
	int i,j;
	
	int x=10,y=2;
	gotoxy(10,0);
	printf("0 1 2 3 4 5 6");
	
	for(i=0;i<fila;i++){
		for(j=0;j<=columna;j++){
			gotoxy(x,y);
			printf("|");
			x = x + 2;
		}
		y=y+2;
		x=10;
	}
	
	x=11;
	y=1;
	
	for(i=0;i<=fila;i++){
		for(j=0;j<columna;j++){
			gotoxy(x,y);
			printf("_");
			x = x + 2;
		}
		y=y+2;
		x=11;
	}
	
	for(i=0;i<fila;i++)
		for(j=0;j<columna;j++){
			gotoxy(2*j+11,2*i+2);
			printf("%c",tablero[i][j].color);
		}

}

void inicializarPuntos(Nodo **puntos){
	
	int pid = fork();
	
	if(pid==0)
		aislar();
	else{
		
		Nodo* nuevo = (Nodo*) malloc(sizeof(Nodo));
	
		nuevo->puntosActualesJ1 = 0;
		nuevo->puntosActualesJ2 = 0;
		nuevo->pidPuntosActuales= pid;
		nuevo->sig = NULL;
		
		*puntos=nuevo;	
		
	}
	
}

void agregarPunto(int puntoParaJ1, int puntoParaJ2, Nodo **puntos){
	
	int pid = fork();
	
	if(pid==0)
		aislar();
	else{
		
		Nodo* nuevo = (Nodo*) malloc(sizeof(Nodo));
		
		nuevo->puntosActualesJ1 = (**puntos).puntosActualesJ1+puntoParaJ1;
		nuevo->puntosActualesJ2 = (**puntos).puntosActualesJ2+puntoParaJ2;
		nuevo->pidPuntosActuales= pid;
		nuevo->sig = *puntos;
		
		*puntos=nuevo;	
	}
}

void liberarPuntos(Nodo **puntos){
		
	if (*puntos){
		
		liberarPuntos( &((*puntos)->sig) );
		kill((**puntos).pidPuntosActuales,SIGKILL);
		free(*puntos);
		*puntos = NULL;
	}
}

void barridoDeBolas(int fila, int columna, Casilla tablero[fila][columna],int tipoLinea, int fi, int ci, Nodo **puntos){

	
	if(tablero[fi][ci].color=='x')
		agregarPunto(1,0,puntos);
	else
		agregarPunto(0,1,puntos);
	
	
	int faux,caux,i,j;
	
	switch(tipoLinea){
		
		case 1:{
			
			for(i=0;i<4;i++){
				kill(tablero[fi][ci+i].pidBola,SIGKILL);
				tablero[fi][ci+i].pidBola = -1;
				tablero[fi][ci+i].color = ' ';
				tablero[fi][ci+i].estaOcupada = false;
			}
				
			
			j=0;
			
			while(j<4){
				
				i=1;
				faux=fi-i;
				caux=ci+j;
				while(faux>-1){
					
					while(faux<fila-1){
						if(!tablero[faux+1][caux].estaOcupada && tablero[faux][caux].estaOcupada){
							
							tablero[faux+1][caux].pidBola = tablero[faux][caux].pidBola;
							
							tablero[faux+1][caux].color = tablero[faux][caux].color;
							tablero[faux+1][caux].estaOcupada = true;
							
							tablero[faux][caux].pidBola = -1;
							tablero[faux][caux].color = ' ';
							tablero[faux][caux].estaOcupada = false;
						}
						
						faux++;
					}
					i++;
					faux=fi-i;
				}

				j++;
			}
			
			
			
			
			//printf("\n\n barrido horizontal\n\n");
			//sleep(4);
			
			//visualizarTablero(fila,columna,tablero);
			
			//getchar();
			//getchar();
			
			
		}break;
		
		case 2:{
			  
			for(i=0;i<4;i++){
				kill(tablero[fi+i][ci].pidBola,SIGKILL);
				tablero[fi+i][ci].pidBola = -1;
				tablero[fi+i][ci].color = ' ';
				tablero[fi+i][ci].estaOcupada = false;
			}
			
			i=1;
			faux=fi-i;
			
			
			while(faux>-1){
				
				while(faux<fila-1){
					if(!tablero[faux+1][ci].estaOcupada && tablero[faux][ci].estaOcupada){
						tablero[faux+1][ci].pidBola = tablero[faux][ci].pidBola;
						tablero[faux+1][ci].color = tablero[faux][ci].color;
						tablero[faux+1][ci].estaOcupada = true;
						
						tablero[faux][ci].pidBola = -1;
						tablero[faux][ci].color = ' ';
						tablero[faux][ci].estaOcupada = false;
					}
					
					faux++;
				}
				i++;
				faux=fi-i;
			}
			
			//printf("\n\n barrido vertical\n\n");
			//sleep(4);
			
			//visualizarTablero(fila,columna,tablero);
			
			//getchar();
			//getchar();
			
		
		}break;
		
		case 3:{
			
			for(i=0;i<4;i++){
				kill(tablero[fi+i][ci+i].pidBola,SIGKILL);
				tablero[fi+i][ci+i].pidBola = -1;
				tablero[fi+i][ci+i].color = ' ';
				tablero[fi+i][ci+i].estaOcupada = false;
			}
			
			
			j=0;
			
			while(j<4){
				
				i=1;
				faux=fi-i+j;
				caux=ci+j;
				
				
				
				while(faux>-1){
					
					while(faux<fila-1){
						
						if(!tablero[faux+1][caux].estaOcupada && tablero[faux][caux].estaOcupada){
							
							tablero[faux+1][caux].pidBola = tablero[faux][caux].pidBola;
							
							tablero[faux+1][caux].color = tablero[faux][caux].color;
							tablero[faux+1][caux].estaOcupada = true;
							
							tablero[faux][caux].pidBola = -1;
							tablero[faux][caux].color = ' ';
							tablero[faux][caux].estaOcupada = false;
						}
						
						faux++;
					}
					i++;
					faux=fi-i+j;
				}

				j++;
			}

			
			//printf("\n\n barrido diagonal principal\n\n");
			//sleep(4);
			
			//visualizarTablero(fila,columna,tablero);
			
			//getchar();
			//getchar();
			
			
			
		}break;
		
		case 4:{
			
			for(i=0;i<4;i++){
				kill(tablero[fi+i][ci-i].pidBola,SIGKILL);
				tablero[fi+i][ci-i].pidBola = -1;
				tablero[fi+i][ci-i].color = ' ';
				tablero[fi+i][ci-i].estaOcupada = false;
			}
			
			
			j=0;
			
			while(j<4){
				
				i=1;
				faux=fi-i+j;
				caux=ci-j;
				
				
				
				while(faux>-1){
					
					while(faux<fila-1){
						
						if(!tablero[faux+1][caux].estaOcupada && tablero[faux][caux].estaOcupada){
							
							tablero[faux+1][caux].pidBola = tablero[faux][caux].pidBola;
							
							tablero[faux+1][caux].color = tablero[faux][caux].color;
							tablero[faux+1][caux].estaOcupada = true;
							
							tablero[faux][caux].pidBola = -1;
							tablero[faux][caux].color = ' ';
							tablero[faux][caux].estaOcupada = false;
						}
						
						faux++;
					}
					i++;
					faux=fi-i+j;
				}

				j++;
			}

			
			//printf("\n\n barrido diagonal secundaria\n\n");
			//sleep(4);
			
			//visualizarTablero(fila,columna,tablero);
			
			//getchar();
			//getchar();
			
			
			
			
			
			
		}break;
	
	}
	
}

double calcularTiempo(struct timeval *inicio, struct timeval *fin)
{
  return (double)(fin->tv_sec + (double)fin->tv_usec/1000000) - (double)(inicio->tv_sec + (double)inicio->tv_usec/1000000);
}

void actualizarTiempo(int tipoLinea, struct timeval inicio[5], struct timeval fin[5], Records *mejorTiempo){
	
	double resultado;
	int i;
	
	if(tipoLinea==0){
		
		for (i=1;i<5;i++){
			inicio[i].tv_sec = inicio[i].tv_sec + (fin[0].tv_sec - inicio[0].tv_sec);
			inicio[i].tv_usec = inicio[i].tv_usec + (fin[0].tv_usec - inicio[0].tv_usec);
		}
		
	}else{
		
		gettimeofday(&fin[tipoLinea], NULL);
		
		resultado = calcularTiempo(&inicio[tipoLinea],&fin[tipoLinea]);
		
		switch(tipoLinea){
			
			case 1:{
				
				if(resultado<mejorTiempo->horizontal)
					mejorTiempo->horizontal = resultado;
					
			}break;
			
			case 2:{
				
				if(resultado<mejorTiempo->vertical)
					mejorTiempo->vertical = resultado;
			
			}break;
			
			case 3:{
				
				if(resultado<mejorTiempo->diagonalP)
					mejorTiempo->diagonalP = resultado;
			
			}break;
			
			case 4:{
				if(resultado<mejorTiempo->diagonalS)
					mejorTiempo->diagonalS = resultado;
			
			}break;
		}
		
		gettimeofday(&inicio[tipoLinea], NULL);
	
	}

}

int procesarLineas(int fila, int columna, Casilla tablero[fila][columna], Nodo** puntos, struct timeval inicio[5], struct timeval fin[5], Records *mejorTiempo, GtkWidget* G_tablero[fila+1][columna+1]){
	
	// 0=NohayLineashechas    1=Horizontal   2=Vertical  3="DiagonalPrincipal"  4="DiagonalSecundaria"
	
	int i=0,j=0,fi=-1,ci=-1,tipoDeLinea=0;
	
	//Verificar lineas horizontales
	for(i=0;(i<fila)&&(tipoDeLinea==0);i++){
		for(j=0;(j<columna-3)&&(tipoDeLinea==0);j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i][j+1].color)&&(tablero[i][j].color==tablero[i][j+2].color)&&(tablero[i][j].color==tablero[i][j+3].color)){
				tipoDeLinea=1;
				fi=i;
				ci=j;
				
				gtk_widget_activate(G_tablero[fi+1][ci+1]);
				gtk_widget_activate(G_tablero[fi+1][ci+2]);
				gtk_widget_activate(G_tablero[fi+1][ci+3]);
				gtk_widget_activate(G_tablero[fi+1][ci+4]);
			}
		}	
	}
	
	//Verificar lineas verticales
	for(i=0;(i<fila-3)&&(tipoDeLinea==0);i++){
		for(j=0;(j<columna)&&(tipoDeLinea==0);j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j].color)&&(tablero[i][j].color==tablero[i+2][j].color)&&(tablero[i][j].color==tablero[i+3][j].color)){
				tipoDeLinea=2;
				fi=i;
				ci=j;
				
				gtk_widget_activate(G_tablero[fi+1][ci+1]);
				gtk_widget_activate(G_tablero[fi+2][ci+1]);
				gtk_widget_activate(G_tablero[fi+3][ci+1]);
				gtk_widget_activate(G_tablero[fi+4][ci+1]);
			}
		}	
	}
	
	//Verificar lineas en "diagonal principal"
	for(i=0;(i<fila-3)&&(tipoDeLinea==0);i++){
		for(j=0;(j<columna-3)&&(tipoDeLinea==0);j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j+1].color)&&(tablero[i][j].color==tablero[i+2][j+2].color)&&(tablero[i][j].color==tablero[i+3][j+3].color)){
				tipoDeLinea=3;
				fi=i;
				ci=j;
				
				gtk_widget_activate(G_tablero[fi+1][ci+1]);
				gtk_widget_activate(G_tablero[fi+2][ci+2]);
				gtk_widget_activate(G_tablero[fi+3][ci+3]);
				gtk_widget_activate(G_tablero[fi+4][ci+4]);
			}
		}	
	}
	
	//Verificar lineas en "diagonal secundaria"
	for(i=0;(i<fila-3)&&(tipoDeLinea==0);i++){
		for(j=3;(j<columna)&&(tipoDeLinea==0);j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j-1].color)&&(tablero[i][j].color==tablero[i+2][j-2].color)&&(tablero[i][j].color==tablero[i+3][j-3].color)){
				tipoDeLinea=4;
				fi=i;
				ci=j;
				
				gtk_widget_activate(G_tablero[fi+1][ci+1]);
				gtk_widget_activate(G_tablero[fi+2][ci]);
				gtk_widget_activate(G_tablero[fi+3][ci-1]);
				gtk_widget_activate(G_tablero[fi+4][ci-2]);
			}
		}	
	}
	
	
	if(tipoDeLinea){
		//printf("\nse encontro una linea de tipo %d q comienza en al fila %d y columna %d\n",tipoDeLinea,fi,ci);
		//sleep(1);
		actualizarTiempo(tipoDeLinea,inicio,fin,mejorTiempo);
		barridoDeBolas(fila,columna,tablero,tipoDeLinea,fi,ci,puntos);
		//visualizarTablero(fila,columna,tablero);
	}

	return tipoDeLinea;
}



bool tableroLleno(int fila, int columna, Casilla tablero[fila][columna]){
	
	int i,j;
	
	for(i=0;i<fila;i++)
		for(j=0;j<columna;j++)
			if(tablero[i][j].estaOcupada==false)
				return false;
	
	return true;
}

void pausar(struct timeval inicio[5], struct timeval fin[5], Records *mejorTiempo){
	
	gettimeofday(&inicio[0], NULL);
	pause();
	gettimeofday(&fin[0], NULL);
	
	actualizarTiempo(0,inicio,fin,mejorTiempo);
	
	//printf("\n\n se quito el pause\n\n");
	//sleep(2);

}

int inicializarTiempo(struct timeval inicio[5], struct timeval fin[5], Records *mejorTiempo){
	
	int i;
	
	for(i=0;i<5;i++){
		gettimeofday(&inicio[i], NULL);
		fin[i]=inicio[i];
	}
	
	mejorTiempo->horizontal= 99999;
	mejorTiempo->vertical  = 99999;
	mejorTiempo->diagonalP = 99999;
	mejorTiempo->diagonalS = 99999;
	
	return 1;
} 

void reiniciar(int fila, int columna, Casilla tablero[fila][columna],Nodo **puntos, struct timeval inicio[5], struct timeval fin[5], Records *mejorTiempo){
	
	int i,j;
	
	for(i=0;i<fila;i++)
		for(j=0;j<columna;j++)
			if(tablero[i][j].estaOcupada){
				kill(tablero[i][j].pidBola,SIGKILL);
				tablero[i][j].pidBola = -1;
				tablero[i][j].estaOcupada = false;
				tablero[i][j].color = ' ';
			}
		
	liberarPuntos(puntos);
	inicializarPuntos(puntos);
	inicializarTiempo(inicio,fin,mejorTiempo);

}

void liberarMarcosYBolas(int fila, int columna, Casilla tablero[fila][columna]){
	
	int i,j;
	
	for(i=0;i<fila;i++){
		for(j=0;j<columna;j++){
			
			kill(tablero[i][j].pidMarco,SIGKILL);
			if(tablero[i][j].estaOcupada)
				kill(tablero[i][j].pidBola,SIGKILL);
		}
	}
}

void mostrarGanadorYRecords(Nodo *puntos, Records *mejorTiempo){
	
	if(puntos->puntosActualesJ1>puntos->puntosActualesJ2)
		g_print("\nHan ganado las fichas Verdes %d a %d\n",puntos->puntosActualesJ1,puntos->puntosActualesJ2);
	else
		if(puntos->puntosActualesJ1<puntos->puntosActualesJ2)
			g_print("\nHan ganado las fichas Azules %d a %d\n",puntos->puntosActualesJ2,puntos->puntosActualesJ1);
		else
			g_print("\nEs un empate a %d\n",puntos->puntosActualesJ1);
	
	g_print("\nEl menor tiempo en formar una Linea Horizontal ha sido %.5g seg\n",mejorTiempo->horizontal);
	g_print("\nEl menor tiempo en formar una Linea Vertical ha sido %.5g seg\n",mejorTiempo->vertical);
	g_print("\nEl menor tiempo en formar una Linea Barra Invertida ha sido %.5g seg\n",mejorTiempo->diagonalP);
	g_print("\nEl menor tiempo en formar una Linea Slash ha sido %.5g seg\n",mejorTiempo->diagonalS);
	
}


void conectarPunteros(Punteros *misPunteros, int *fila, int *columna, int *turno, int *modo, int *colorJ1, Casilla tablero[*fila][*columna], key_t *claveMemoria, int *idMemoria, Casilla *buffer, Records *mejorTiempo, struct timeval inicio[5], struct timeval fin[5], Nodo* puntos, GtkWidget* ventana, GtkWidget* contenedor, GtkWidget *G_tablero[*fila][*columna+1], pthread_t *hilo,bool *finalizarHilo, int *dificultad){
	
	misPunteros->fila = fila;
	misPunteros->columna = columna;
	misPunteros->turno = turno;
	misPunteros->modo = modo;
	misPunteros->colorJ1 = colorJ1;
	misPunteros->tablero = tablero;
	misPunteros->claveMemoria=claveMemoria;
	misPunteros->idMemoria=idMemoria;
	misPunteros->buffer=buffer;
	misPunteros->mejorTiempo = mejorTiempo;
	misPunteros->inicio = inicio;
	misPunteros->fin = fin;
	misPunteros->puntos = puntos;
	misPunteros->ventana = ventana;
	misPunteros->contenedor = contenedor;
	misPunteros->G_tablero = G_tablero;
	misPunteros->hilo = hilo;
	misPunteros->finalizarHilo = finalizarHilo;
	misPunteros->dificultad = dificultad;
	
}

void G_salir(int fila, int columna, Casilla tablero[fila][columna], Nodo* puntos){
	

	liberarPuntos(&puntos);
	
	int i,j;
	for(i=0;i<fila;i++)
		for(j=0;j<columna;j++){
			kill(tablero[i][j].pidMarco,SIGKILL);
			if(tablero[i][j].estaOcupada)
				kill(tablero[i][j].pidBola,SIGKILL);
		}
	
	
}

bool manejarSalida(GtkWidget *widget, Punteros* misPunteros){
	
	G_salir(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero,misPunteros->puntos);
	
	exit(1);
	return 0;
}

bool manejarSalidaConfig(GtkWidget *widget, gpointer data){

	exit(1);
	return 0;
}


char* itoa(int val, int base){
	
	static char buf[32] = {0};
	
	int i = 30;
	
	for(; val && i ; --i, val /= base)
	
		buf[i] = "0123456789abcdef"[val % base];
	
	return &buf[i+1];
	
}


void G_actualizarTurno(int fila, int columna, GtkWidget* G_tablero[fila+1][columna+1], int *turno){
	
	if(*turno==0){
			*turno=1;
			gtk_label_set_label(GTK_LABEL(G_tablero[0][0]),"Turno: Azules");
		}
		else{
			*turno=0;
			gtk_label_set_label(GTK_LABEL(G_tablero[0][0]),"Turno: Verdes");
		}
	
}



void G_inicializarTablero(int fila, int columna, GtkWidget* G_tablero[fila+1][columna+1], int modo, int *turno, bool *finalizarHilo){
	
	int i,j;
	GdkColor color;
    gdk_color_parse ("white", &color);
    
	
	for(i=1;i<fila+1;i++)
		for(j=1;j<columna+1;j++){
			G_tablero[i][j] = gtk_button_new_with_label(" ");
			gtk_widget_modify_bg (GTK_WIDGET(G_tablero[i][j]), GTK_STATE_NORMAL, &color);
				
		}
	
	for(i=7;i<fila+1;i++)
		G_tablero[i][0] = gtk_label_new(" ");
			
	G_tablero[0][0] = gtk_label_new("Turno");
    G_tablero[1][0] = gtk_label_new("Verdes = 0");
    G_tablero[2][0] = gtk_label_new("Azules = 0");
    G_tablero[3][0] = gtk_label_new("Modo:");
    G_tablero[4][0] = gtk_label_new("Mjs en Terminal");
    G_tablero[5][0] = gtk_button_new_with_label("Pause");
    G_tablero[6][0] = gtk_button_new_with_label("Reiniciar");
    
    char nombre[8];
	char *nom1 = "Tirar";
	char *nom2 = itoa(0,10);
    
    for(i=1;i<columna+1;i++){
		
		nom2 = itoa(i,10);
		strncpy(nombre, nom1, 6);
		strncat(nombre, nom2, 7 - strlen(nombre));
		
		if(i>9)
			nombre[7]='\0';
		else
			nombre[6]='\0';
		
		G_tablero[0][i] = gtk_button_new_with_label(nombre);
    }
    
    switch(modo){
		case 0:{
			G_tablero[3][0] = gtk_label_new("Modo: J1vsPC");
		}break;
		
		case 1:{
			G_tablero[3][0] = gtk_label_new("Modo: J1vsJ2");
		}break;
		
		case 2:{
			G_tablero[3][0] = gtk_label_new("Modo: PCvsPC");
		}break;
	}
	
	*finalizarHilo=false;
	*turno=rand()%2;
	G_actualizarTurno(fila,columna,G_tablero,turno);
	
	
}

void manejarPause(GtkWidget* btnPausar, Punteros *misPunteros){
	
	g_print("\nPara quitar el PAUSE pulse ctrl+c aqui\n");
	pausar(misPunteros->inicio,misPunteros->fin,misPunteros->mejorTiempo);
	
}

int columnaATKoDEF(int fila, int columna, Casilla tablero[fila][columna]){
	
	int i=0,j=0,resultado;
	
	//Verificar lineas horizontales
	for(i=0;i<fila;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i][j+1].color)&&(tablero[i][j].color==tablero[i][j+2].color)&&(tablero[i][j+3].color==' ')){
				if(i==fila-1){
					resultado = j+3;
					return resultado;
				}else{
					if(tablero[i+1][j+3].estaOcupada){
						resultado = j+3;
						return resultado;
					}
				}
			}
		}	
	}
	
	for(i=0;i<fila;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i][j+1].color)&&(tablero[i][j+2].color==' ')&&(tablero[i][j+3].color==tablero[i][j].color)){
				if(i==fila-1){
					resultado = j+2;
					return resultado;
				}else{
					if(tablero[i+1][j+2].estaOcupada){
						resultado = j+2;
						return resultado;
					}
				}
			}
		}	
	}
	
	for(i=0;i<fila;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j+1].color==' ')&&(tablero[i][j].color==tablero[i][j+2].color)&&(tablero[i][j+3].color==tablero[i][j].color)){
				if(i==fila-1){
					resultado = j+1;
					return resultado;
				}else{
					if(tablero[i+1][j+1].estaOcupada){
						resultado = j+1;
						return resultado;
					}
				}
			}
		}	
	}
	
	for(i=0;i<fila;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j+1].color!=' ')&&(tablero[i][j].color==' ')&&(tablero[i][j+1].color==tablero[i][j+2].color)&&(tablero[i][j+3].color==tablero[i][j+1].color)){
				if(i==fila-1){
					resultado = j;
					return resultado;
				}else{
					if(tablero[i+1][j].estaOcupada){
						resultado = j;
						return resultado;
					}
				}
			}
		}	
	}
	
	
	
	//Verificar lineas verticales
	for(i=0;i<fila-3;i++){
		for(j=0;j<columna;j++){
			if((tablero[i+1][j].color!=' ')&&(tablero[i][j].color==' ')&&(tablero[i+1][j].color==tablero[i+2][j].color)&&(tablero[i+1][j].color==tablero[i+3][j].color)){
				resultado = j;
				return resultado;
			}
		}	
	}
	
	
	//Verificar lineas en "diagonal principal"
	for(i=0;i<fila-3;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j+1].color)&&(tablero[i][j].color==tablero[i+2][j+2].color)&&(tablero[i+3][j+3].color==' ')){
				if(i==fila-4){
					resultado = j+3;
					return resultado;
				}else{
					if(tablero[i+4][j+3].estaOcupada){
						resultado = j+3;
						return resultado;
					}
				}
			}
		}	
	}
	
	for(i=0;i<fila-3;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j+1].color)&&(tablero[i+2][j+2].color==' ')&&(tablero[i+3][j+3].color==tablero[i][j].color)){
				if(tablero[i+3][j+2].estaOcupada){
					resultado = j+2;
					return resultado;
				}
			}
		}	
	}
	
	for(i=0;i<fila-3;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i+1][j+1].color==' ')&&(tablero[i+2][j+2].color==tablero[i][j].color)&&(tablero[i+3][j+3].color==tablero[i][j].color)){
				if(tablero[i+2][j+1].estaOcupada){
					resultado = j+1;
					return resultado;
				}
			}
		}	
	}
	
	for(i=0;i<fila-3;i++){
		for(j=0;j<columna-3;j++){
			if((tablero[i+1][j+1].color!=' ')&&(tablero[i][j].color==' ')&&(tablero[i+2][j+2].color==tablero[i+1][j+1].color)&&(tablero[i+3][j+3].color==tablero[i+1][j+1].color)){
				if(tablero[i+1][j].estaOcupada){
					resultado = j;
					return resultado;
				}
			}
		}	
	}
	
	
	
	//Verificar lineas en "diagonal secundaria"
	for(i=0;i<fila-3;i++){
		for(j=3;j<columna;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j-1].color)&&(tablero[i][j].color==tablero[i+2][j-2].color)&&(tablero[i+3][j-3].color==' ')){
				if(i==fila-4){
					resultado = j-3;
					return resultado;
				}else{
					if(tablero[i+4][j-3].estaOcupada){
						resultado = j-3;
						return resultado;
					}
				}
			}
		}	
	}
	
	for(i=0;i<fila-3;i++){
		for(j=3;j<columna;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i][j].color==tablero[i+1][j-1].color)&&(tablero[i+2][j-2].color==' ')&&(tablero[i+3][j-3].color==tablero[i][j].color)){
				if(tablero[i+3][j-2].estaOcupada){
					resultado = j-2;
					return resultado;
				}
			}
		}	
	}
	
	for(i=0;i<fila-3;i++){
		for(j=3;j<columna;j++){
			if((tablero[i][j].color!=' ')&&(tablero[i+1][j-1].color==' ')&&(tablero[i+2][j-2].color==tablero[i][j].color)&&(tablero[i+3][j-3].color==tablero[i][j].color)){
				if(tablero[i+2][j-1].estaOcupada){
					resultado = j-1;
					return resultado;
				}
			}
		}	
	}
	
	for(i=0;i<fila-3;i++){
		for(j=3;j<columna;j++){
			if((tablero[i+1][j-1].color!=' ')&&(tablero[i][j].color==' ')&&(tablero[i+2][j-2].color==tablero[i+1][j-1].color)&&(tablero[i+3][j-3].color==tablero[i+1][j-1].color)){
				if(tablero[i+1][j].estaOcupada){
					resultado = j;
					return resultado;
				}
			}
		}	
	}
	
	
    resultado = rand()%columna;
	return resultado;
}

void G_juegaPC(int fila, int columna, GtkWidget* G_tablero[fila+1][columna+1],Casilla tablero[fila][columna], int dificultad){
	
	int i;
	
	if(dificultad==0)
		i= rand()%columna;
	else
		i = columnaATKoDEF(fila,columna,tablero);
	
	gtk_widget_activate(G_tablero[0][i+1]);
		
}

void G_PCvsPC(Punteros *misPunteros){
	

	while(!tableroLleno(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero) && !*misPunteros->finalizarHilo){
		sleep(1);
		G_juegaPC(*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,misPunteros->tablero,*misPunteros->dificultad);
	}
	
	*misPunteros->finalizarHilo = false;
}

void G_reiniciar(int fila, int columna, GtkWidget* G_tablero[fila+1][columna+1],int *turno, int modo, int colorJ1, pthread_t *hilo, bool *finalizarHilo, Punteros *misPunteros){
	
	*finalizarHilo = true;
	int i,j;
	GdkColor color;
    gdk_color_parse ("white", &color);
	
	for(i=1;i<fila+1;i++)
		for(j=1;j<columna+1;j++){
			gtk_widget_modify_bg (GTK_WIDGET(G_tablero[i][j]), GTK_STATE_NORMAL, &color);
			
		}
    
    gtk_label_set_label(GTK_LABEL(G_tablero[1][0]),"Verdes = 0");
    gtk_label_set_label(GTK_LABEL(G_tablero[2][0]),"Azules = 0");
    gtk_label_set_label(GTK_LABEL(G_tablero[4][0]),"Mjs en Terminal");
    
    *turno = rand()%2;
    G_actualizarTurno(fila,columna,G_tablero,turno);
    
    if((modo==0) &&(colorJ1!=*turno)) 
		G_juegaPC(fila,columna,G_tablero,misPunteros->tablero,*misPunteros->dificultad);
		
	//*finalizarHilo = false;	
	if(modo==2)
		pthread_create (hilo, NULL, (void*)G_PCvsPC, (void*)misPunteros);
}

void manejarReinicio(GtkWidget* btnPausar, Punteros *misPunteros){
	
	reiniciar(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero,&misPunteros->puntos,misPunteros->inicio,misPunteros->fin,misPunteros->mejorTiempo);
	G_reiniciar(*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,misPunteros->turno,*misPunteros->modo,*misPunteros->colorJ1, misPunteros->hilo, misPunteros->finalizarHilo, misPunteros);
}

void G_inicialiarVentana(GtkWidget* ventana, GtkWidget* contenedor, int fila, int columna, GtkWidget* G_tablero[fila+1][columna+1], Punteros *misPunteros){
	
	int i,j;	
	
	ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(ventana,TAMPANTALLAX,TAMPANTALLAY);
	gtk_window_set_resizable(GTK_WINDOW(ventana),false);
	gtk_window_set_position(GTK_WINDOW(ventana),3);
	gtk_window_set_title(GTK_WINDOW(ventana), "JUEGO Cuatro P en Linea Carrasco_Sanchez");
	gtk_container_set_border_width(GTK_CONTAINER(ventana), 5);
	
	contenedor = gtk_table_new(fila+1,columna+1,false);
	gtk_table_set_row_spacings(GTK_TABLE(contenedor), 1);
    gtk_table_set_col_spacings(GTK_TABLE(contenedor), 1);
    gtk_container_add(GTK_CONTAINER(ventana), contenedor);
    
    for(i=0;i<fila+1;i++)
		for(j=0;j<columna+1;j++)
			gtk_table_attach_defaults(GTK_TABLE(contenedor), G_tablero[i][j], j,j+1,i,i+1);
	
	if(*misPunteros->modo!=2)
	g_signal_connect(G_tablero[5][0],"clicked", G_CALLBACK(manejarPause),misPunteros);
	g_signal_connect(G_tablero[6][0],"clicked", G_CALLBACK(manejarReinicio),misPunteros);
	g_signal_connect(ventana, "delete-event", G_CALLBACK(gtk_main_quit),NULL);
	
	gtk_widget_show_all(ventana);
	
	
}

void G_cargarValoresPredeterminados(Punteros *misPunteros){
	
	inicializarPuntos(&misPunteros->puntos);
	inicializarTablero(*misPunteros->fila,*misPunteros->columna, misPunteros->tablero);
	inicializarMemoriaCompartida(misPunteros->claveMemoria,misPunteros->idMemoria,&misPunteros->buffer);
	
	G_inicializarTablero(*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,*misPunteros->modo,misPunteros->turno,misPunteros->finalizarHilo);
	G_inicialiarVentana(misPunteros->ventana,misPunteros->contenedor,*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,misPunteros);
	inicializarTiempo(misPunteros->inicio,misPunteros->fin,misPunteros->mejorTiempo);
}


bool G_configuracionInicial(int argc, char **argv, int* fila, int* columna, int*modo, int* colorJ1, int* dificultad){
	
	GtkWidget* ventana;
	GtkWidget* contenedor;
	GtkWidget* lblNroFilas;
	GtkWidget* lblModo;
	GtkWidget* lblColorJ1;
	GtkWidget* lblDificultad;
	GtkWidget* cboNroFilas;
	GtkWidget* cboModo;
	GtkWidget* cboColorJ1;
	GtkWidget* cboDificultad;
	GtkWidget* btnSalir;
	GtkWidget* btnAceptar;
	
	ventana = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_widget_set_size_request(ventana,350,250);
	gtk_window_set_resizable(GTK_WINDOW(ventana),false);
	gtk_window_set_position(GTK_WINDOW(ventana),3);
	gtk_window_set_title(GTK_WINDOW(ventana), "Configuración inicial");
	gtk_container_set_border_width(GTK_CONTAINER(ventana), 5);
	
	contenedor = gtk_table_new(5,2,false);
	
	gtk_table_set_row_spacings(GTK_TABLE(contenedor), 2);
    gtk_table_set_col_spacings(GTK_TABLE(contenedor), 2);
    gtk_container_add(GTK_CONTAINER(ventana), contenedor);
    
    lblNroFilas = gtk_label_new("Nro de Filas");
    gtk_table_attach_defaults(GTK_TABLE(contenedor), lblNroFilas, 0,1,0,1);
    
    cboNroFilas = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboNroFilas), "6");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboNroFilas), "8");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboNroFilas), "10");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboNroFilas), "12");
    gtk_table_attach_defaults(GTK_TABLE(contenedor), cboNroFilas, 1,2,0,1);
    if(!establecerDimensiones(argc,argv,fila,columna,cboNroFilas))
		return false;
    
    lblModo = gtk_label_new("Modo");
    gtk_table_attach_defaults(GTK_TABLE(contenedor), lblModo, 0,1,1,2);
    
    cboModo = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboModo), "J1 vs PC");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboModo), "J1 vs J2");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboModo), "PC vs PC");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cboModo), 0);
    gtk_table_attach_defaults(GTK_TABLE(contenedor), cboModo, 1,2,1,2);
    
    lblColorJ1 = gtk_label_new("Color J1");
    gtk_table_attach_defaults(GTK_TABLE(contenedor), lblColorJ1, 0,1,2,3);
    
    cboColorJ1 = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboColorJ1), "Verdes");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboColorJ1), "Azules");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cboColorJ1), 0);
    gtk_table_attach_defaults(GTK_TABLE(contenedor), cboColorJ1, 1,2,2,3);
    
    lblDificultad = gtk_label_new("Dificultad");
    gtk_table_attach_defaults(GTK_TABLE(contenedor), lblDificultad, 0,1,3,4);
    
    cboDificultad = gtk_combo_box_new_text();
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboDificultad), "Principiante");
    gtk_combo_box_append_text(GTK_COMBO_BOX(cboDificultad), "Dificil");
    gtk_combo_box_set_active(GTK_COMBO_BOX(cboDificultad), 0);
    gtk_table_attach_defaults(GTK_TABLE(contenedor), cboDificultad, 1,2,3,4);
    

	btnSalir = gtk_button_new_with_label("Salir");
	gtk_table_attach_defaults(GTK_TABLE(contenedor), btnSalir, 0,1,4,5);

	btnAceptar = gtk_button_new_with_label("Aceptar");
	gtk_table_attach_defaults(GTK_TABLE(contenedor), btnAceptar, 1,2,4,5);
	
	g_signal_connect (btnSalir, "clicked",G_CALLBACK (manejarSalidaConfig), NULL);
	g_signal_connect (btnAceptar, "clicked",G_CALLBACK (gtk_main_quit), NULL);
	g_signal_connect(ventana, "delete-event", G_CALLBACK(manejarSalidaConfig),NULL);
	
	gtk_widget_show_all(ventana);
	
	gtk_main();
	
	*fila = gtk_combo_box_get_active(GTK_COMBO_BOX(cboNroFilas))*2+6;
	*columna = *fila+1;
	*modo = gtk_combo_box_get_active(GTK_COMBO_BOX(cboModo));
	*colorJ1 = gtk_combo_box_get_active(GTK_COMBO_BOX(cboColorJ1));
	*dificultad = gtk_combo_box_get_active(GTK_COMBO_BOX(cboDificultad));
	gtk_widget_destroy(ventana);
	usleep(500);
	 
	return true;
}

void G_ponerBola(int fila, int columna, GtkWidget* G_tablero[fila+1][columna+1],int filaAJugar, int columnaAJugar, int turno, GtkWidget* ventana){

	GdkColor color;
	
	if(turno==0)
		gdk_color_parse ("green", &color);
	else
		gdk_color_parse ("blue", &color);
	
	gtk_widget_modify_bg (GTK_WIDGET(G_tablero[filaAJugar+1][columnaAJugar+1]), GTK_STATE_NORMAL, &color);
	
}

void G_actualizarTablero(int fila, int columna, Casilla tablero[fila][columna], GtkWidget* G_tablero[fila+1][columna+1], Nodo* puntos, int *turno,int filaAJugar, int columnaAJugar){
	
	int i,j;
	GdkColor color;
	char nombre1[12];
	char nombre2[12];
	char *nom1 = "Color";
	char *nom2 = itoa(0,10);
    
	for(i=0;i<fila;i++)
		for(j=0;j<columna;j++){
			
			if(tablero[i][j].color==' '){
				gdk_color_parse ("white", &color);
				gtk_widget_modify_bg (GTK_WIDGET(G_tablero[i+1][j+1]), GTK_STATE_NORMAL, &color);
			}
			
			if(tablero[i][j].color=='x'){
				gdk_color_parse ("green", &color);
				gtk_widget_modify_bg (GTK_WIDGET(G_tablero[i+1][j+1]), GTK_STATE_NORMAL, &color);
			}
			
			if(tablero[i][j].color=='o'){
				gdk_color_parse ("blue", &color);
				gtk_widget_modify_bg (GTK_WIDGET(G_tablero[i+1][j+1]), GTK_STATE_NORMAL, &color);
			}	
		}
	
	//g_print("\nverdes=%d  azules=%d\n",puntos->puntosActualesJ1,puntos->puntosActualesJ2);
	
	nom1 = "Verdes = ";
	if(puntos->puntosActualesJ1==0)
		nom2="O";
	else
		nom2 = itoa(puntos->puntosActualesJ1,10);
	strncpy(nombre1, nom1, 9);
	strncat(nombre1, nom2, strlen(nom2));
	gtk_label_set_label(GTK_LABEL(G_tablero[1][0]),nombre1);
	
	nom1 = "Azules = ";
	if(puntos->puntosActualesJ2==0)
		nom2="O";
	else
		nom2 = itoa(puntos->puntosActualesJ2,10);
	strncpy(nombre2, nom1, 9);
	strncat(nombre2, nom2, strlen(nom2));
    gtk_label_set_label(GTK_LABEL(G_tablero[2][0]),nombre2);
	
}

int manejarJugada(GtkWidget* btnPulsado, Punteros *misPunteros){
	
	int columnaAJugar, filaAJugar;
	const char *nombreDeBoton = gtk_button_get_label(GTK_BUTTON(btnPulsado));
	char color;
	
	if(*misPunteros->turno==0)
		color = 'x';
	else
		color = 'o';
	
	columnaAJugar = atoi(nombreDeBoton+5)-1;
	filaAJugar = filaMasBajaLibre(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero,columnaAJugar);
	
	if(filaAJugar!=-1){	
		crearBola(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero,filaAJugar,columnaAJugar,misPunteros->buffer,color);
		G_ponerBola(*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,filaAJugar,columnaAJugar,*misPunteros->turno,misPunteros->ventana);
		G_actualizarTurno(*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,misPunteros->turno);
		
		while(procesarLineas(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero,&misPunteros->puntos,misPunteros->inicio,misPunteros->fin,misPunteros->mejorTiempo,misPunteros->G_tablero)){
			G_actualizarTablero(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero,misPunteros->G_tablero,misPunteros->puntos,misPunteros->turno,filaAJugar,columnaAJugar);
		}
		
		if(tableroLleno(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero))
			mostrarGanadorYRecords(misPunteros->puntos,misPunteros->mejorTiempo);
	}
	
	if(((*misPunteros->modo==0) &&(*misPunteros->colorJ1!=*misPunteros->turno)) && !tableroLleno(*misPunteros->fila,*misPunteros->columna,misPunteros->tablero)) 
		G_juegaPC(*misPunteros->fila,*misPunteros->columna,misPunteros->G_tablero,misPunteros->tablero,*misPunteros->dificultad);
		
	return 1;
}

void G_conectarBotonesDeJuego(int fila, int columna, GtkWidget *G_tablero[fila+1][columna+1], Punteros *misPunteros){
	
	int i;
	
	for(i=1;i<columna+1;i++)
		g_signal_connect (G_tablero[0][i], "clicked",G_CALLBACK (manejarJugada),misPunteros);
	
}



int main(int argc, char **argv){
	 
	signal(SIGCONT,manejador);
	signal(SIGKILL,manejador);
	signal(SIGINT,manejador);

	srand(time(NULL));	
	
	Punteros misPunteros;
	int fila,columna,turno,modo,colorJ1;
	key_t claveMemoria;
	int idMemoria;
	Casilla *buffer=NULL;
	Records mejorTiempo;
	struct timeval inicio[5],fin[5];
	Nodo *puntos=NULL;
	GtkWidget* ventana=NULL;
	GtkWidget* contenedor=NULL;
	pthread_t hilo;
	bool finalizarHilo;
	int dificultad;
  
    gtk_init(&argc, &argv);
    
	if(!G_configuracionInicial(argc,argv,&fila,&columna,&modo,&colorJ1,&dificultad))
		return 0;
		
	Casilla tablero[fila][columna];
	GtkWidget*  G_tablero[fila+1][columna+1];
	
	conectarPunteros(&misPunteros,&fila,&columna,&turno,&modo,&colorJ1,tablero,&claveMemoria,&idMemoria,buffer,&mejorTiempo,inicio,fin,puntos,ventana,contenedor,G_tablero,&hilo,&finalizarHilo,&dificultad);

	G_cargarValoresPredeterminados(&misPunteros);
	
	G_conectarBotonesDeJuego(fila,columna,G_tablero,&misPunteros);
	
	if((modo==0) &&(colorJ1!=turno)) 
		G_juegaPC(fila,columna,G_tablero,tablero,dificultad);
	
	if(modo==2)
		pthread_create (&hilo, NULL, (void*)G_PCvsPC, (void*)&misPunteros);
			
	gtk_main();
	
    liberarMarcosYBolas(fila,columna,tablero);
	liberarBuffer(idMemoria,buffer);
	liberarPuntos(&puntos);
	
	return 0;
}
