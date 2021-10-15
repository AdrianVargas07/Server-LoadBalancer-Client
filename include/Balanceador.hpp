#ifndef BALANCEADOR_HEADER
#define MAX_SERVIDORES 12
#define MAX_CLIENTES   24
#define PORT_BROADCAST 9786
#define PORT_HTTP      80
#define WHITE   "\033[0m"
#define RED     "\033[31m"
#define ERRORPAGE_HTML_PATH "/bin/errorpage.html"
#define ERRORPAGE_PLACEHOLDER "!\\$ERROR_MESSAGE"
#define BIND true

#include <vector>
#include <iostream>
#include <string>
#include <cstring>
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
#include "Socket.hpp"
#include "FileSystem.hpp"
#include "InterpreteHTTP.hpp"
#include "Utilidades.hpp"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <algorithm>

using namespace std;

struct Estadisticas{
    int servidoresActuales;
    int clientesActuales;
};

struct ServidorRegistrado{
    char  comparador[64];
    char   ip[32];
    int     port;
    int    carga_actual;
    int    carga_maxima;
};

struct ClienteRegistrado{
    sockaddr_in IP;
    ServidorRegistrado* servidor_asociado;
};

class Balanceador{
    public:

        string ip_subnet;
        int puerto_cliente;
        int puerto_servidor;

        FileSystem fs;             
        Estadisticas stats;        
        int filePosition;

        int roundRobin_Actual; 

        mutex mutex_servidores;
        mutex mutex_clientes;
        mutex mutex_mensajes;
        mutex mutex_stats;
        mutex mutex_hilos; 
        mutex mutex_roundRobin;

        bool  ejecutandose;

        queue<string> log_mensajes;

        enum ALGORITMO{
            ROUND_ROBIN       = 0,
            ROUND_ROBIN_PESOS = 1,
            DIRECCION_CLIENTE = 2,
            CONEXIONES_ACTIVA = 3,
        };

        enum STATUS_CODES{
            STATUS_INTERNAL_SRV_ERR_500 = 500,
            STATUS_NOT_IMPLMTD_501 = 501,
            STATUS_BD_GTWY_502 = 502,
            STATUS_SRV_UNAVLBL_503 = 503,
            STATUS_GTWY_TIMEOUT_504 = 504,
            STATUS_HTTP_VRSN_NOT_SPPRTD_505 = 505
        };

        atomic<int> algoritmoBalanceo;

        vector<ServidorRegistrado> servidores;
        vector<ClienteRegistrado>  clientes; 
        vector<thread*> hilos; 

        Socket* escuchador_http;
        Socket* escuchador_broadcast;

        char* direccionHost;
        
        static const string CRLF;
        static const string DOUBLECRLF;
        static const string HTTP_VERSION_1;
        static const string HTTP_VERSION_11;

        Balanceador(int algoritmo, char* ip, int pc, int ps); 
        ~Balanceador();

        void avisarServidores(bool);
        void vincularPuertos();     

        int enviarDatagram(const char* buffer); 
        int recibirDatagram(char* buffer); 
        int recibirMensaje(char* buffer, int bufferSize, Socket* fuente);  
        int enviarMensaje(const char* buffer, int bufferSize, Socket* destino); 

        void log(string mensaje);    
        void logRed(string mensaje);   
        void setAlgoritmoBalanceo(int algoritmo); 
        int  ejecutarBalanceador();                
        void manejarBitacora(); 

        void agregarServidor(char* hilera);  
        void eliminarServidor(char* hilera);                
        int  buscarServidor(const char* hilera);     

        void agregarCliente(Socket* socket);    
        int  buscarCliente(Socket* socket);
        int  buscarServidorConMenorCarga();
        
        string recuperarEstado(int numero);
        string recuperarHeader(int estadoCodigo);
        int    recuperarBodyPaginaError(char* buffer, int buffersize, int error);
        void   mostrarEstadisticas();
};
#endif 
