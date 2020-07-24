#include <stdio.h>

#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "comunicacion.h"
#define BACKLOG 10

struct Punto last;

char buffer[MAXDATASIZE];
int nro_cliente = 0;
int* sockets_clientes;
struct Punto* last_color;
double color_bg_red,color_bg_green,color_bg_blue;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void funcion_hilo_point(int id)
{
    printf("Funcion hilo point del hilo %d\n",id);
    printf("Funcion hilo point del socket_clientes[%d] %d\n",id,sockets_clientes[id]);
    
	int i = 0;
	char tipo;
	int stop = 0;
	char c;

	last_color[id].id = id;
/*
    
	for(i=0; i<max_clientes; i++)
	{
				
		if(i!=id && sockets_clientes[i]!=-1)

			if (send(sockets_clientes[id], &last_color[i], sizeof(struct Punto), 0) == -1)
				perror("send");
				
	}*/
    printf("No hace nada en el primer for \n");
    /*
	last.tipo = NEWFONDO;
	last.red = color_bg[0];
	last.green = color_bg[1];
	last.blue = color_bg[2];
    
	for(i=0; i<max_clientes; i++)
	{
		
		if(sockets_clientes[i]!=-1)
		{
			if (send(sockets_clientes[i], &last, sizeof(struct Punto), 0) == -1)
				perror("send");
		}
	}
    */

	while(stop == 0)
	{
			if (recv(sockets_clientes[id], &buffer, sizeof(buffer), 0) == -1)
			{
				printf("FUNCION PUNTO: NO SE RECIBIERON DATOS DEL CLIENTE %i\n", id);
				stop = 1;

			}
			else //El servidor recibio un cambio.
			{
                procesar_mensaje_recibido();
				//fflush(NULL);
                //s0top =1;
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
                        color_bg_red = last.red;
                        color_bg_green = last.green;
                        color_bg_blue = last.blue;
                    default:
                        break;
                }
                pthread_mutex_lock(&mutex);
                //envio el valor del punto a todos los miembros
                printf("No se rompio lock \n");

                char mensaje[MAXDATASIZE];
                strcpy(mensaje,buffer);
                char cid[1];
                sprintf(cid,"%d",last.id);
                strcat(mensaje,"|");
                strcat(mensaje,cid);
                //mensaje = tipo|red|green|blue|id
                int prueba;
                for(i=0; i<max_clientes; i++)
                {
                    
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
                pthread_mutex_unlock(&mutex);
                /*
				if(last.tipo == DESCONECTAR)
				{
					stop = 1;

				}
				else
				{
					if(last.tipo == NEWCOLORPINCEL)
					{
						
					}
					if(last.tipo == NEWFONDO)
					{
						color_bg[0] = last.red;
						color_bg[1] = last.green;
						color_bg[2] = last.blue;

					}
					{ 
						
					}
				}*/
                stop =1;
            }
		
		
	
	}
    /*

	printf("El cliente %i abandono la sala\n",id);
	close(sockets_clientes[id]);
	sockets_clientes[id] = -1; //marco

	//lock
	nro_cliente--;
	//unlock
	pthread_exit(0);
*/
}
/*
void determinar_tipo_mensaje(){
    
    printf("El servidor mira el tipo de mensaje recibido \n");
    char mensaje[strlen(buffer)];
    strcpy(mensaje,buffer);
    switch (mensaje[0])
    {
        case '1':
            cambiar_color_pincel();
            break;
        default:
            break;
    }
}
*/

void procesar_mensaje_recibido(){
    char value[8];
    memset(value,'\0',1);
    char men[1024];
    printf("estoy en procesar_mensaje_recibido \n");
    strcpy(men,buffer);
    last.tipo=men[0];
    printf("el tipo es %c\n",last.tipo);

    int i=2; //saltea tipo|
    while(men[i]!='|'){
        value[i-2]=men[i];
        i++;
    }
    printf("Despues del primer while\n");

    sscanf(value,"%lf",&last.red);
    i++; //saltea tipo|red|
    int j=0;
    while(men[i]!='|'){
        value[j]=men[i];
        i++;
        j++;
    }

    sscanf(value,"%lf",&last.green);
    i++; //saltea tipo|red|green|
    j=0;
    while(men[i]!='\0'){
        value[j]=men[i];
        i++;
        j++;
    }
    sscanf(value,"%lf",&last.blue);
    printf("red %lf \n",last.red);
    printf("blue %lf \n",last.green);
    printf("green %lf \n",last.blue);
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

	int actual;

	struct sockaddr_in clnt_addr;

	struct sockaddr_in svr_addr;
	
	sockets_clientes = (int*) malloc(sizeof(int)*max_clientes);//por ahora maximo 10

	last_color = (struct Punto*) malloc(sizeof(struct Punto)*max_clientes);//por ahora maximo 10

	pthread_mutex_unlock(&mutex);
	for(i=0;i<max_clientes;i++)
	{
		sockets_clientes[i] = -1;
		bzero(last_color,sizeof(struct Punto)*max_clientes);
	}

	pthread_t* tid = malloc(sizeof(pthread_t)*max_clientes);

	printf("antes de crear el socket \n");

	//se crea el socket y se checkea
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	//defino familia
	svr_addr.sin_family = AF_INET;
	//asigno puerto
	svr_addr.sin_port = htons(PORT);
	//se asigna la ip local
	svr_addr.sin_addr.s_addr = INADDR_ANY;
	//rellena con ceros
	bzero(&(svr_addr.sin_zero), 8);


	//se ejecuta y checkea el enlazador (bind)
	if ( bind(sockfd, (struct sockaddr *)&svr_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, BACKLOG) != -1) {	
        printf("listen %d\n",sockfd);
    sin_size = sizeof(struct sockaddr_in);
	//se ejecuta el listen
		while(1)//bucle 
		{ 
            if ((newfd = accept(sockfd, (struct sockaddr *)&clnt_addr, &sin_size)) != -1){
                printf("accept %d\n",newfd);
                if((actual = next_cliente())>-1)
                {
                    printf("acepta la conexion \n");
                    actual = next_cliente();
                    sockets_clientes[actual] = newfd;
                    char* ip = inet_ntoa_my(clnt_addr.sin_addr);
                    printf("Se establecio una coneccion desde %s, ID miembro: %i\n",ip,actual);
                    //envio todos los colores de los miembros actuales
        
                    //se crean el hilo para recibir puntos
                    status = pthread_create(&tid[actual], NULL, funcion_hilo_point, (void*) actual);
                    //en caso de error
                    if(status)
                          exit(-1);
        
        
        
                    //si todo sale bien se incrementa el numero de cliente
                    
        
                }
            }else{
                close(newfd);
                perror("acept ");
                printf("ERROR AL ACEPTAR LA CONECCION, MAXIMA CANTIDAD DE MIEMBROS\n");
            }
		}//fin del bucle
    
	}else
	{
			perror("Listen");
	}

    close(sockfd);
	return 0;
} 
