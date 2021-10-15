#include "InterpreteHTTP.hpp"
#include "Utilidades.hpp"
#include <iostream>

// STATIC CLASS VARIABLES
const string InterpreteHTTP::CRLF = "\r\n";
const string InterpreteHTTP::DOUBLECRLF = "\r\n\r\n";
const unordered_map<string,string> InterpreteHTTP::MIMETYPES{   
    {"arc", "application/octet-stream"},
    {"avi", "video/x-msvideo"},
    {"azw", "application/vnd.amazon.ebook"},
    {"bin", "application/octet-stream"},
    {"bmp", "image/bmp"},
    {"bz", "application/x-bzip"},
    {"bz2", "application/x-bzip2"},
    {"csh", "application/x-csh"},
    {"css", "text/css"},
    {"csv", "text/csv"},
    {"doc", "application/msword"},
    {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"ecmascript", "application/ecmascript"},
    {"epub", "application/epub+zip"},
    {"gif", "image/gif"},
    {"gz", "application/gzip"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"ico", "image/x-icon"},
    {"jpeg", "image/jpeg"},
    {"jpg", "image/jpeg"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"markdown", "text/markdown"},
    {"mjs", "text/javascript"},
    {"odp", "application/vnd.oasis.opendocument.presentation"},
    {"ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {"odt", "application/vnd.oasis.opendocument.text"},
    {"pdf", "application/pdf"},
    {"php", "application/x-httpd-php"},
    {"png", "image/png"},
    {"ppt", "application/vnd.ms-powerpoint"},
    {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {"sh", "application/x-sh"},
    {"svg", "image/svg+xml"},
    {"tar", "application/x-tar"},
    {"tif", "image/tiff"},
    {"tiff", "image/tiff"},
    {"txt", "text/plain"},
    {"xhtml", "application/xhtml+xml"},
    {"xls", "application/vnd.ms-excel"},
    {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"xml", "application/xml"},
    {"zip", "application/zip"},
    {"7z", "application/x-7z-compressed"}
    };

InterpreteHTTP::InterpreteHTTP(){

}

InterpreteHTTP::~InterpreteHTTP(){
    
}

// Requiere: Una hilera con el nombre del metodo HTTP por procesar, una hilera con la ruta del
//           recurso solicitado, una hilera con la representacion textual de la version HTTP de
//           de la solicitud.
// Modifica: 
// Retorna: 
void InterpreteHTTP::procesarRequest(string metodo, string ruta, string htmlv, string headers){

    // ost permite Buscar Host y host al mismo tiempo!
    string host = Utilidades::extraerValorHeader(headers, "ost: ");
    if(host.compare("") == 0){
        this->respuesta_estadoCodigo = STATUS_BAD_REQ_400;
        return;
    }

    if(htmlv.compare(string("HTTP/1.0")) == 0 || htmlv.compare(string("HTTP/1.1")) == 0){
        this->respuesta_protocolo = htmlv; 
    }else if(htmlv.compare(string("HTTP/2.0")) == 0 || htmlv.compare(string("HTTP/3.0"))){
        this->respuesta_protocolo = "HTTP/1.1";
        this->respuesta_estadoCodigo = STATUS_HTTP_VRSN_NOT_SPPRTD_505;
        return;
    }else{
        this->respuesta_protocolo = "HTTP/1.1";
        this->respuesta_estadoCodigo = STATUS_BAD_REQ_400;
        return;
    }

    if(metodo.compare(string("GET")) == 0 || metodo.compare(string("HEAD")) == 0){

        if(metodo.compare(string("HEAD")) == 0){
            this->enviarSoloHeaders = false;
        }

        this->setRutaRecurso(ruta);

        if(fs.existeArchivo()){
            
            fs.extensionArchivo();
            if(setRespuestaMIME(this->fs.getExtension())){

                this->respuesta_estadoCodigo = STATUS_OK_200;
                this->fs.getInformacionArchivo();
                this->setRespuestaUltimaEdicion();
                this->setRespuestaSize();

            }else{

                this->respuesta_estadoCodigo = STATUS_NOT_IMPLMTD_501;
            }

        }else{
            this->respuesta_estadoCodigo = STATUS_NOT_FOUND_404;
            return;
        }

    }else{
        this->respuesta_estadoCodigo = STATUS_BAD_REQ_400;
        return;
    }

}


// Requiere: Solicitud http completa una hilera de texto.
// Modifica:
// Retorna:  Codigo de estado, 200, 404, etc
int InterpreteHTTP::construirRespuesta(string httpRequest){

    this->enviarSoloHeaders = false;
    this->errorOcurrido = false;

    // Separa el GET / HTTP de las cabeceras
    vector<string> request = Utilidades::split(httpRequest, CRLF, 1);

    // Separa todas las partes del request
    vector<string> request_separado = Utilidades::split(request[0], " ", 3);

    if(request.size() < 2 || request_separado.size() < 3){
        this->respuesta_protocolo = "HTTP/1.1";
        this->respuesta_estadoCodigo = STATUS_BAD_REQ_400;
    }else{
        bool abortar = false;
        for(int i=0; i < request_separado.size() ; i++){
            if(request_separado[i].compare("") == 0){
                this->respuesta_protocolo = "HTTP/1.1";
                this->respuesta_estadoCodigo = STATUS_BAD_REQ_400;
                abortar = true;
            }
        }
        if(!abortar){
            procesarRequest(request_separado[0], request_separado[1], request_separado[2], request[1]);
        }
    }

    this->respuesta_estadoMensaje = recuperarEstado(this->respuesta_estadoCodigo);
    return this->respuesta_estadoCodigo;
    
}

// Requiere: El metodo construirRespuesta debe ejecutarse antes.
// Modifica:
// Retorna:  Una hilera que contiene los encabezados de respuesta.
string InterpreteHTTP::recuperarHeader(){

    string stringBuffer;
    stringBuffer.append(this->respuesta_protocolo + " ");
    stringBuffer.append(to_string(this->respuesta_estadoCodigo) + " ");
    stringBuffer.append(this->respuesta_estadoMensaje  + " ");
    stringBuffer.append((CRLF));
    this->setRespuestaFecha();

    stringBuffer.append("Server: ").append(SERVER_NAME);
    stringBuffer.append(CRLF);
    stringBuffer.append("Date: ").append(this->respuesta_fecha);
    stringBuffer.append(CRLF);
    
    switch(this->respuesta_estadoCodigo){
        case STATUS_OK_200:{
            stringBuffer.append("Accept-Ranges: ").append(this->respuesta_rangos);
            stringBuffer.append(CRLF);
            stringBuffer.append("Content-Length: ").append(this->respuesta_size);
            stringBuffer.append(CRLF);
            stringBuffer.append("Content-Type: ").append(this->respuesta_MIME);
            stringBuffer.append(CRLF);
            stringBuffer.append("Last-Modified: ").append(this->respuesta_ultimaEdicion);
            
        }
        break;
    }

    stringBuffer.append(DOUBLECRLF);
    return stringBuffer;
}

// Requiere: Puntero a un buffer, un numero entero que marca la posicion del body desde donde hay que 
//           llenar el buffer, un numero entero que contiene el tamano de buffer.
// Modifica: Modifica el buffer con un body predeterminado solo si el codigo de estado es diferente
//           a 200 OK. Si el estado es 200 OK, se delega la escritura en el buffer a la instancia
//           de FileSystem.
// Retorna:  devuelve un numero entero con la cantidad de bytes escritos en el buffer.
int InterpreteHTTP::recuperarBody(char* buffer, int pos, int buffersize){

    if(this->respuesta_estadoCodigo == STATUS_OK_200){

        if(this->enviarSoloHeaders){
            return 0;
        }

        return this->fs.leerArchivo(buffer, pos, buffersize);
        
    }else{

        if(!this->errorOcurrido){
            string respuesta;
            respuesta += this->bodyErrorParte1;
            respuesta += "\tError Processing Request: ";
            respuesta += this->recuperarEstado(this->respuesta_estadoCodigo);
            respuesta += this->bodyErrorParte2;
            memset(buffer, 0, buffersize);

            int bytesEscritos = snprintf(buffer, buffersize, "%s", respuesta.c_str());
            this->errorOcurrido = true;
            return bytesEscritos;
        }

    }
    
    return 0;
}

// Requiere: un numero cuyo valor coincida con los valores en el enum STATUS_CODES.
// Modifica: 
// Retorna: una hilera con la representacion textual del estado.
string InterpreteHTTP::recuperarEstado(int numero){
    string retorno;

    switch(numero){
        case STATUS_OK_200:
            retorno = "OK";
        break;
        case STATUS_BAD_REQ_400:
            retorno = "BAD REQUEST";
        break;
        case STATUS_FORBIDDEN_403:
            retorno = "FORBIDDEN";
        break;
        case STATUS_NOT_FOUND_404:
            retorno = "NOT FOUND";
        break;
        case STATUS_INTERNAL_SRV_ERR_500:
            retorno = "INTERNAL SERVER ERROR";
        break;
        case STATUS_NOT_IMPLMTD_501:
            retorno = "NOT IMPLEMENTED";
        break;
        case STATUS_HTTP_VRSN_NOT_SPPRTD_505:
            retorno = "HTTP VERSION NOT SUPPORTED";
        break;
    }
    return retorno;
}


// Requiere: hilera de la solicitud HTTP que corresponde al recurso solicitado.
// Retorna:
// Modifica: Establece la ruta en la instancia de FileSystem.
void InterpreteHTTP::setRutaRecurso(string ruta){
    string rutaFisica;

    if(ruta.size() < 1 || ruta.at(ruta.size() - 1) != '/'){
        ruta.append("/");
    }
    
    if(ruta.compare("/") == 0){
        rutaFisica.append(ROOT_FOLDER).append(ruta).append(HOME_HTML);
    }else{
        rutaFisica.append(ROOT_FOLDER).append(ruta);
    }
    
    fs.setRuta(rutaFisica);
}


bool InterpreteHTTP::recursoValido(){
    return this->fs.existeArchivo();
}

// Requiere: 
// Modifica: Establece la fecha de la solicitud en GMT en la variable repuesta_fecha.
// Retorna:
void InterpreteHTTP::setRespuestaFecha(){
    this->respuesta_fecha = Utilidades::getFechaGMT();
}

// Requiere: El metodo construirRespuesta debe ejecutarse antes. El codigo de estado debe 
//           ser 200 OK y el recurso debe existir. El codigo de estado debe ser 200 OK
//           y el recurso debe existir.
// Modifica: Establece la hora de ultima edicion en GMT del recurso solicitado 
//           en la variable respuesta_ultimaEdicion.
// Retorna: 
void InterpreteHTTP::setRespuestaUltimaEdicion(){
    struct timespec fechaModificacion = this->fs.getFechaMod();
    this->respuesta_ultimaEdicion = Utilidades::getFechaGMT(fechaModificacion);
}

// Requiere: El metodo construirRespuesta debe ejecutarse antes. 
//           El codigo de estado debe ser 200 OK y el recurso debe existir.
// Modifica: Establece el tamano en bytes del recurso solicitado en la variable respuesta_size;
// Retorna: 
void InterpreteHTTP::setRespuestaSize(){
    this->respuesta_size = to_string(this->fs.getTamanoArchivo());
}

// Requiere: Una hilera con el nombre abreviado de una extension de archivo sin un punto
//           como prefijo.
// Modifica: Establece el mimetype con el nombre apropiado en la variable respuesta_MIME.
// Retorna:  True si el servidor soporta la extension del archivo solicitado. De lo contrario
//           devuelve false. 
bool InterpreteHTTP::setRespuestaMIME(string extension){
    bool success = false;
    auto tipo = InterpreteHTTP::MIMETYPES.find(extension);

    if(tipo != InterpreteHTTP::MIMETYPES.end())
    {
        this->respuesta_MIME = get<1>(*tipo);
        success = true;
    }
    
    return success;
}