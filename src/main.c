#include <gtk/gtk.h>
#include <glib.h>
#define _GNU_SOURCE
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>

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
//arreglo de miembros
pthread_t t_id; //hilo de ejecucion que escucha al servidor
int conect;
char buffer[MAXDATASIZE]; // Buffer donde se reciben los datos
struct hostent *he; // Se utiliza para convertir el nombre del hosta su dirección IP
struct sockaddr_in addr; // dirección del server donde se conectara 
char *ip;
int stop=0;
int sockfd_point = -1;


/*Arreglos de structs para almacenar los puntos de los usuarios*/
struct Punto *last_point;
struct Punto *old_point;
struct Punto * usuarios_pizarra;//para almacenar los colores de los miembros (lo distingo de los puntos que son volatiles)


GdkColor **colores_pinceles_usuarios;
GdkRGBA **pinceles_usuarios;
struct Punto last;

int iniciar_conexion=0;
int draw_other[10];
//

//Variables globales GUI

struct Point {
	int x;
	int y;
	double red, green, blue,alpha;
	struct Point *next;
} *p1, *p2, *start;

//ultimas modificaciones
GdkRGBA *pincel=NULL;
GdkPixbuf *mapa_pixels=NULL;
GdkColor *c=NULL;
gint point[2];
int line_width = 4;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//fin ultimas modificaciones.

struct Punto color_pincel;
GtkWidget  *window;
GtkWidget  *fixed1;
GtkWidget  *draw_area;
GtkWidget  *clear;
GtkWidget  *color_button;
GtkWidget  *btn_spin;
GtkWidget  *btn_conectar;
GtkWidget  *lbl_connect;
GtkWidget  *btn_bg;
GtkWidget  *box;
GtkBuilder *builder; 

double red, green, blue, alpha;
double red_bg , green_bg, blue_bg, alpha_bg;
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

    if (event->state & GDK_BUTTON1_MASK ) 
    draw_brush (widget, event->x, event->y);
    return TRUE;
    }

static void draw_brush (GtkWidget *widget, gdouble x, gdouble y) {

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

void send_select_color_bg(){
    if(conect == 1)
    {	
	char mensaje[MAXDATASIZE];
	memset(mensaje,'\0',1);
	char tipo[1],ared[8],ablue[8],agreen[8];
	memset(ared,'\0',1);
	memset(ablue,'\0',1);
	memset(agreen,'\0',1);
	tipo[0]= NEWCOLORBG;
	strcpy(mensaje,tipo);
	strcat(mensaje,"|");
	sprintf(ared,"%lf",red_bg);
	strcat(mensaje,ared);
	strcat(mensaje,"|");
	sprintf(agreen,"%lf",green_bg);
	strcat(mensaje,agreen);
	strcat(mensaje,"|");
	sprintf(ablue,"%lf",blue_bg);
	strcat(mensaje,ablue);

	if (send(sockfd_point, mensaje, sizeof(mensaje), 0) == -1){
	    perror("send: ");
	    exit(EXIT_FAILURE);
	}
	
	printf("send_select_Color_bg EL cliente envio el mensaje %s \n",mensaje);
	fflush(NULL);
    }
    
}

/*
void on_color_button_color_set(GtkWidget *c) { 
    GdkRGBA color; //es un struct
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c),&color);
    red = color.red;  green = color.green; blue = color.blue; alpha = color.alpha;
    send_select_color();
}*/

void on_btn_bg_color_set(GtkWidget *c){
    printf("se dispara el evento on_btn_bg_Color_set \n");
    GdkRGBA color; //es un struct
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c),&color);
    printf("Color rojo %lf, verde %lf, azul %lf \n",color.red,color.green,color.blue);	
    update_color_bg(color.red,color.green,color.blue);
    send_select_color_bg();
}
//finaliza el hilo listen_Server
void desconectar()
{
	printf("El servidor esta caido: cerrando coneccion\n");
	stop = 1;
}


void update_color_bg(double r_bg,double g_bg, double b_bg){
    printf("update_color_bg red es %lf \n",r_bg);
    GdkRGBA cbg;
    cbg.red = r_bg;
    cbg.green = g_bg;
    cbg.blue = b_bg;
    
    gtk_widget_modify_bg(GTK_WIDGET(window),GTK_STATE_NORMAL,&cbg);

}

