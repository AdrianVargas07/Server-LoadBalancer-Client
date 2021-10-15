#ifndef CLIENTE_HEADER
#define CLIENTE_HEADER
#define BUFFER_SIZE 4096
#define PORT_BROADCAST 9786
#define PORT_HTTP      80

#include "Socket.hpp"
#include "InterpreteHTTP.hpp"
#include <string>
#include <vector>
#include <signal.h>

using namespace std;

class WebServidor{

    private:
        char               comando[128];
        vector<int>        procesos;
        Socket             conexion;
        string             peticion;
        string             peticionParcial;

    public:
        WebServidor(int, int, char*);
        ~WebServidor();
        void               avisarBalanceadores(bool);
        void               ejecutarServidor();
        int                crearProceso(bool);
        Socket*            broadcast;
        char*              direccionHost;
};

WebServidor* webServidor;
static void  controladorInterrupciones(int);



#endif

