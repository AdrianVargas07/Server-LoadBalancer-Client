#ifndef UTILIDADES_HEADER
#define UTILIDADES_HEADER
#include <vector>
#include <string>
#include <cstring>
#include <regex>
#include <fstream>
#include <iomanip>
#include <bitset>
#include <math.h>
using namespace std;
class Utilidades{
    public:
        static void prepararRandom();
        static vector<string> split(string linea, string delimitador, int veces);
        static string split(string* linea, string delimitador);
        static string split(char* linea, const char* delimitador, int* posicion);
        static void obtenerNuevasRutas(vector<string>* rutas, string* rutaActual, string* cuerpoHTML);
        static string extraerValorHeader(string header, string clave);
        static void colorError();
        static void colorReset();
        static int getNumeroRandom(int minimo, int maximo);
        static string getFecha();
        static string getFechaGMT();
        static string getFechaGMT(struct timespec fecha);
        static void replacePlaceHolder(string regexString, string& text, string replacementString);
        static char* recuperarSubnet();
        static string leerArchivo(string ruta);
        static string getBroadcastIP(string ip, int mascara);
        static void* crearMemoriaCompartida(size_t size);
        
        
};
#endif //UTILIDADES_HEADER
