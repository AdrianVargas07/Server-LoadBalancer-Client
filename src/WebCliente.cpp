#include <stdio.h>
#include <string.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> 
#include <iostream>
#include "WebCliente.hpp"
#include "Utilidades.hpp"

//Requiere: una IP y un puerto
//Retorna:  -
//Modifica: Los valores del socket. (lo abre)
WebCliente::WebCliente(string direccion, int port){
    this->conexion.conectar(direccion, port);  
}

//Requiere: -
//Retorna:  -
//Modifica: Los valores del socket. (lo cierra)
WebCliente::~WebCliente(){
    this->conexion.cerrar();
}

//Requiere: Una peticion de HTTP, puede ser GET o HEAD
//Retorna:  -
//Modifica: La variable peticion
void WebCliente::setPeticion(string str){
    this->peticion = str;
}

//Requiere: La ruta que el request quiere acceder
//Retorna:  -
//Modifica: La variable ruta
void WebCliente::setRuta(string str){
    this->ruta = str;
}

//Requiere: La version del protocolo HTTP que usaran los futuros requests
//Retorna:  -
//Modifica: La variable htmlver
void WebCliente::setProtocoloHTML(string str){
    this->htmlver = str;
}

//Requiere:El host que usaran los futuros requests
//Retorna:
//Modifica: La variable host
void WebCliente::setHost(string str){
    this->host = str;
}

//Requiere: -
//Retorna:  -
//Modifica: Realiza una peticion inmediatamente al servidor
void WebCliente::realizarPeticion(){
    this->conexion.escribir(peticion + " " + ruta + " HTTP/" + htmlver + "\r\nhost: " + host +"\r\n\r\n"); 
}

//Requiere: Un buffer de escritura y una bandera de Socket (opcional)
//Retorna:  Numero de bytes que se leyeron
//Modifica: El socket
int WebCliente::leerRespuesta(char* buffer, int flag){
    return this->conexion.leer(buffer, BUFFER_SIZE, flag, false); 
}

//Requiere: Un buffer de una hilera char*
//Retorna:  -
//Modifica: -
void WebCliente::imprimirRespuesta(char* buff){
    printf("\n%s", buff);
}

//Requiere: Un buffer de una hilera en formato string
//Retorna:  -
//Modifica: -
void WebCliente::imprimirRespuesta(string str){
    printf("\n%s", str.c_str());
}

//Esta funcion no es necesaria para esta entrega, pero es preparativa para una futura entrega
//Permitira descargar los recursos que esten en el HTML, desde src y href, etc.
void WebCliente::construirNuevasRutas(vector<string>* rutas, string ruta){
    /*string rutaCorrecta = Utilidades::rutaEnDirectorio(ruta);
    Utilidades::obtenerNuevasRutas(rutas, &(rutaCorrecta), &(this->buffer));*/
}

//Requiere: Buffer modificable, y un puntero modificable que almacena un size
//Retorna:  Retorna la parte antes del corte
//Modifica: Conserva en el buffer la parte cortada, 
//          y size almacena el size almacena el size de buff despues del corte
string WebCliente::splitRespuesta(char* buff, int* size){
    return Utilidades::split(buff, "\r\n\r\n", size);
}

//Requiere: -
//Retorna:  Retorna un puntero que permite acceder a la Clase del FileSystem
//Modifica: -
FileSystem* WebCliente::getFs(){
	return &(this->fs);
}

//Requiere: -
//Retorna:  Retorna un puntero que permite acceder al socket directamente
//Modifica: -
Socket* WebCliente::getConexion(){
	return &(this->conexion);
}

