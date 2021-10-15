#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <ctype.h>
#include <netinet/in.h>

#include "Socket.hpp"

// Constructor mas comun para usar este socket
// usarStream: true para usar STREAM, falso para DATAGRAM
Socket::Socket(bool usarStream, bool protocoloV6){
   int dominio, tipo;

   memset(&in_IPv4, 0, sizeof(&in_IPv4));
   memset(&in_IPv6, 0, sizeof(&in_IPv6));
      
   this->stream               = usarStream;
   this->ipv6                 = protocoloV6;

   this->in_IPv4.sin_family   = AF_INET;
   this->in_IPv6.sin6_family  = AF_INET6;

   if(protocoloV6){ dominio = AF_INET6; }else{ dominio = AF_INET; }
   if(usarStream) { tipo = SOCK_STREAM; }else{ tipo = SOCK_DGRAM; }

   this->id = socket(dominio, tipo, 0);
   
}

// Asigna un timeout por defecto, para cerrar el Socket
void Socket::prepararTimeOut(){
   struct timeval socket_timeout;      
   socket_timeout.tv_sec  = 5;
   socket_timeout.tv_usec = 0;

   setsockopt(this->id, SOL_SOCKET, SO_RCVTIMEO, (char*) &socket_timeout, sizeof(socket_timeout));
   setsockopt(this->id, SOL_SOCKET, SO_SNDTIMEO, (char*) &socket_timeout, sizeof(socket_timeout));

}

// Constructor por defecto del socket, usa STREAM con IPv4
Socket::Socket(){
   this->stream   = true;
   this->ipv6     = false;

   this->in_IPv4.sin_family   = AF_INET;
   this->in_IPv6.sin6_family  = AF_INET6;

   this->id = socket(AF_INET, SOCK_STREAM, 0);
}

// Asegurese de terminar el socket al destruir la instancia
Socket::~Socket(){
    cerrar();
}

// Cierra el socket usando el llamado al sistema
void Socket::cerrar(){
   close(this->id);
}

// Conectese a una URL de un web-server
// host: direccion IP en hilera, puerto: canal para conectarse
int Socket::conectar(string hostv, int puerto) {

   const char* host = hostv.c_str();
   //printf("Conectandose al servidor web: %s en el puerto %d.\n", host, puerto);

   this->in_IPv4.sin_port           = htons(puerto);
   this->in_IPv4.sin_addr.s_addr    = inet_addr(host);
   this->in_IPv6.sin6_port          = htons(puerto);

   inet_pton(AF_INET , host, &in_IPv4.sin_addr );
   inet_pton(AF_INET6, host, &in_IPv6.sin6_addr);

   int estado = -1;
   if(!this->ipv6){
      estado = connect(this->id, (struct sockaddr* ) &in_IPv4, sizeof(in_IPv4));
   }else{
      estado = connect(this->id, (struct sockaddr* ) &in_IPv6, sizeof(in_IPv6));
   }

   return estado;
}

// Lectura de una hilera desde el socket
int Socket::leer(string* text, int len, int flag, bool isServer) {
   
   int estado = -1;
   char buffer[len];

   if (flag){
      estado = recv(this->id, buffer, len, flag);
   }else{
      estado = read(this->id, buffer, len);
   }
   
   (*text) = buffer;
   return estado;
}

// Lectura de una hilera desde el socket (version con char*)
int Socket::leer(char * text, int len, int flag, bool isServer) {
   
   int estado = -1;

   if (flag){
      estado = recv(this->id, text, len, flag);
   }else{
      estado = read(this->id, text, len);
   }

   return estado;
}

int Socket::leerDe(char* text, int len){
   size_t fuente_size = sizeof(this->in_IPv4);
   memset(&this->in_IPv4, 0, fuente_size);

   int estado = -1;
   estado = recvfrom(this->id, text, len, MSG_WAITALL, (struct sockaddr*)&this->in_IPv4 , (socklen_t*)&fuente_size);

   return estado;
}

int Socket::leerDe(string* text, int len){
   int estado;
   char buffer[len];
   size_t fuente_size = sizeof(this->in_IPv4);
   memset(&this->in_IPv4, 0, fuente_size);

   estado = recvfrom(this->id, buffer, len, MSG_WAITALL, (struct sockaddr*)&this->in_IPv4 , (socklen_t*)&fuente_size);
   
   (*text) = buffer;
   return estado;
}

