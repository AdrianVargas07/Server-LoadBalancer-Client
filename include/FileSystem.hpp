#ifndef FS_HEADER
#define FS_HEADER

#include <string>
#include <vector>
#include <list>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;

class FileSystem{

    private:

        string          rutaDirectorios;
        string          nombreArchivo;
        string          extension;
        vector<string>  directorios; 
        DIR            *directorio = nullptr;
        struct dirent  *elemento;
        struct stat     buffer;
        int             fichero;
        FILE           *file;

        //atributos del archivo
        struct timespec fechaModificacion;
        off_t           tamanoArchivo;

        //atributos para lectura del archivo
        int             bytesLeidos;
        int             bytesPorLeer;
        int             bytesFaltantes;
        int             bytesEscritos;
        
    public:
        FileSystem();
        ~FileSystem();

        void            setRuta(string); 
        void            setNombre(string ruta, bool esDirectorio);

        string          getNombreArchivo();
        string          getRutaDirectorios(); 
        struct timespec getFechaMod(); 
        off_t           getTamanoArchivo();
        string          getExtension();
        string          getDirectorioActual();
        void            getInformacionArchivo(); 

        void            crearArchivo();
        void            crearDirectorios(string rutaDirPorCrear);
        void            extensionArchivo();
        int             leerArchivo(char* buffer, int posicion, int len);
        bool            escribirArchivo(const char* buffer, int posicion, int len);
        bool            esDirectorio();
        bool            existeDirectorio();  
        bool            existeArchivo();
        bool            tienePermisoAcceso();
        list<string>    listarSubdirectorios();

        #ifdef DEBUG
            DIR* getDirectorio(){return this->directorio;}
        #endif
    
};

#endif