int main(int argc, char **argv){

    // Buffers temporales para los sockets
	char bufferSocket[BUFFER_SIZE];	
    char backbufferSocket[BUFFER_SIZE];	

    // Variables de los parametros por consola
    bool mostrarHeader    = false;
    bool descargable      = false;
    bool imprimible       = false;
    bool badRequestTest   = false;
    bool badHTMLversion   = false;
    bool moreThanHeaders  = false;

    // Manejo de flags por consola
    int  ip_argvPos = -1;
    if (argc < 2){
        printf("Uso correcto: ./bin/WebCliente ip:puerto/ruta parámetros\n");
        printf("Escriba -help para ver todos los parámetros disponibles.\n");
        exit(0);
    }

    for(int i = 0; i < argc; i++){

        if(strcmp("-help", argv[i]) == 0){
            printf("Uso correcto: ./bin/WebCliente ip:puerto/ruta parámetros\n");
            printf("Todos los parámetros disponibles: \n");
            printf("-headers     Muestra el encabezado HTML del recurso. \n");
            printf("-download    Descarga el recurso como un archivo con su mismo nombre. \n");
            printf("-testBADR    Envia una request incorrecto, para probar el bad request. \n");
            printf("-testHTML    Envia una version no soportada de HTML por nuestro server. \n\n");
        }

        if(strcmp("-headers", argv[i]) == 0){
            mostrarHeader = true;
        }

        if(strcmp("-download", argv[i]) == 0){
            descargable = true;
            moreThanHeaders = true;
        }

        if(strcmp("-testBADR", argv[i]) == 0){
            badRequestTest = true;
            moreThanHeaders = true;
        }

        if(strcmp("-testHTML", argv[i]) == 0){
            badHTMLversion = true;
            moreThanHeaders = true;
        }

        if(argv[i][0] != '-' && i > 0){
            ip_argvPos = i;
        }

    }

    if(ip_argvPos < 1){
        exit(0);
    }

    // Splitea los datos que le llegan por ruta
    string ruta = argv[ip_argvPos];
    if(ruta.find("/") == string::npos){
        ruta += "/";
    }
    string direccion = Utilidades::split(&ruta, "/");
    vector<string> ipaddress = Utilidades::split(direccion, ":", 1);
    
    // Asuma el puerto 80 si no existe en los argumentos
    if(ipaddress.size() < 2){
        ipaddress.push_back("80");
    }

    // Construye un cliente y su respectiva peticion
    WebCliente cliente(ipaddress[0], stoi(ipaddress[1]));

    cliente.getConexion()->prepararTimeOut();
    cliente.setPeticion("GET");
    if(mostrarHeader && !moreThanHeaders){
        cliente.setPeticion("HEAD");
    }
    cliente.setRuta("/"+ruta);
    cliente.setProtocoloHTML("1.1");

    if(badRequestTest){
        cliente.setRuta("ocarinaOfTime");
        cliente.setProtocoloHTML("");
        cliente.setPeticion("");
    }

    if(badHTMLversion){
        cliente.setProtocoloHTML("2.0");
    }

    cliente.setHost("www.google.com");

    // Ejecutela y espere una respuesta
    cliente.realizarPeticion();
    cliente.leerRespuesta(backbufferSocket, MSG_WAITALL);
	
    // Obtenga los headers, en bufferSocket quedara el resto del mensaje
    // sizePostSplit almacena el size del body despues de quitarle los headers
    int sizePostSplit;
    string headers = cliente.splitRespuesta(backbufferSocket, &sizePostSplit); 
    sizePostSplit = BUFFER_SIZE - sizePostSplit;

    if(mostrarHeader){
        cliente.imprimirRespuesta(headers);
        if(!moreThanHeaders){
            exit(0);
        }else{
            printf("\n");
        }
    }

    // Obtenga el mime-type para saber si debe imprimirse o no
    vector<string> headers_lines = Utilidades::split(headers, "\r\n", 2);
    vector<string> status_code = Utilidades::split(headers_lines[0], " ", 2);
    
	string mimetipo = Utilidades::extraerValorHeader(headers,"Content-Type: ");

	if(mimetipo.compare("text/html") == 0 || mimetipo.compare("text/javascript") == 0 || mimetipo.compare("text/html") == 0 || status_code[1].compare("200") != 0 ){
		imprimible = true;
	}
	
	if(!descargable && !imprimible){
		printf("No se puede imprimir datos binarios. Utilize -download\n");
		exit(0);
	}

    if(status_code[1].compare("200") != 0){
        descargable = false;
    }
	
	auto linea = 0;

	while((ruta.find("/",linea))!= std::string::npos){
		linea = ruta.find("/",linea);
		if((linea)!=std::string::npos){
			linea+=1;
		};
	}
	
	string nombreArchivo = ruta.substr(linea);
    if (nombreArchivo.size() < 2){
        nombreArchivo = "index.html";
    }

	int bytesLeidos;
    int posBuffer = 0;

    // Cree los directorios y carpetas para contener todo lo que se necesite
    if(descargable){
		cliente.getFs()->crearDirectorios("/descargas/");
		cliente.getFs()->setRuta("/descargas/" + nombreArchivo);
		cliente.getFs()->crearArchivo();
		cliente.getFs()->setRuta("descargas/" + nombreArchivo);
	}

    // El primer paquete usa el buffer del splitteo anterior
    if(imprimible && !descargable){
        printf("%s", backbufferSocket);
	}else if(descargable){
        printf("Se ha solicitado descargar el archivo: %s\n", nombreArchivo.c_str());
		cliente.getFs()->escribirArchivo(backbufferSocket, 0, sizePostSplit);
        posBuffer += sizePostSplit;
    }

    // Ahora con un while, descargue el resto del archivo, es full body, no contiene headers
    while((bytesLeidos = cliente.getConexion()->leer(bufferSocket, BUFFER_SIZE, MSG_WAITALL, false)) >0 ){
        
		if(imprimible && !descargable){
			printf("%s", bufferSocket);
		}
		if(descargable){ 
			cliente.getFs()->escribirArchivo(bufferSocket, posBuffer, bytesLeidos);
		}

        memset(bufferSocket, 0, BUFFER_SIZE);
		posBuffer += bytesLeidos;

	}

    if(descargable){
        printf("Se ha descargado en: %s/descargas/%s\n", cliente.getFs()->getDirectorioActual().c_str(),
        nombreArchivo.c_str());
    }

    printf("\n");

}

