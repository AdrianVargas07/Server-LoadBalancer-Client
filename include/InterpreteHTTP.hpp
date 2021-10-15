#ifndef HTTP_HEADER
#define HTTP_HEADER

#define ROOT_FOLDER "public_html"
#define HOME_HTML "index.html"
#define SERVER_NAME "PiroServer"

#include <string.h>
#include <sstream>
#include <map>
#include <locale>
#include <algorithm>
#include <unordered_map>
#include <time.h>
#include "FileSystem.hpp"

using namespace std;

class InterpreteHTTP{

    private:
        FileSystem   fs; 

        bool         enviarSoloHeaders;
        bool         errorOcurrido = false;
        string       respuesta_cuerpo;

        string       respuesta_protocolo;      
        int          respuesta_estadoCodigo;   
        string       respuesta_estadoMensaje; 
        string       respuesta_fecha; 
        string       respuesta_ultimaEdicion; 
        string       respuesta_rangos = "bytes";
        string       respuesta_size; 
        string       respuesta_MIME;

        map<string, string> requestHeaders;

        string bodyErrorParte1 = "<html>\n\t<head>\n\t\t<title></title>\n\t\t<meta content=\"\">\n\t\t<style></style>\n\t</head>\n\t<body>";
        string bodyErrorParte2 = "</body>\n</html>\r\n";


        string       recuperarEstado(int);
        
        void         procesarRequest(string, string, string, string); 
        void         setRutaRecurso(string ruta);
        bool         recursoValido();
        void         setRespuestaFecha();
        void         setRespuestaUltimaEdicion();
        void         setRespuestaSize();
        bool         setRespuestaMIME(string extension);


    public:
        enum STATUS_CODES{
            STATUS_CONTINUE_100 = 100,
            STATUS_OK_200 = 200,
            STATUS_BAD_REQ_400 = 400,
            STATUS_FORBIDDEN_403 = 403,
            STATUS_NOT_FOUND_404 = 404,
            STATUS_INTERNAL_SRV_ERR_500 = 500,
            STATUS_NOT_IMPLMTD_501 = 501,
            STATUS_HTTP_VRSN_NOT_SPPRTD_505 = 505
        };

        InterpreteHTTP();
        ~InterpreteHTTP();
        static const string CRLF;
        static const string DOUBLECRLF;
        static const unordered_map<string, string> MIMETYPES;
        int          construirRespuesta(string httpRequest);
        int          recuperarBody(char* buffer, int pos, int buffersize);
        string       recuperarHeader();
        
};

#endif

