#include <gtk/gtk.h>
#define _GNU_SOURCE
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>

#define PORT 14550
#define MAXDATASIZE 2048
#define TOPEPRUEBAS 10

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "comunicacion.h"



//Variables globales sockets  
pthread_t t_id; //hilo de ejecucion que escucha al servidor
int conect;
int sockfd, numbytes;
char buf[MAXDATASIZE]; // Buffer donde se reciben los datos
struct hostent *he; // Se utiliza para convertir el nombre del hosta su dirección IP
struct sockaddr_in their_addr; // dirección del server donde se conectara 
char *ip;
//

//Variables globales GUI
GtkWidget  *window;
GtkWidget  *fixed1;
GtkWidget  *draw_area;
GtkWidget  *clear;
GtkWidget  *color_button;
GtkWidget  *btn_spin;
GtkWidget  *btn_conectar;
GtkWidget  *lbl_connect;
GtkWidget  *btn_dark;
GtkWidget  *box;
GtkBuilder *builder; 

Point p1, *p2, *start;
double red, green, blue, alpha;
double red_bg , green_bg, blue_bg;
//

//funciones
void on_destroy(); 
gboolean on_draw_area_draw (GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean on_draw_area_button_release_event (GtkWidget *widget, GdkEventButton *event);
gboolean on_draw_area_button_press_event (GtkWidget *widget, GdkEventButton *event);
gboolean on_draw_area_motion_notify_event (GtkWidget *widget, GdkEventMotion *event, gpointer data);
static void draw_brush (GtkWidget *widget, gdouble x, gdouble y);
void on_color_button_color_set(GtkWidget *c);
void on_btn_dark_toggled(GtkCheckButton *b);
void on_btn_conectar_toggled(GtkToggleButton *toggledButton);
static void draw_brush (GtkWidget *widget, gdouble  x, gdouble  y);
void configure_GUI();
//

int main(int argc, char *argv[]) {
    if(argc!=2){
		printf("usage: ./cliente ip\n");
		exit(1);
	}

    if ((he=gethostbyname(argv[1])) == NULL) { 
        herror("gethostbyname");
        exit(1);
    }
    ip = (char*)malloc (sizeof(char)*15);
    memset(ip,'\0',1);
    ip = argv[1];
    gdk_threads_init ();
    gdk_threads_enter();

    gtk_init (&argc, &argv);
    memset(buf,'\0',MAXDATASIZE);
    configure_GUI();
    
	

    // Le dice al sistema que muestre la ventana q tenemos guardada en el puntero window 
    gtk_widget_show(window);
    
    //espera por senales y eventos y se ejecuta el callback correspondiente.
    gtk_main();
    gdk_threads_leave();

    return EXIT_SUCCESS;
}

void on_destroy() {
    gtk_main_quit();
}
        
/*
 * Esta es la funcion que se llamara cuando el sistema quiera dibujar en el drawing area
*/
gboolean on_draw_area_draw (GtkWidget *widget, cairo_t *cr, gpointer data) {

    guint width, height;

    width = gtk_widget_get_allocated_width (widget);
    height = gtk_widget_get_allocated_height (widget);

    //Establece el ancho de la linea
    cairo_set_line_width(cr, 1.0);

    if (start == NULL) return FALSE;

    int old_x = start->x;
    int old_y = start->y;

    p1 = start->next;

    while ( p1 != NULL ) {
	//obtiene el color a pintar 
	cairo_set_source_rgb(cr, p1->red, p1->green, p1->blue);

	cairo_move_to (cr, (double) old_x, (double) old_y);
	//dibuja una linea hasta este otro punto.
	cairo_line_to (cr, (double) p1->x, (double) p1->y);
	cairo_stroke (cr);

	//actualiza old & p1.
	old_x = p1->x;
	old_y =  p1->y;
	p1 = p1 ->next;
    }

    return FALSE;
}

//se ejecuta ante un evento de release e indica que debe dejar de dibujarse ante un motion event
gboolean on_draw_area_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
    printf("release_event\n");
    return TRUE;
}

//callback para el evento button_press_event 
gboolean on_draw_area_button_press_event (GtkWidget *widget, GdkEventButton *event) {
    printf("on_draw_area_button_press_event \n");

    //le pasa la posicion en donde se hizo click.
    draw_brush (widget, event->x, event->y);
    return TRUE;
}

