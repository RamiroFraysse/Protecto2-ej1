#include <stdio.h>

#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "comunicacion.h"
#define BACKLOG 100


char buffer[MAXDATASIZE];
int nro_cliente = 0;
int *sockets_clientes;
struct Punto* last_color;
double color_bg_red,color_bg_green,color_bg_blue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void procesar_mensaje_recibido(struct Punto *last,int id){
    last->id=id;
    char value[8];
    memset(value,'\0',1);
    char men[1024];
    strcpy(men,buffer);
    last->tipo=men[0];
    
    int i=2; //saltea tipo|
    while(men[i]!='|'){
        value[i-2]=men[i];
        i++;
    }
    sscanf(value,"%lf",&last->red);
    i++; //saltea tipo|red|
    int j=0;
    while(men[i]!='|'){
        value[j]=men[i];
        i++;
        j++;
    }
    sscanf(value,"%lf",&last->green);
    i++; //saltea tipo|red|green|
    j=0;
    while(men[i]!='\0'){
        value[j]=men[i];
        i++;
        j++;
    }
    sscanf(value,"%lf",&last->blue);
    //mensaje = tipo|red|green|blue|id
}

void send_new_bg(struct Punto *last,int id){
	printf("id es %d \n",id);
	char mensaje[MAXDATASIZE];
	memset(mensaje,'\0',1);
	char tipo[1],ared[8],ablue[8],agreen[8],cid[1];
	memset(ared,'\0',1);
	memset(ablue,'\0',1);
	memset(agreen,'\0',1);
	tipo[0]= NEWCOLORBG;
	strcpy(mensaje,tipo);
	strcat(mensaje,"|");
	sprintf(ared,"%lf",last->red);
	strcat(mensaje,ared);
	strcat(mensaje,"|");
	sprintf(agreen,"%lf",last->green);
	strcat(mensaje,agreen);
	strcat(mensaje,"|");
	sprintf(ablue,"%lf",last->blue);
	strcat(mensaje,ablue);
	strcat(mensaje,"|");
	sprintf(cid,"%d",last->id);
	strcat(mensaje,cid);
	printf("el mensaje a enviar es %s hecho por %d o %d\n",mensaje,last->id,id);
	int i,prueba;
		for(i=0; i<max_clientes; i++){
		    
		    if(sockets_clientes[i]!=-1 && i!=id)
		    {
			printf("ENVIANDO NUEVO FONDO %s al cliente %d con un tamano %d\n",mensaje,i,strlen(mensaje));
			prueba = send(sockets_clientes[i], mensaje, strlen(mensaje), 0);
			if ( prueba== -1)
			    perror("send");
			else
			    printf("el servidor envio %d bytes \n",prueba);
			    
		    }
		}
	
}

void funcion_hilo_point(int id)
{	
	printf("Este es el hilo del cliente %d \n",id);
	struct Punto last;
	int i = 0;
	char tipo;
	int stop = 0;
	char c;
	last_color[id].id = id;
	while(stop == 0){
		printf("stop es %d \n",stop);
		if (recv(sockets_clientes[id], &buffer, sizeof(buffer), 0) == -1){
			printf("FUNCION PUNTO: NO SE RECIBIERON DATOS DEL CLIENTE %d \n", id);
			stop = 1;
		}else //El servidor recibio un cambio.
		{
			fflush(NULL);
			procesar_mensaje_recibido(&last,id);
			printf("El servidor recibio un cambio hecho por el cliente %d: %s \n",id,buffer);
			if(last.tipo == DESCONECTAR){
				stop = 1;
			}
			
			switch(last.tipo){
			    case NEWCOLORPINCEL:
				last.id = id;
				last_color[id].tipo = NEWCOLORPINCEL;
				last_color[id].red = last.red;
				last_color[id].green = last.green;
				last_color[id].blue = last.blue;
				printf("entro al case que corresponde \n");
				//last_color[id].pencil_width = last.pencil_width;
				break;
			    case NEWCOLORBG:
				{
					color_bg_red = last.red;
					color_bg_green = last.green;
					color_bg_blue = last.blue;
					pthread_mutex_lock(&mutex);
					//envio el valor del punto a todos los miembros
					send_new_bg(&last,id);
					pthread_mutex_unlock(&mutex);
			    }
			    default:
					break;
			}
		}
	}
	printf("El cliente %i abandono la sala\n",id);
	close(sockets_clientes[id]);
	sockets_clientes[id] = -1; //marco
	
	//lock
	nro_cliente--;
	//unlock
	pthread_exit(0);

}

int next_cliente()
{
	int i = 0;
	while(sockets_clientes[i]!=-1 && i<max_clientes)
		i++;
	if(i<BACKLOG)
		return i;
	else 
		return -1;
}

int main()
{
	int sockfd, newfd;
	int sin_size;
	int i = 0;
	int status;
	int actual=0;
	struct sockaddr_in clnt_addr;
	struct sockaddr_in svr_addr;
	
	sockets_clientes = (int*) malloc(sizeof(int)*max_clientes);//por ahora maximo 10
	last_color = (struct Punto*) malloc(sizeof(struct Punto)*max_clientes);//por ahora maximo 10
	pthread_mutex_unlock(&mutex);
	
	//cliente Desconectado = -1
	for(i=0;i<max_clientes;i++){
		sockets_clientes[i] = -1;
		bzero(last_color,sizeof(struct Punto)*max_clientes);
	}

	pthread_t* tid = malloc(sizeof(pthread_t)*max_clientes);

	//apertura del socket q retorna un descriptor q el sistema asocia con una conexion en red.
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	printf("sockfd vale despues de crear el socket %d\n",sockfd);
	//define familia
	svr_addr.sin_family = AF_INET;
	//asigna puerto
	svr_addr.sin_port = htons(PORT);
	//asigna la ip local
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	//rellena con ceros
	bzero(&(svr_addr.sin_zero), 8);
	sin_size = sizeof(struct sockaddr_in);

	//avisar al sistema operativo q hemos abierto un socket y qremos q asocie nuestro programa al socket
	if ( bind(sockfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr)) == -1){ 
		perror("bind");
		exit(EXIT_FAILURE);
	}
	//avisa al sistema q empieza a atender conexiones.
	if (listen(sockfd, BACKLOG) != -1) {	
		while(1)
		{ 
			//pide y acepta las conexiones de clientes al sistema operativo
		    if ((newfd = accept(sockfd, (struct sockaddr *)&clnt_addr, &sin_size)) != -1){
				nro_cliente++;
				printf("Cantidad de clientes conectados %d\n",nro_cliente);
				if((actual = next_cliente())>-1){
					sockets_clientes[actual] = newfd;
					char* ip = inet_ntoa_my(clnt_addr.sin_addr);
					printf("Se establecio una conexion desde %s, ID cliente: %i, con identificador socket: %d \n",ip,actual,newfd);
					status = pthread_create(&tid[actual], NULL, funcion_hilo_point, (void*) actual);
					printf("status vale: %d \n",status);
					//en caso de error
					if(status)
					  exit(1);
				}
		    }else{
				close(newfd);
				perror("acept ");
				printf("ERROR AL ACEPTAR LA CONEXION, MAXIMA CANTIDAD DE MIEMBROS\n");
		    }
		}//fin del bucle
	}else{
		perror("Listen");
	}
	return 0;
} 