void procesar_mensaje_recibido(){
    char value[8];
    memset(value,'\0',1);
    char men[1024];
    strcpy(men,buffer);
    last.tipo=men[0];
    printf("estoy en procesar_mensaje recibido el tipo es %c\n",last.tipo);

    int i=2; //saltea tipo|
    while(men[i]!='|'){
        value[i-2]=men[i];
        i++;
    }

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
    while(men[i]!='|'){
        value[j]=men[i];
        i++;
        j++;
    }
    sscanf(value,"%lf",&last.blue);
    i++; //saltea tipo|red|green|blue
    j=0;
    while(men[i]!='\0'){
        value[j]=men[i];
        i++;
        j++;
    }
    //recorrio tipo|red|green|blue|id
    sscanf(value,"%d",&last.id);
}
/*
void cambiar_color_pincel(int id){
    printf("estoy en cambiar color pincel \n");
    colores_pinceles_usuarios[id]->red = usuarios_pizarra[id].red;
    colores_pinceles_usuarios[id]->green = usuarios_pizarra[id].green;
    colores_pinceles_usuarios[id]->blue = usuarios_pizarra[id].blue;
    
    /*GdkRGBA color; //es un struct
    color.red =  colores_pinceles_usuarios[id].red;
    color.green = colores_pinceles_usuarios[id].green;
    color.blue = colores_pinceles_usuarios[id].blue; 
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color_button),&color);
    
    gdk_color_alloc (gdk_colormap_get_system (), colores_pinceles_usuarios[id]);
    gdk_gc_set_foreground (pinceles_usuarios[id], colores_pinceles_usuarios[id]);
    gdk_gc_set_line_attributes(pinceles_usuarios[id],usuarios_pizarra[id].pencil_width,GDK_LINE_SOLID,
    GDK_CAP_BUTT, GDK_JOIN_ROUND);
}
*/
//crea y retorna un GdkGC* que simboliza a un crayon en la libreria
//debe recibir el mapa de pixeles, las componenetes de color y una instancia de GdkColor *
GdkRGBA *getPincel (GdkKeymap *mapa_pixels, double n_red, double n_green, double n_blue)
{	
	GdkRGBA *color = (GdkRGBA *) g_malloc (sizeof (GdkRGBA)); //es un struct
	color->red=n_green;  color->green=n_green; color->blue=n_blue;
	gtk_color_chooser_set_rgba(color_button,color);
	return (color);
}

//funcion que ejecuta el hilo que se conecta con el servidor
void listen_server()
{
    int i = 0;
    char tipo;
    int id;
    int stop = 0;
    while(!stop){
	printf("stop es %d \n",stop);
	int num = recv(sockfd_point, buffer, strlen(buffer), 0);
	printf("num bytes recibidos %d \n",num);
	if (num == -1){
		perror("recv");
		printf("FUNCION PUNTO: NO SE RECIBIERON DATOS DEL CLIENTE %i\n", id);
		stop = 1;

	}
	else //El servidor envio un cambio echo por un cliente.
	{
	    fflush(NULL);
	    {
		id = last.id;
		printf("El cliente recibio del servidor el siguiente mensaje: %s con tamano \n",buffer,strlen(buffer));
		
		procesar_mensaje_recibido();
		//Actualiza los valores de los dos ultimos puntos del cliente.
		last_point[id].id = id;
		old_point[id].x = last_point[id].x;
		old_point[id].y = last_point[id].y;
		
		switch(last.tipo){
		    case NEWCOLORPINCEL:
			usuarios_pizarra[id].red=last.red;
			usuarios_pizarra[id].green=last.green;
			usuarios_pizarra[id].blue=last.blue;
			//cambiar_color_pincel(id);
			break;
		    case NEWCOLORBG:
			gdk_threads_enter(); //entra a la seccion critica
			update_color_bg(last.red,last.blue,last.green);
			gdk_threads_leave(); //deja la seccion critica.
		    default:
			break;

		}
	    }
	}
    }
    conect=0;
    pthread_exit(0);
    //limpia el buffer
    //memset(buffer,'\0',MAXDATASIZE);
}