void on_clear_clicked(GtkWidget *b1) {
    p1 = start;
    //mientras q p1 is not null
    while (p1) { 
	//libera todo los punteros.
	p2 = p1 -> next; 
	free(p1); 
	p1 = p2; 
    }
    start = NULL;
    //limpia el area de dibujo.
    gtk_widget_queue_draw (draw_area);
}

/*
 *  callback llamado por el evento motion_notify_event
 *  Cuando estas presionando el boton del mouse y a la vez te moves.
*/
gboolean on_draw_area_motion_notify_event (GtkWidget *widget, GdkEventMotion *event, gpointer data) {
    printf("on_draw_area_motion_notify_event \n");

    if (event->state & GDK_BUTTON1_MASK ) 
    draw_brush (widget, event->x, event->y);
    return TRUE;
    }

static void draw_brush (GtkWidget *widget, gdouble x, gdouble y) {
    printf("draw_brush \n");

    p1 = malloc (sizeof(struct Point));
    if (p1 == NULL) { printf("out of memory\n"); abort(); }
    p1->x = x;
    p1->y = y;
    p1->red = red;
    p1->green = green;
    p1->blue = blue;
    p1->alpha = alpha;
    p1->next = start;
    start = p1;
    gtk_widget_queue_draw (draw_area);
}


void on_color_button_color_set(GtkWidget *c) { 
    GdkRGBA color; //es un struct
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c),&color);
    red = color.red;  green = color.green; blue = color.blue; alpha = color.alpha;
    send_select_color();
}

void send_select_color(){
	
	
    if(conect == 1)
    {	
	char mensaje[MAXDATASIZE];
	memset(mensaje,'\0',1);
	char ared[8],ablue[8],agreen[8];
	memset(ared,'\0',1);
	memset(ablue,'\0',1);
	memset(agreen,'\0',1);

	sprintf(ared,"%lf",red);
	strcpy(mensaje,ared);
	strcat(mensaje,"|");
	sprintf(agreen,"%lf",green);
	strcat(mensaje,agreen);
	strcat(mensaje,"|");
	sprintf(ablue,"%lf",blue);
	strcat(mensaje,ablue);
	

	if (send(sockfd, mensaje, sizeof(mensaje), 0) == -1){
	    perror("send: ");
	    exit(EXIT_FAILURE);
	}
	fflush(NULL);
    }
}

void on_btn_dark_toggled(GtkCheckButton *b) { 
    gboolean T = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
    GdkColor color_bg;
    if(T){
	
        //dark background color
        red_bg =0.0;
        green_bg=0.0;
	blue_bg = 0.0;
        color_bg.red = red_bg;
        color_bg.green = green_bg;
        color_bg.blue = blue_bg;
        gtk_widget_modify_bg(GTK_WIDGET(window),GTK_STATE_NORMAL,&color_bg);
	//send_background(&color_bg);
    }else{
	printf("entra al else \n");
	red_bg =1.0;
        green_bg=1.0;
	blue_bg = 1.0;
        color_bg.red = red_bg;
        color_bg.green = green_bg;
        color_bg.blue = blue_bg;
        gtk_widget_modify_bg(GTK_WIDGET(window),GTK_STATE_NORMAL,&color_bg);
    }
}

//funcion que ejecuta el hilo que se conecta con el servidor
void listen_server()
{
	
    /* Recibimos los datos del servidor */
    if ((numbytes=recv(sockfd, buf, MAXDATASIZE, 0)) == -1)
    {
	perror("recv");
	exit(1);
    }
    printf("Recibi del servidor %s\n");
    //conectado = 0;
    pthread_exit(0);

}

//envia un evento de movimiento y el punto actual por parte del miembro
/*void enviar_punto()
{
	//printf("ENVIAR %i %i\n",point[0],point[1]);

	if(conect == 1)
	{
		if(point[0] > -1){
			mi_punto.point[0] = point[0];
			mi_punto.point[1] = point[1]; 

			if (send(sockfd_point, &mi_punto, sizeof(struct point), 0) == -1){
				desconectar();
			}
			fflush(NULL);
			release = 0;
		}
		else if(release == 0){
			enviar_release();
			release = 1;
		}

	}
}*/


