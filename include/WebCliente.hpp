#ifndef CLIENTE_HEADER
#define CLIENTE_HEADER
#define BUFFER_SIZE 4096
#include "Socket.hpp"
#include <string>
#include <FileSystem.hpp>

using namespace std;

class WebCliente{

    private:
        Socket         conexion;
		FileSystem     fs;
        string         peticion;
        string         ruta;
        string         htmlver;
        string         host;  
        
    public:
        WebCliente(string, int);
        ~WebCliente();

        void           setPeticion(string);
        void           setRuta(string);
        void           setProtocoloHTML(string);
        void           setHost(string);
        void           realizarPeticion();
        int            leerRespuesta(char*, int);
        void           imprimirRespuesta(char*);
        void           imprimirRespuesta(string);
        string         splitRespuesta(char*, int*);           
        void           construirNuevasRutas(vector<string>*, string);
        Socket*		   getConexion();
        FileSystem*	   getFs();
};

#endif

