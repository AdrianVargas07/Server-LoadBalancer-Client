#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <errno.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <signal.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <unistd.h>
#include <unistd.h> 
#include "WebServidor.hpp"
#include "Utilidades.hpp"
#define BIND true

bool* ejecutandose  = (bool*) Utilidades::crearMemoriaCompartida(1);
bool* conectado     = (bool*) Utilidades::crearMemoriaCompartida(1);
int puertoRequests  = 0;
int puertoBroadcast = 0;

//Requiere:  Puerto de http, puerto de broadcast, IP de este servidor en la subred
//Retorna:   -
//Modifica:  -
WebServidor::WebServidor(int p, int p2, char *ip){

  *ejecutandose   = true;
  *conectado      = false;

  puertoRequests  = p;
  puertoBroadcast = p2;

  // Si no puedo vincularme, termine
  if(this->conexion.vincular(nullptr, puertoRequests) == -1){
    Utilidades::colorError();
    printf(" * Servidor %s/ fallo en abrir.\n", "/" ROOT_FOLDER); 
    Utilidades::colorReset();
    printf(" * Acceso denegado al puerto %d, intente con otro puerto. \n", puertoRequests); 
    printf(" * Uso: ./bin/WebServidor <direccion-ip> <puertoHttp> <puertoBroadcast>\n");
    Utilidades::colorError();
    printf(" * Abortando el programa... \n");
    _exit(1);
  }

  this->conexion.escuchar(128);
  this->direccionHost = ip;

  if(this->direccionHost == nullptr){
    Utilidades::colorError();
    printf(" * Servidor %s/ fallo en abrir.\n", "/" ROOT_FOLDER); 
    Utilidades::colorReset();
    printf(" * No se pudo recuperar la ip local. \n");
    Utilidades::colorError();
    printf(" * Abortando el programa... \n");
    _exit(1);
  }

  // Si no puedo vincularme, termine
  this->broadcast = new Socket(false, false);
  if((this->broadcast->vincularBroadcast(Utilidades::getBroadcastIP(Utilidades::recuperarSubnet(), 28).c_str(), puertoBroadcast, BIND)) == -1){
    Utilidades::colorError();
    printf(" * Servidor %s/ fallo en abrir.\n", "/" ROOT_FOLDER); 
    Utilidades::colorReset();
    printf(" * Acceso denegado al puerto %d, broadcast ocupado. \n", puertoBroadcast); 
    Utilidades::colorError();
    printf(" * Abortando el programa... \n");
    _exit(1);
  }
  printf(" * Puerto de Requests: %d \n", puertoRequests);
  printf(" * Puerto de Broadcast: %d \n", puertoBroadcast);
  printf(" * Direccion IP Local: %s \n", this->direccionHost);
  printf(" * Direccion IP Subnet: %s \n", Utilidades::recuperarSubnet());
  sleep(1);

  // Control del while en multihilo
  *ejecutandose = true;
}

//Requiere:  -
//Retorna:   -
//Modifica:  -
WebServidor::~WebServidor(){
  printf("\n"); 
  if(*conectado){
    printf(" * Servidor %s/ cerrado.\n", "/" ROOT_FOLDER); 
    webServidor->avisarBalanceadores(true);
  }
  this->conexion.apagar(2);
  this->conexion.cerrar();
  webServidor->broadcast->apagar(2);
  webServidor->broadcast->cerrar();
}

// No usa interrupcion, maneja el Control+C
//Requiere:  -
//Retorna:   -
//Modifica:  -
void controladorInterrupciones(int interrupcion){
  *ejecutandose = false;
}

//Requiere:  Booleano que indica si este hilo debe ser matado en el futuro automaticamente o manualmente
//Retorna:   El pid (Process ID)
//Modifica:  -
int WebServidor::crearProceso(bool recordar){
	int pID = fork();
  if(pID == -1){
    Utilidades::colorError();
    printf(" * Ha ocurrido un error inesperado, abortado.\n"); 
    Utilidades::colorReset();
    exit(1);
  }else if (pID > 0 && recordar){
    procesos.push_back(pID);
  }
  return pID;

}

//Requiere:  Avisa a un balanceador si yo estoy conectandome o desconectandose
//Retorna:   -
//Modifica:  -
void WebServidor::avisarBalanceadores(bool desconectar){

  if(this->broadcast != nullptr){
    webServidor->broadcast->apagar(2);
    webServidor->broadcast->cerrar();
  }

  this->broadcast = new Socket(false, false);
  this->broadcast->vincularBroadcast(Utilidades::getBroadcastIP(Utilidades::recuperarSubnet(), 28).c_str(), puertoBroadcast, BIND);

  // Preparo el mensaje que tengo que enviar
  string bufferBroadcast = "S/C/";
  if(desconectar){
    bufferBroadcast = "S/D/";
    printf(" * El servidor envia DISCONNECT al balanceador. \n");
  }

  bufferBroadcast += string(this->direccionHost);
  bufferBroadcast += "/" + to_string(puertoRequests);

  // Envio el broadcast y espero respuesta
  if ((this->broadcast->escribirA(bufferBroadcast)) == -1){
    Utilidades::colorError();
    printf(" * El servidor fallo en escribir en broadcast. \n");
    perror(" * Detalles: ");
    Utilidades::colorReset();
  }
}