void on_btn_conectar_toggled(GtkToggleButton *toggledButton){
    printf("on_btn_conectar \n");
    int status;
    gboolean T = gtk_toggle_button_get_active(toggledButton);
    //background color
    GdkColor color_bg;
    if(T){
        gtk_label_set_text(GTK_LABEL(lbl_connect),(const gchar*) "Conectado"); 
        color_bg.red = 0x0000;
        color_bg.green = 0xffff;
        color_bg.blue = 0x0000;  
        gtk_widget_modify_bg(GTK_WIDGET(lbl_connect),GTK_STATE_NORMAL,&color_bg);
        
         /* Creamos el socket */
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket");
            exit(1);
        } 
	printf("se crea el socket \n");

        /* Establecemos their_addr con la direccion del server */
        their_addr.sin_family = AF_INET;
        their_addr.sin_port = htons(PORT);

        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
		printf("antes de conectarse 1 \n");

        bzero(&(their_addr.sin_zero), 8);
	printf("antes de conectarse \n");

        if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)
        {
            perror("connect");
	    conect=1;
            exit(1);
        }
        else{
	    conect = 1;
	    //creando hilo para escuchar al servidor
	    status = pthread_create(&t_id, NULL, listen_server, (void*) 0);
	    //en caso de error
	    if(status)
	      //    conectado = 0;
	    
	    printf("Coneccion establecida\n");
	}
        
	
        /* Devolvemos recursos al sistema */
        //close(sockfd);

    }
    else{
        gtk_label_set_text(GTK_LABEL(lbl_connect),(const gchar*) "Desconectado");
        color_bg.red = 0xffff;
        color_bg.green = 0x0000;
        color_bg.blue = 0x0000;  
        gtk_widget_modify_bg(GTK_WIDGET(lbl_connect),GTK_STATE_NORMAL,&color_bg);
    }
}

void configure_GUI(){
    p1 = p2 = start = NULL;
    
    red = green = blue = alpha = 0.0;
    red_bg = green_bg = blue_bg = alpha = 0.0;
   
    
    //Accediendo al XML file para que lo pueda leer.
    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "glade/window_main.glade", NULL);
    
    //Como parametro le pasas el builder 
    //y el id en el xml file con el que te queres conectar
    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main")); 
    
    //cuando windows es destroy hacer un callback a la funcion on_destroy
    g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);
    
    //identifica que senales van a querer
    gtk_builder_connect_signals(builder, NULL);
    
    //Accediendo a los distintos widgets del XMLfile
    fixed1 = GTK_WIDGET(gtk_builder_get_object(builder, "fixed1"));
    draw_area = GTK_WIDGET(gtk_builder_get_object(builder, "draw_area"));
    clear = GTK_WIDGET(gtk_builder_get_object(builder, "clear"));
    color_button = GTK_WIDGET(gtk_builder_get_object(builder, "color_button"));
    btn_spin = GTK_WIDGET(gtk_builder_get_object(builder, "btn_spin"));
    btn_conectar = GTK_WIDGET(gtk_builder_get_object(builder, "btn_conectar"));
    btn_dark = GTK_WIDGET(gtk_builder_get_object(builder, "btn_dark"));
    box = GTK_WIDGET(gtk_builder_get_object(builder, "box"));
    lbl_connect = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_connect"));
	
    //no se para que es
    g_object_unref(builder);
    
    /*El moviemiento del boton es parte del sistema gdk, por lo que debe configurar
      los eventos diciendole que podra recibir ciertas cosas del gdk, esta habilitando 
      el movimiento del boton y el presionado del boton para que esten disponibles como 
      eventos y de lo controraio no lo veras.*/
    gtk_widget_set_events(draw_area, GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_window_set_keep_above (GTK_WINDOW(window), TRUE);
    
    //default background color
    GdkColor color_bg;
    
 
    color_bg.red = 0xffff;
    color_bg.green = 0x0000;
    color_bg.blue = 0x0000;
    gtk_widget_modify_bg(GTK_WIDGET(box),GTK_STATE_NORMAL,&color_bg);
}