// Escribe una hilera en el socket
int Socket::escribir(string text) {
   return write(this->id, text.c_str(), text.length());;
}

// Escribe una hilera en el socket (version con char*)
int Socket::escribir(const char * text,int len) {
   return write(this->id, text, len);
}

// Escribe una hilera en el socket (version con char*), pero orientado a datagram
int Socket::escribirA(const char* text, int len){
   return sendto(this->id, text, len, MSG_CONFIRM, (const struct sockaddr*)&in_IPv4, sizeof(in_IPv4));
}

// Escribe una hilera en el socket (version con string), pero orientado a datagram
int Socket::escribirA(string text){
   return sendto(this->id, text.c_str(), text.length(), MSG_CONFIRM, (const struct sockaddr*)&in_IPv4, sizeof(in_IPv4));
}

// Vincula un puerto al socket
int Socket::vincular(char* ip, int puerto) {

   in_IPv4.sin_port            = htons(puerto);
   if(ip == nullptr){
      in_IPv4.sin_addr.s_addr  = htonl(INADDR_ANY);
   }else{
      in_IPv4.sin_addr.s_addr  = inet_addr(ip);
   }

   in_IPv4.sin_addr.s_addr  = htonl(INADDR_ANY);
   in_IPv6.sin6_port        = htons(puerto);

   int reusarPuerto = 1; 
   setsockopt(this->id, SOL_SOCKET, SO_REUSEPORT, &reusarPuerto, sizeof(reusarPuerto));

   int estado = -1;
   if(!this->ipv6){
      estado = bind(this->id, (struct sockaddr* ) &in_IPv4, sizeof(in_IPv4));
   }else{
      estado = bind(this->id, (struct sockaddr* ) &in_IPv6, sizeof(in_IPv6));
   }

   return estado;
}

// Vincula un puerto a una direccion IP para broadcast:
int Socket::vincularBroadcast(const char* ip, int puerto, bool soyServidor)
{

   in_IPv4.sin_family      = AF_INET;
   in_IPv4.sin_port        = htons(puerto);
   in_IPv4.sin_addr.s_addr = inet_addr(ip);

   int reusarPuerto = 1;
   if (setsockopt(this->id, SOL_SOCKET, SO_REUSEPORT, &reusarPuerto, sizeof(reusarPuerto)) < 0){
      return -1;
   }
   if (setsockopt(this->id, SOL_SOCKET, SO_BROADCAST, &reusarPuerto, sizeof(reusarPuerto)) < 0){
      return -1;
   }

   int estado = 0;
   if(soyServidor){
      estado = bind(this->id, (struct sockaddr*)&in_IPv4, sizeof(in_IPv4));
   }
   
   return estado;
}

// Permite escuchar nuevas conexiones
// Debe indicar el limite en cola
int Socket::escuchar(int cola) {
   return listen(this->id, cola);
}

// Acepta la siguiente nueva conexión entrante 
Socket* Socket::aceptar(){

   Socket* nuevoSocket = new Socket(this->stream, this->ipv6);
   socklen_t socklen = 0;

   int id_nuevo = -1;
   if(!this->ipv6){
      socklen = sizeof(in_IPv4);
      id_nuevo = accept(this->id, (struct sockaddr*) &nuevoSocket->in_IPv4, &socklen);
   }else{
      socklen = sizeof(in_IPv6);
      id_nuevo = accept(this->id, (struct sockaddr*) &nuevoSocket->in_IPv6, &socklen);
   }

   if(-1 == id_nuevo){
      return nullptr;
   }
   nuevoSocket->asignarID(id_nuevo);

   return nuevoSocket;
}

// Cierre del socket de un modo especifico
// SHUT_RD, SHUT_WR, SHIT_RDWR (0, 1, 2) respectivamente
int Socket::apagar(int modo){
   return shutdown(this->id, modo);
}

// Asigna un ID al socket. Usar despues de crear un nuevo socket
void Socket::asignarID(int id_nuevo){
   this->id = id_nuevo;
}

