#ifndef SOCKET_HEADER
#define SOCKET_HEADER

#include <netinet/in.h>
#include <string>
#include "Utilidades.hpp"
using namespace std;

class Socket{

    private:
        bool stream;
        bool ipv6;
        int  id;

    public:
        Socket(bool, bool);
        Socket();
        ~Socket();

        void    prepararTimeOut();
        int     conectar(string, int);
        void    cerrar();
        int     leer(string*, int, int, bool);
        int     leer(char *, int, int, bool);
        int     leerDe(char*, int);
        int     leerDe(string*, int);
        int     escribir(string);
        int     escribir(const char *,int);
        int     escribirA(const char*, int);
        int     escribirA(string);
        int     vincular(char*, int);
        int     vincularBroadcast(const char*, int, bool);
        int     escuchar(int);
        Socket* aceptar();
        int     apagar(int);
        void    asignarID(int);


        struct sockaddr_in  in_IPv4;
        struct sockaddr_in6 in_IPv6;
        
    
};

#endif

