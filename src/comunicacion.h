#define max_clientes 10
#define PORT 14550
#define DESCONECTAR '0'
#define NEWCOLORPINCEL '1'
#define NEWCOLORBG '2'
#define NEWDRAWPOINT '3'
#define CLEAR '4'
#define NEWLINEWIDTH '5'

#define MAXDATASIZE 1024


typedef struct Punto {
	int id;
	double x;
	double y;
	char tipo;
	int pencil_width;
	double red, green, blue,alpha;
} Punto;