void on_btn_conectar_toggled(GtkToggleButton *toggledButton){
    printf("on_btn_conectar \n");
    gboolean T = gtk_toggle_button_get_active(toggledButton);
    //background color
    GdkColor color_bg;
    int i;
    if(T){
	if(!iniciar_conexion){
	    iniciar_conexion = 1;
	    
	    //guardan los ultimos 2 puntos de cada miembro para dibujar una recta
	    last_point = (struct Punto*)malloc(sizeof(struct Punto)*max_clientes);
	    old_point = (struct Punto*)malloc(sizeof(struct Punto)*max_clientes);
	    
	    usuarios_pizarra = (struct Punto*)malloc(sizeof(struct Punto)*max_clientes);
	    pinceles_usuarios = (GdkRGBA*)malloc(sizeof(GdkRGBA *)*max_clientes);
	    colores_pinceles_usuarios = (GdkColor*)malloc(sizeof(GdkColor *)*max_clientes);
	    /*
	    int i;
	    for(i=0;i<max_clientes;i++)
	    {
		draw_other[i] = 0;
		//creo crayones para los demas miembros
		//pinceles_usuarios[i] = getPincel(mapa_pixels,0.0,0.0,1.0);
		//printf("salgo de esa funcion %d vez \n",i);
	    }*/
	}
	if(!conect){
	    int status;
	    stop=0;
	    conect=1;
	    gtk_label_set_text(GTK_LABEL(lbl_connect),(const gchar*) "Conectado"); 
	    color_bg.red = 0x0000;
	    color_bg.green = 0xffff;
	    color_bg.blue = 0x0000;  
	    gtk_widget_modify_bg(GTK_WIDGET(lbl_connect),GTK_STATE_NORMAL,&color_bg);
	    
	    if ((he = gethostbyname(ip)) == NULL){
		printf("ERROR EN EL NOMBRE O DIRECCION INGRESADA %s NO ES UN NOMBRE VALIDO\n", ip);
		conect = 0;
		exit(EXIT_FAILURE);
	    }
	    
	    //apertura del socket q retorna un descriptor q el sistema asocia con una conexion de red
	    if ((sockfd_point = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		    printf("ERROR EN LA CREACION DEL SOCKET\n");
		    conect = 0;
		    exit(EXIT_FAILURE);
	    }

	    /* Establece addr con la direccion del server */
	    addr.sin_family = AF_INET;
	    addr.sin_port = htons(PORT);
	    addr.sin_addr = *((struct in_addr *)he->h_addr); 
	    bzero(&(addr.sin_zero), 8); 
	    //se queda bloqueado hasta que el servidor acepte la conexion.
	    if (connect(sockfd_point, (struct sockaddr*) &addr, sizeof(struct sockaddr)) == -1){
		    printf("NO SE PUDO CONECTAR CON EL SERVIDOR\n");
		    conect = 0;
		    exit(EXIT_FAILURE);
	    
	    }else{
		    printf("sockfd_point vale %d\n",sockfd_point);
		    
		    //creando hilo para escuchar al servidor
		    status = pthread_create(&t_id, NULL, listen_server, (void*) 0);
		    //en caso de error
		    if(status)
			  conect = 0;
		    //enviando el color actual al servidor
		    printf("Conexion establecida\n");
	    }
	}

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
    btn_bg = GTK_WIDGET(gtk_builder_get_object(builder, "btn_bg"));
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
    
 
    color_bg.red = 0x0000;
    color_bg.green = 0x0000;
    color_bg.blue = 0x6000;
    gtk_widget_modify_bg(GTK_WIDGET(box),GTK_STATE_NORMAL,&color_bg);
}

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
    memset(buffer,'\0',MAXDATASIZE);
    configure_GUI();
    
    // Le dice al sistema que muestre la ventana q tenemos guardada en el puntero window 
    gtk_widget_show(window);
    
    //espera por senales y eventos y se ejecuta el callback correspondiente.
    gtk_main();
    gdk_threads_leave();

    return EXIT_SUCCESS;
}

