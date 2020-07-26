#define max_clientes 10
#define PORT 14550
#define DESCONECTAR '0'
#define NEWCOLORPINCEL '1'
#define NEWCOLORBG '2'
#define NEWDRAWPOINT '3'
#define CLEAR '4'

#define MAXDATASIZE 1024


char * 
inet_ntoa_my(in)
	struct in_addr in;
{
	static char b[18];
	register char *p;

	p = (char *)&in;
	#define	UC(b)	(((int)b)&0xff)
	(void)snprintf(b, sizeof(b),
	    "%d.%d.%d.%d%c", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]),'\0');
	return (b);
}


typedef struct Punto {
	int id;
	double x;
	double y;
	char tipo;
	int pencil_width;
	double red, green, blue,alpha;
} Punto;
