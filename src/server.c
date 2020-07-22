#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/utsname.h>

#define MYPORT 14550 /*Nro de puerto donde se conectaran los clientes*/
#define BACKLOG 10 /* Tamaño de la cola de conexiones recibidas */

int main() {
    int sockfd,numbytes; /* El servidor escuchara por sockfd */
    int newfd; /* las transferencias de datos se realizar mediante newfd */
    struct sockaddr_in my_addr; /* contendrá la dir IP y el nro de puerto local */
    struct sockaddr_in their_addr; //Contendrá la dir IP y nro de puerto del cliente
    int sin_size; /* Contendra el tamaño de la escructura sockaddr_in */
    char buf[500]; /* Buffer donde se reciben los datos */
    /****se crea el socket*****/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Asigna valores a la estruct my_addr para luego poder llamar a la función bind()
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(MYPORT); /*formato de network byte order htons es para convertirlo para que se pueda transmitir por la red */
    my_addr.sin_addr.s_addr = INADDR_ANY; /* automaticamente usa IP local */
    bzero(&(my_addr.sin_zero), 8); /* rellena con ceros el resto de la estructura */

    /*******BIND*****Asigna un nombre al socket */
    if ( bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /********LISTEN***Habilita el socket para recibir conexiones, con una cola de x conexiones en espera como máximo */
    if (listen(sockfd, BACKLOG) == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /***********ACCEPT**************/
    while(1) /* loop que llama a accept() */
    {
        sin_size = sizeof(struct sockaddr_in);
        /*Se espera por conexiones ,*/
        if ((newfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1)
        {
            perror("accept");
            continue; /* Si se produce un error se finaliza el programa */
        }
        printf("server: conexion desde: %s\n", inet_ntoa(their_addr.sin_addr));
        printf("Desde puerto: %d \n", ntohs(their_addr.sin_port));
        
        if (!fork())
        { /* Comienza el proceso hijo, enviamos los datos mediante newfd */
            /* Recibimos la solicitud del cliente*/
                if((numbytes = recv(newfd,buf,sizeof(buf),0))==-1){
                    perror("recv: ");
                    exit(EXIT_FAILURE);
                }
                printf("recibi %s \n",buf);
                
            if (send(newfd, "hola",4, 0) == -1)
                perror("send");
            close(newfd);
            exit(0);
        }
    }
    close(sockfd);
    return 0;

}
