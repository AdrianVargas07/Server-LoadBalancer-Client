#include "Utilidades.hpp"
#include <WebCliente.hpp>
#include <chrono>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

//Requiere: Una linea para dividir, un delimitador
//Retorna:  Devuelve un grupo de hileras spliteadas, hasta un maximo de n - veces
//Modifica: -
vector<string> Utilidades::split(string line, string delimitador, int veces){
    vector<string> hileras;

	if(line.size() > 0){
		int    contador    = 0;
    	size_t posicion    = 0;
    
    	while (contador < veces+1) {
        	posicion = line.find(delimitador);
			if (posicion == string::npos)
				break;
			string subparte = line.substr(0, posicion);
        	hileras.push_back(subparte);
        	line.erase(0, posicion + delimitador.length());
        	contador++;
    	}
		if(line.compare("") != 0){
    		hileras.push_back(line);
		}
	}
    
    return hileras;
    
}

//Requiere: Una linea para dividir, un delimitador
//Retorna:  Devuelve una hilera con la hilera antes del corte.
//Modifica: Line
string Utilidades::split(string* line, string delimitador){
    size_t posicion    = 0;
	string subparte = *line;

    posicion = line->find(delimitador);
	if(posicion == string::npos)
	{
		*line = "";
	}
	else{
    	subparte = line->substr(0, posicion);
    	line->erase(0, posicion + delimitador.length());
	}
    return subparte;
}

//Requiere: Una linea para dividir, un delimitador y una variable que recuerda la nueva posicion
//Retorna:  Devuelve una hilera con la hilera antes del corte
//Modifica: Line, y Pos
string Utilidades::split(char* line, const char* delimitador, int* pos){

	char bufferTemporal[BUFFER_SIZE + 1];
	memcpy(bufferTemporal, line, BUFFER_SIZE);

    char* ptr = strstr(bufferTemporal, delimitador); 

	*pos = int(ptr - bufferTemporal) + 4;
	int diferencia = BUFFER_SIZE - *pos;
	memcpy(line, &bufferTemporal[*pos], diferencia);
	line[diferencia] = '\0';
	bufferTemporal[*pos] = '\0';

	return string(bufferTemporal);
}

//Requiere: Un grupo de headers completos y un atributo
//Retorna:  El valor del atributo solicitado
//Modifica: -
string Utilidades::extraerValorHeader(string header, string clave){

	auto posicion = header.find(clave);

	if(posicion == string::npos){
		return "";
	}

	posicion += clave.length();

	int posicionCierre_1 = header.find("\r\n", posicion);
	int posicionCierre_2 = header.find(";", posicion);

	int size = std::min(posicionCierre_1 - posicion, posicionCierre_2 - posicion);
	return header.substr(posicion, size);
}

// Esta funcion no es para este entregable, esta en desarrollo y servira para el futuro
void Utilidades::obtenerNuevasRutas(vector<string>* rutas, string* ruta, string* cuerpo){
	int sum;
	auto pun = cuerpo->find("src");
    
	if (pun==std::string::npos){
		auto pos = cuerpo->find("link");
		if (pos!=std::string::npos){
			pun = cuerpo->find("href",pos);
		}
	}

	while(pun!=std::string::npos){
		*cuerpo = cuerpo->substr(pun);
		pun = cuerpo->find("”");
		auto simp = cuerpo->find("'");
		sum = 3;
		if(simp < pun){
			pun = simp;
			sum = 1;
		}
		*cuerpo = cuerpo->substr(pun+sum);
		auto fin = cuerpo->find("”");
		 simp = cuerpo->find("'");
		 sum = 3;
		if(simp<fin){
			fin = simp;
			sum = 1;
		}
		string objeto = cuerpo->substr(0,fin);
		auto puntos = objeto.find(*ruta);
		if (puntos == std::string::npos){
			objeto =*ruta + objeto;
		}
		rutas->push_back(objeto);
		*cuerpo=cuerpo->substr(fin);
		pun = cuerpo->find("src");
		if (pun==std::string::npos){
			auto pos = cuerpo->find("link");
			if (pos!=std::string::npos){
				pun = cuerpo->find("href",pos);
			}
		}
		
	}

}

void Utilidades::colorError(){
  printf("\033[31m");
}

void Utilidades::colorReset(){
  printf("\033[0m");
}

int Utilidades::getNumeroRandom(int minimo, int maximo){
	return minimo + (rand() % (maximo - minimo + 1));
}

void Utilidades::prepararRandom(){
	srand (time(NULL)); 
}

string Utilidades::getFecha(){
	auto reloj         = chrono::system_clock::now();
    time_t reloj_fecha = chrono::system_clock::to_time_t(reloj);
    char direccion[INET_ADDRSTRLEN];
    string tiempo = ctime(&reloj_fecha);
    tiempo = tiempo.substr(0, tiempo.size() - 1);
	return tiempo;
}

string Utilidades::getFechaGMT(){
	time_t horaActual;
    struct tm temporal;
    struct tm *horaEnGMT;

    time(&horaActual);
    horaEnGMT = gmtime_r(&horaActual, &temporal);

    char buffer [80];
    strftime(buffer, 100, "%a, %d %b %Y %T %Z", horaEnGMT);

    return string(buffer);
}

string Utilidades::getFechaGMT(struct timespec fecha){
	time_t horaDeModificacion;
    struct tm temporal;
    struct tm *horaModificacionGMT;

    horaModificacionGMT = gmtime_r(&fecha.tv_sec, &temporal);
    char buffer[80];

    strftime(buffer, 100, "%a, %d %b %Y %T %Z", horaModificacionGMT);
	return string(buffer);
}

char* Utilidades::recuperarSubnet(){
	char* ip = new char[32];
	string ipArchivo = Utilidades::leerArchivo("network.config");
    strcpy(ip, ipArchivo.c_str());
    return ip;
	//return (char*) "172.16.123.48"; //
}

void Utilidades::replacePlaceHolder(string placeHolder, string& text, string replacementText){
	regex placeHolderRegex(placeHolder);
	text = regex_replace(text, placeHolderRegex, replacementText);
}

string Utilidades::leerArchivo(string ruta){
	string linea;
	string doc;
	ifstream archivo(ruta);

	if(archivo.is_open()){
		while(getline(archivo, linea)){
			doc += linea;
		}
		archivo.close();
	}

	return doc;
}

string Utilidades::getBroadcastIP(string ip, int mascara){
	
	in_addr_t direccion_local = htonl(inet_addr(ip.c_str()));
	
	int complemento = 32 - mascara;
	int suma = 0;
	for(int i = 0; i < complemento; ++i)	{
		suma += pow(2, i);
	}
	
	int mascara_red = ~suma;
	in_addr_t direccion_subnet = 0;
	direccion_subnet = (uint32_t)(mascara_red) & direccion_local;
	struct sockaddr_in s;

	direccion_subnet += pow(complemento, 2) - 1;
	direccion_subnet = ntohl(direccion_subnet);

	s.sin_addr.s_addr = direccion_subnet;
	return string(inet_ntoa(s.sin_addr));
}

// Crea memoria compartida de size de largo
void* Utilidades::crearMemoriaCompartida(size_t size) {
  return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}