//Requiere:  -
//Retorna:   -
//Modifica:  -
void WebServidor::ejecutarServidor(){
  *conectado    = false;

  int pID = crearProceso(true);
  if(pID > 0){

      pID = crearProceso(true);
      if(pID > 0){

        // Maneja Control+C
        printf(" * (PRESIONE CTRL + C para cerrar el servidor de forma segura). \n");
        signal(SIGINT, controladorInterrupciones);

        while(*ejecutandose){
        }
      
      }else{
       
        while(*ejecutandose){
          while(*conectado){
      
            // Maneja Broadcast de Cierre
            char bufferBalanceadorRespuesta[64];
            int leidosBroadcast = this->broadcast->leerDe(bufferBalanceadorRespuesta, 64);
            bufferBalanceadorRespuesta[leidosBroadcast] = '\0';

            if (bufferBalanceadorRespuesta[0] == 'B' && bufferBalanceadorRespuesta[2] == 'D'){
              printf(" * Se finalizo la conexion con el balanceador, finalizando... \n"); 
              *ejecutandose = false;
            }

          }
        }
      
      }

      
  }else{

    // Maneja Broadcast de Apertura
    printf(" * Direccion IP Broadcast: %s \n", Utilidades::getBroadcastIP(Utilidades::recuperarSubnet(), 28).c_str());
    avisarBalanceadores(false);
    printf(" * Servidor en stand-by, esperando respuesta... \n");
    
    // Inicia el Servidor
    char bufferBalanceadorRespuesta[64];
    int leidosBroadcast = 0;
    while(*conectado == false){
      int leidosBroadcast = this->broadcast->leerDe(bufferBalanceadorRespuesta, 64);
      bufferBalanceadorRespuesta[leidosBroadcast] = '\0';
      if (bufferBalanceadorRespuesta[0] == 'B' && bufferBalanceadorRespuesta[2] == 'C'){
        printf(" * Servidor %s/ abierto.\n", "/" ROOT_FOLDER); 
        printf(" * Corriendo en: http://127.0.0.1:%d/\n", puertoRequests); 
        avisarBalanceadores(false);
        *conectado = true;
      }

    }
    
    // Proceso child, maneja los requests HTTP que le lleguen
    while(true){

      // Pero este a su vez, se mantendra en un while atendiendo las nuevas conexiones
      // Eventualmente el proceso de consola me matara, terminando el while(true)

      Socket* conexionEntrante = this->conexion.aceptar();
      int pID_peticion = crearProceso(false);

      if (pID_peticion == 0) {
        // Proceso nuevo, que maneja este Request nuevo y muere
        // No es necesario que me recuerden, muero al terminar
        // Creo mis propios buffers e instancias, y solo me intereso en mi request

        InterpreteHTTP  httpman;
        char            buffer[BUFFER_SIZE];        

        bool encontroFinal = false;
        while(!encontroFinal){
          conexionEntrante->leer(&(this->peticionParcial), BUFFER_SIZE, 0, true); 
          this->peticion += peticionParcial;
          if(this->peticion.find("\r\n\r\n") != string::npos){
            encontroFinal = true;
          }
        }
        
        // Construya los headers
        int estado = httpman.construirRespuesta(this->peticion);
        conexionEntrante->escribir(httpman.recuperarHeader());

        // Imprima los datos de este nuevo request
        auto reloj = chrono::system_clock::now();
        time_t reloj_fecha = chrono::system_clock::to_time_t(reloj);
        char direccion[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(conexionEntrante->in_IPv4.sin_addr), direccion, INET_ADDRSTRLEN);

        string tiempo = ctime(&reloj_fecha);
        tiempo = tiempo.substr(0, tiempo.size() - 1);
        printf(" * [%s] hacia [%s] [%d]\n", tiempo.c_str(), direccion, estado);

        // Envie los headers previamente construidos
        int bytesLeidos    = 0;
        int posicionBuffer = 0;
          
        while((bytesLeidos = httpman.recuperarBody(buffer, posicionBuffer, BUFFER_SIZE)) >0){
  	      conexionEntrante->escribir(buffer, bytesLeidos);
	        memset(buffer,0,BUFFER_SIZE);
		      posicionBuffer += bytesLeidos;
		    }

        conexionEntrante->apagar(2);
        conexionEntrante->cerrar();

        // Muero sin callear los destructores
        _exit(0);

      } 

    }

  }

  // Ahora que el proceso padre salio del while(ejecutandose), matare todos los PIDs que no sean yo mismo
  if(pID > 0){
    for(int i = 0; i < procesos.size(); i++){
      kill(procesos[i], SIGTERM);
    }
    if(broadcast != nullptr){   
      delete broadcast;
    }
  }  

}

//Requiere:  -
//Retorna:   -
//Modifica:  -
int main(int argc, char **argv){

  Utilidades::colorError();
  printf(" * Web Server Development Build \n");
  Utilidades::colorReset();

  int puertoClientes   = PORT_HTTP;
  int puertoServidores = PORT_BROADCAST;

  if(argc < 4){
    Utilidades::colorError();
    printf(" * Servidor %s/ fallo en abrir.\n", "/" ROOT_FOLDER); 
    Utilidades::colorReset();
    printf(" * Parametros insuficientes. Se necesitan tres parametros.\n", puertoRequests); 
    printf(" * Uso: ./bin/WebServidor <direccion-ip> <puertoHttp> <puertoBroadcast>\n");
    Utilidades::colorError();
    printf(" * Abortando el programa... \n");
    Utilidades::colorReset();
    exit(1);
  }
  puertoClientes = atoi(argv[2]);
  puertoServidores = atoi(argv[3]);
  webServidor = new WebServidor(puertoClientes, puertoServidores, argv[1]);
  webServidor->ejecutarServidor();
  delete webServidor;

  return 0;

}

