#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
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


void send_update(Punto *nuevo,int id){
	int i,prueba;
		for(i=0; i<max_clientes; i++){
		    
		    if(sockets_clientes[i]!=-1 && i!=id)
		    {
			printf("ENVIANDO NUEVO color al cliente %d \n",i);
			prueba = send(sockets_clientes[i], nuevo, sizeof(Punto), 0);
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
	//struct Punto last;
	int i = 0;
	char tipo;
	int stop = 0;
	char c;
	//last_color[id].id = id;
	while(stop == 0){
		Punto nuevo;
		if (recv(sockets_clientes[id], &nuevo, sizeof(Punto), 0) == -1){
			printf("FUNCION PUNTO: NO SE RECIBIERON DATOS DEL CLIENTE %d \n", id);
			stop = 1;
		}else //El servidor recibio un cambio.
		{
			fflush(NULL);
			if(nuevo.tipo == DESCONECTAR){
				stop = 1;
			}
			switch(nuevo.tipo){
				case NEWCOLORPINCEL:
				{
					nuevo.id = id;
					pthread_mutex_lock(&mutex);
					send_update(&nuevo,id);
					pthread_mutex_unlock(&mutex);

				}
			    case NEWCOLORBG:
				{
					nuevo.id = id;
					pthread_mutex_lock(&mutex);
					//envio el valor del punto a todos los miembros
					send_update(&nuevo,id);
					pthread_mutex_unlock(&mutex);
			    }
			    case NEWDRAWPOINT:
				{
					nuevo.id = id;
					pthread_mutex_lock(&mutex);
					//envio el valor del punto a todos los miembros
					send_update(&nuevo,id);
					pthread_mutex_unlock(&mutex);
			    }
			    case CLEAR:
					pthread_mutex_lock(&mutex);
					//envio el valor del punto a todos los miembros
					send_update(&nuevo,id);
					pthread_mutex_unlock(&mutex);
			    default:
					break;
			}
		}
	}
	printf("El cliente %i abandono la pizarra\n",id);
	close(sockets_clientes[id]);
	sockets_clientes[id] = -1; 
	nro_cliente--;
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
