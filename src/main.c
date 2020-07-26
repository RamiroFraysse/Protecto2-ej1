#include <gtk/gtk.h>
#include <glib.h>
#define _GNU_SOURCE

#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>
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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int conect;
struct hostent *he; // Se utiliza para convertir el nombre del hosta su dirección IP
struct sockaddr_in addr; // dirección del server donde se conectara 
char *ip;   
int stop=0;
int sockfd_point = -1;


struct Punto * usuarios_pizarra;//para almacenar los colores de los miembros
int iniciar_conexion=0;

//Variables globales GUI

struct Point {
	int x;
	int y;
	double red, green, blue,alpha;
	struct Point *next;
} *p1, *p2, *start, *end;

double line_width = 1.0;


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
GtkWidget  *color_bg_chooser;
GtkWidget  *color_pen_chooser;
GtkBuilder *builder; 

double red, green, blue, alpha;


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
void on_id_erase_toggled(GtkCheckButton *toggledCheck);
static void draw_brush (GtkWidget *widget, gdouble  x, gdouble  y);
void configure_GUI();
void send_new_draw_punto();
void update_color_bg();

	

void on_destroy() {
    if(sockfd_point!=-1){
	stop = 1;
	Punto cerrar;
	cerrar.tipo=DESCONECTAR;

	if (send(sockfd_point, &cerrar, sizeof(Punto), 0) == -1)
		printf("Error la comunicarse con el servidor\n");
	stop = 1;
	close(sockfd_point);
    }
    gtk_main_quit();
} 
        
/*
 * Esta es la funcion que se llamara cuando el sistema quiera dibujar en el drawing area
*/
gboolean on_draw_area_draw (GtkWidget *widget, cairo_t *cr, gpointer data) {

    //Establece el ancho de la linea
    cairo_set_line_width(cr, line_width);

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
    	p1 = p1->next;
    }

    return FALSE;
}

//se ejecuta ante un evento de release e indica que debe dejar de dibujarse ante un motion event
gboolean on_draw_area_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
    
    
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
    end = NULL;
    //limpia el area de dibujo.
    gtk_widget_queue_draw (draw_area);

    if(conect){
      Punto new;
	    new.tipo = CLEAR;
	    int bytes=send(sockfd_point, &new, sizeof(Punto), 0);
	    if ( bytes== -1){
	      perror("send clear: ");
	      exit(EXIT_FAILURE);
	    }
    }
}

void borrarTodo() {
    p1 = start;
    //mientras q p1 is not null
    while (p1) { 
	    //libera todo los punteros.
	    p2 = p1 -> next; 
	    free(p1); 
	    p1 = p2; 
    }
    start = NULL;
    end = NULL;
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
    p1->next = NULL;    

    if( start == NULL && end == NULL ){
      p1->next = end;
      end = p1;    
      start =  p1;
      
    }else{
      end->next = p1;
      end = p1;      
    }
    //start = p1;
    send_new_draw_punto(x,y);
    gtk_widget_queue_draw (draw_area);
}


static void draw_brush_original (GtkWidget *widget, gdouble x, gdouble y) {

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
    send_new_draw_punto(x,y);
    gtk_widget_queue_draw (draw_area);
}

void dibujar(Punto new){
    
    p1 = malloc (sizeof(struct Point));
    if (p1 == NULL) { printf("out of memory\n"); abort(); }
    p1->x = new.x;
    p1->y = new.y;
    p1->red = new.red;
    p1->green = new.green;
    p1->blue = new.blue;
    p1->alpha = new.alpha;
    p1->next = NULL;    

    if( start == NULL && end == NULL ){
      p1->next = end;
      end = p1;    
      start =  p1;
      
    }else{
      end->next = p1;
      end = p1;      
    }
    //start = p1;
    //send_new_draw_punto(x,y);
    gtk_widget_queue_draw (draw_area);
}

void send_new_draw_punto(double x, double y){
    if(conect == 1){

	Punto new;
	new.tipo=NEWDRAWPOINT;
	new.red = red;
	new.green=green;
	new.blue=blue;
	new.x=x;
	new.y=y;
	int bytes=send(sockfd_point, &new, sizeof(Punto), 0);
	if ( bytes== -1){
	    perror("send: ");
	    exit(EXIT_FAILURE);
	}
    }
}

void send_select_color_bg(double r, double g, double b,double a){
    if(conect == 1)
    {	
	Punto color_bg;
	color_bg.tipo = NEWCOLORBG;
	color_bg.red = r;
	color_bg.green = g;
	color_bg.blue = b;
	color_bg.alpha = a;

	if (send(sockfd_point, &color_bg, sizeof(Punto), 0) == -1){
	    perror("send: ");
	    exit(EXIT_FAILURE);
	}	
    }
    
}

void send_new_color_pincel(double red,double green,double blue){
    Punto new;
    new.tipo = NEWCOLORPINCEL;
    new.red = red;
    new.green = green;
    new.blue = blue;
    if (send(sockfd_point, &new, sizeof(Punto), 0) == -1){
	    perror("send: ");
	    exit(EXIT_FAILURE);
    }
}

void on_color_button_color_set(GtkWidget *c) { 
    GdkRGBA color; //es un struct
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c),&color);
    red = color.red;  
    green = color.green; 
    blue = color.blue; 
    alpha = color.alpha;
    if(conect){    
      send_new_color_pincel(red,green,blue);
    }
}

void on_btn_bg_color_set(GtkWidget *c){
    GdkRGBA color; //es un struct
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(c),&color);
    update_color_bg(color.red,color.green,color.blue,color.alpha);
    send_select_color_bg(color.red,color.green,color.blue,color.alpha);
}

void on_btn_spin_value_changed(GtkButton *b){
    gdouble val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(btn_spin)); 
    line_width = val;
}

void desconectar()
{
	stop = 1;
}


void update_color_bg(double r_bg,double g_bg, double b_bg,double a_bg){
    fflush(NULL);

    GdkRGBA cbg;
    cbg.red = r_bg;
    cbg.green = g_bg;
    cbg.blue = b_bg;
    cbg.alpha = a_bg;    

    gtk_color_chooser_set_rgba(color_bg_chooser, &cbg);
    gtk_widget_override_background_color(GTK_WIDGET(window),GTK_STATE_NORMAL,&cbg);

}

//funcion que ejecuta el hilo que se conecta con el servidor
void* listen_server(void *args)
{
    
    int i = 0;
    
    char tipo;
    int id;
    int stop = 0;
    while(!stop){
	Punto update;
	int num = recv(sockfd_point, &update, sizeof(Punto), 0);
	if (num == -1){
		perror("recv");
		printf("FUNCION PUNTO: NO SE RECIBIERON DATOS DEL CLIENTE %i\n", id);
		stop = 1;

	}
	else //El servidor envio un cambio echo por un cliente.
	{
	    fflush(NULL);
	    {
		id = update.id;
		switch(update.tipo){
		  case NEWCOLORPINCEL:
			  usuarios_pizarra[id].red=update.red;
			  usuarios_pizarra[id].green=update.green;
			  usuarios_pizarra[id].blue=update.blue;
			  printf("EL usuario %d ahora tiene el color azul %lf \n",id,update.blue);
			  break;
		    
		  case NEWCOLORBG:
			  gdk_threads_enter();//entra a la seccion critica
			  update_color_bg(update.red,update.green,update.blue,update.alpha);
			  gdk_threads_leave(); //deja la seccion critica.
        break;			

		  case NEWDRAWPOINT:
			  gdk_threads_enter();//entra a la seccion critica
			  dibujar(update);
			  gdk_threads_leave(); //deja la seccion critica.
        break;		    

		  case CLEAR:
        gdk_threads_enter();//entra a la seccion critica
			  borrarTodo();
			  gdk_threads_leave(); //deja la seccion critica.
        break;			  

		    /*case NEWLINEWIDTH:
			gdk_threads_enter();//entra a la seccion critica
			line_width = update.x;
			gdk_threads_leave(); //deja la seccion critica.*/
		  default:
			  break;
		    
		}
	    }
	}
    }
    conect=0;
    pthread_exit(0);
   
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
	    usuarios_pizarra = (struct Punto*)malloc(sizeof(struct Punto)*max_clientes);
	    
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


void on_id_erase_toggled(GtkCheckButton *toggledCheck){

  gboolean isEraseActive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggledCheck));
  GdkRGBA color;
  gdouble val;
  
  if(isEraseActive){
    // Pinto del color del fondo
    gtk_color_chooser_get_rgba(color_bg_chooser, &color);
    red = color.red;
    green = color.green;
    blue = color.blue;
    alpha = color.alpha;
  }else{
    gtk_color_chooser_get_rgba(color_pen_chooser, &color);
    red = color.red;
    green = color.green;
    blue = color.blue;
    alpha = color.alpha;
  }
  
}

void configure_GUI(){
    p1 = p2 = start = end = NULL;
    
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
    color_bg_chooser = GTK_WIDGET(gtk_builder_get_object(builder, "btn_bg"));
    color_pen_chooser = GTK_WIDGET(gtk_builder_get_object(builder, "color_button"));
    lbl_connect = GTK_WIDGET(gtk_builder_get_object(builder, "lbl_connect"));
    
	
    //no se para que es
    g_object_unref(builder);
    
    /*El moviemiento del boton es parte del sistema gdk, por lo que debe configurar
      los eventos diciendole que podra recibir ciertas cosas del gdk, esta habilitando 
      el movimiento del boton y el presionado del boton para que esten disponibles como 
      eventos y de lo controraio no lo veras.*/
    gtk_widget_set_events(draw_area, GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    gtk_window_set_keep_above (GTK_WINDOW(window), TRUE);
    
    // Ancho de linea inicial
    gtk_spin_button_set_value(btn_spin, 2);
    
    //default background color for window
    GdkRGBA cbg;
    cbg.red = 0xffff;
    cbg.green = 0xffff;
    cbg.blue = 0xffff;
    cbg.alpha = 0xffff;

    gtk_color_chooser_set_rgba(color_bg_chooser, &cbg);

    gtk_widget_override_background_color(GTK_WIDGET(window),GTK_STATE_NORMAL,&cbg);


    //default background color for pen
    GdkRGBA pen_color;
    
    red = 0x0000;
    green = 0x0000;
    blue = 0x0000;
    alpha = 0xffff;
    pen_color.red = red;
    pen_color.green = green;
    pen_color.blue = blue;
    pen_color.alpha = alpha;

   
    gtk_color_chooser_set_rgba(color_pen_chooser, &pen_color);

    gtk_widget_override_background_color(GTK_WIDGET(window),GTK_STATE_NORMAL,&cbg);

    //default background color for box
    GdkColor color_bg;
    
    color_bg.red = 0xaaaa;
    color_bg.green = 0xaaaa;
    color_bg.blue = 0xaaaa;
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
    configure_GUI();
    
    // Le dice al sistema que muestre la ventana q tenemos guardada en el puntero window 
    gtk_widget_show(window);
    
    //espera por senales y eventos y se ejecuta el callback correspondiente.
    gtk_main();
    gdk_threads_leave();

    return EXIT_SUCCESS;
}

