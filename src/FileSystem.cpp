#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include "FileSystem.hpp"

using namespace std;

//Constructor por defecto de la clase
FileSystem::FileSystem(){
}

//Destructor de la clase
FileSystem::~FileSystem(){
    closedir(directorio);
    close(fichero);
}

//Require: void
//Retorna: nombre del archivo
//Modifica: void
string FileSystem::getNombreArchivo(){
    return nombreArchivo;
}

//Require: void
//Retorna: ruta de directorios
//Modifica: void
string FileSystem::getRutaDirectorios(){
    return rutaDirectorios;
}

//Requiere:
//Retorna: string con ultima fecha de modificacion de nombreArchivo
//Modifica:
struct timespec FileSystem::getFechaMod(){
    return fechaModificacion;
}

//Requiere:
//Retorna: tamano del nombreArchivo en bytes
//Modifica:
off_t FileSystem::getTamanoArchivo(){
    return tamanoArchivo;
}

//Requiere:
//Retorna: estenseion de nombreArchivo
//Modifica:
string FileSystem::getExtension(){
    return extension;
}


//Requiere: ruta de directorios/recurso que se desea solicitar
//Retorna: void
//Modifica: Este metodo se encarga de recibir una solicitud, la procesa y se encarga de guardarla en el atributo rutaDirectorios en el 
//formato requerido para poder, en otro metodo, revisar si la ruta de directorio existe o no. Si la string que se recibe trae el nombre del recurso
//requerido, de igual forma en rutaArchivo solo se guarda la ruta de directorios, el nombre del recurso se almacenaria en el atributo nombreArchivo
void FileSystem::setRuta(string ruta){

    int contador = 0;
    int posicion = 0;
    rutaDirectorios = "";
    directorios.clear();
    bool directorioExiste = true;
    string rutaAbsoluta = ruta;
    string rutaCorrecta="";
    string separador  = "/";
    string dir = "";

    //se revisa el primer caracter de ruta, si es un "/", se elimina
    if( rutaAbsoluta[0] == '/')
        rutaAbsoluta.erase(0,1);

    //Se revisa si el ultimo caracter de la ruta es un "/", sino lo es, se agrega pues si no la posee la string se procesaria mal en el siguiente paso
    if( rutaAbsoluta[rutaAbsoluta.size()-1] != '/')
        rutaAbsoluta += '/';

    //linea necesaria por si se le hace una correcion de "/" en las lineas anteriores, no se pierda el cambio en el while donde se procesa la ruta
    rutaCorrecta = rutaAbsoluta;

    //busca en el parametro ruta el separador "/", cada vez que encuentra uno, guarda en una casilla del vector "directorios" el nombre de lo que deberia ser un
    //directorio y quiza, quede guardado en la ultima posicion un directorio o nombre de archivo
    while ((posicion = rutaAbsoluta.find(separador)) != string::npos) {
        dir = rutaAbsoluta.substr(0, posicion);
        directorios.push_back(dir);
        rutaAbsoluta.erase(0, posicion + separador.length());
        contador++;
    }

    //se asigna de nuevo porque en el proceso anterior se destruyo rutaAbsoluta
    rutaAbsoluta = rutaCorrecta;

    if(esDirectorio()){
        setNombre(rutaAbsoluta, true);
    }else{
        setNombre(rutaAbsoluta,false);
    }

   
    //se intenta abrir la ruta que se recibio
    directorio = opendir( rutaDirectorios.c_str() ); //se intenta abrir la ruta de directorios guardada en rutaDirectorios
        // if (NULL == directorio) {
        //     perror("Error al abrir directorios:");
        //     exit(0);
        // }//fichero
   
    if(directorio != NULL){

        string rutaParaAbrirArchivo = rutaDirectorios + nombreArchivo;

        fichero = open ( rutaParaAbrirArchivo.c_str(), O_RDWR );

        //se llama para el siguiente metodo para guardar los datos del archivo pues lo podrian necesitar otros metodos.
        getInformacionArchivo();

        bytesFaltantes = getTamanoArchivo();
            // if (fichero == -1) {
            //     perror("Error al abrir fichero:");
            //     exit(0);
            // }
    }
}

// Requiere: ruta absoluta del directorio que se solicita | si la ruta original es solo directorios o si incluia nombre de archivo
// Retorna: void
// Modifica: Este metodo (que es llamado por el metodo setRuta), se encarga de guardar en los atributos correspondientes, la ruta de directorios que se le 
// pasa como parametro (en rutaDirectorios). Si se incluye  junto a la ruta el nombre del archivo, se guarda donde corresponde (nombre archivo) el nombre
// del archivo
void FileSystem::setNombre(string ruta, bool esDirectorio){
    
    //caso en el que solo una lista de directorios
    if(esDirectorio){       
        rutaDirectorios = ruta;
        nombreArchivo = "";

    //caso en el que son directorios y en la ultima casilla es el nombre del archivo   
    }else{

        for(int i=0; i<directorios.size()-1; i++){

            rutaDirectorios += directorios[i]+"/";
        }            
            nombreArchivo = directorios[directorios.size()-1];
    }
}

//Requiere: void
//Retorna: void
//Modifica: este metodo se encarga de crear directorios dentro de otros directorios.
void FileSystem::crearDirectorios(string rutaDirPorCrear){
    
    int contador = 0;
    int posicion = 0;
    bool directorioExiste = true;
    string rutaAbsoluta = rutaDirPorCrear;
    string  carpetaAnidada = "";
    vector<string>  dirPorCrear; 
    string separador  = "/";
    string dir = "";
    int id = 0;

    //se revisa el primer caracter de ruta, si es un "/", se elimina
    if( rutaAbsoluta[0] == '/')
        rutaAbsoluta.erase(0,1);

    //Se revisa si el ultimo caracter de la ruta es un "/", sino lo es, se agrega pues si no la posee la string se procesaria mal en el siguiente paso
    if( rutaAbsoluta[rutaAbsoluta.size()-1] != '/')
        rutaAbsoluta += '/';

    //busca en el parametro ruta el separador "/", cada vez que encuentra uno, guarda en una casilla del vector "directorios" el nombre de lo que deberia ser un
    //directorio y quiza, quede guardado en la ultima posicion un directorio o nombre de archivo
    while ((posicion = rutaAbsoluta.find(separador)) != string::npos) {
        dir = rutaAbsoluta.substr(0, posicion);
        dirPorCrear.push_back(dir);
        rutaAbsoluta.erase(0, posicion + separador.length());
        contador++;
    }

    // se guarda el nombre de la primer carpeta que esta en  el vector
    carpetaAnidada = dirPorCrear[0];
    
    //es necesario saber si se creara una solo carpeta (en ese caso entra en el else) o se crearan unas carpetas dentro de otras (en ese caso entra en el if)
    if (dirPorCrear.size() > 1){

        //para controlar el total de carpetas a crear
        for (int i = 1; i <= dirPorCrear.size(); i ++){

            //se revisa si la carpeta que se quiere crear ya existe o no
            if( ! opendir(carpetaAnidada.c_str() ) ){

                //en caso de que no exista la carpeta, se crea la carpeta
                id = mkdir( carpetaAnidada.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );

                if (-1 == id){
                    perror ("Error creando directorios:\n");
                }

                //if para asegurarse de no intentar incluir una carpeta que no existe (puede pasar en la ultima iteracion del for)
                if(i < dirPorCrear.size())
                    carpetaAnidada += "/"+dirPorCrear[i];

            }else{//en el caso de que la carpeta ya existiera, no se crea y se adjunta la siguiente carpeta
                
                if(i < dirPorCrear.size())
                    carpetaAnidada += "/"+dirPorCrear[i];  
            }            
            
        }

    }else{//caso en el que solo hay que crear una carpeta sin otras carpetas anidadas

        //se revisa si la carpeta que se quiere crear ya existe o no
        if( ! opendir(carpetaAnidada.c_str() ) ){

            //en caso de que no exista la carpeta, se crea la carpeta
            id = mkdir( carpetaAnidada.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH );

                if (-1 == id){
                    perror ("Error creando directorios:\n");
                }
        }
    }

}

//Requiere: void
//Retorna: void
//Modifica: crea un archivo en la ruta asignada
void FileSystem::crearArchivo(){

    string archivo = rutaDirectorios + nombreArchivo;

    //se crea archivo con permiso de escritura
    file = fopen(archivo.c_str(), "w+");

    if (file == NULL) 
        perror ("Error creando file");

}

//Requiere: void
//Retorna: void
//Modifica: guarda extension de archivo en la variable extension
void FileSystem::extensionArchivo(){

    int posicion = 0;
    string nombre = nombreArchivo;

    posicion = nombre.find('.');//se busca el punto (dado que es el separador de la extension)
    extension = nombre.erase(0, posicion+1);

    //se convierte la extension de mayusculas a minusculas entrada por entrada
    for (int i = 0; i < extension.length(); i++) {
        extension[i] = tolower(extension[i]);
    }
    
}

//Requiere: buffer donde se guardan los datos, posicion donde hay que empezar a leer el archivo, len la cantidad de datos por leer
//Retorna: el numero de bytes que se han leido
//Modifica: lee la cantidad de bytes especificados en bytesPorLeer
int FileSystem::leerArchivo(char* buffer, int posicion, int len){    
    
    bytesFaltantes = bytesFaltantes - len;
    
    //se debe saber la cantidad de bytes que queda por leer, pues si es el final de archivo o una cantidad menor mas pequeño que el enviado por len,
    //se debe saber cuando es exactamente para no incluir basura.
    //caso en el que los bytesPorLeer
    if(bytesFaltantes > 0){
        bytesPorLeer = len;
    }else{
        bytesPorLeer = getTamanoArchivo()-posicion;
    }

    //se posiciona el puntero en el lugar correspondiente
    lseek(fichero, posicion , SEEK_SET);
    
    //se leen los bytes
    bytesLeidos = read(fichero, buffer, bytesPorLeer);

    return bytesLeidos;
}

//Requiere: buffer donde se escribiran los datos | la posicion desde donde hay que escribir | tamano de lo que hay que leer
//Retorna:
//Modifica:
bool FileSystem::escribirArchivo(const char* buffer, int posicion, int len){

    //se posiciona el puntero en el lugar correspondiente
    lseek(fichero, posicion , SEEK_SET);
    
    //se escriben los bytes correspondientes
    bytesEscritos = write(fichero, buffer, len);

    return bytesEscritos;
}

//Recibe: void
//Retorna: se revisa la ultima string que venia en la hilera de la solicitud para saber si solo una lista de directorios o si es la lista de directorios junto
//con el nombre del archivo. Si es solo directorios retorna true, en el otro caso retorna false
//Modifica: void
bool FileSystem::esDirectorio(){

    bool esDirectorio=true;
    string dato = directorios[directorios.size()-1];// se toma el ultimo dato guardado en el vector

    //se analiza si "dato" posee un ".", es decir, para saber si posee algun tipo de extension(archivo) o si corresponde al nombre de un directorio (sin extension)
    for(int i= 0; i<dato.length(); i++){

        if(dato[i] == '.'){
            esDirectorio=false;

        }
    } 
    return esDirectorio;
}

//Requiere: void
//Retorna: si revisa si lo que se guardo en rutaDirectorios corresponde a directorios que realmente existen. En caso afirmativo se retorna true, en caso contrario
//se retorna false 
//Modifica: void
bool FileSystem::existeDirectorio(){

    bool existeDirectorio = true;

    //si se retorno NULL, la ruta de directorios no existe, en caso contrario, la ruta existe
    if(directorio == NULL)
        existeDirectorio = false;       

    return existeDirectorio;
}

//Requiere: void
//Retorna: se revisa si el nombre del archivo guardado en nombreArchivo corresponde a un archivo existente. En caso afirmativo se retorna true, en caso contrario false
//Modifica: void
bool FileSystem::existeArchivo(){

    bool existeArchivo=false;

    //si se retorno un puntero al flujo de directorio se revisan, de lo contrario se retorna false
    if(directorio != NULL && nombreArchivo != "") {

        //se lee cada entrada que posee el directorio guardado en rutaDirectorios, si el nombre de alguna de esas entradas coincide con el nombre almacenado en
        //nombreArchiO_WRONLY, S_IRWXUvo, se retorna true, en caso contrario se retorna false
        while((elemento = readdir(directorio)) != NULL ){
            if ( strcmp( elemento->d_name, nombreArchivo.c_str()) == 0)
                existeArchivo = true;
        }

        // este metodo resitua el puntero, es decir, vuelve a poner el puntero al inicio de la carpeta que se accede en setRuta, esto es por si algun otro metodo
        //decide utilizar readdir
        rewinddir(directorio); 

    }    
 
    return existeArchivo;
}

//Requiere:
//Retorna:
//Modifica:
bool FileSystem::tienePermisoAcceso(){
    // Esto requiere del uso del .htaccess
    return true;
}

//Requiere: void
//Retorna: retorna la ruta del directorio actual desde el directorio raiz de la computadora
//Modifica: void 
string FileSystem::getDirectorioActual(){

    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    return string(cwd);

}


//Requiere: void
//Retorna: void
//Modifica: busca el archivo dentro del directorio correspondiente, si lo encuentra, guarda en la estructura correspondiente los datos del archivo
void FileSystem::getInformacionArchivo(){

    bool seguirBuscando=true;
    string ruta = rutaDirectorios + nombreArchivo;

    //si se pudo abrir el fichero, se recuperan los datos
    if ( -1 != fichero ){

        stat(ruta.c_str(),&buffer);//se guardan los datos del directorio en la estructura
        fechaModificacion =  buffer.st_mtim;
        tamanoArchivo = buffer.st_size;
        seguirBuscando = false;       

    }else{
        tamanoArchivo = 0;
    }
  
}

//Requiere: void
//Retorna: lista de directorios/archivos
//Modifica: se guarda en una lista el nombre de los directorios y archivos, que se encuentran en la variable rutaDirectorios
list<string> FileSystem::listarSubdirectorios(){
    list<string> dir;

    //si se retorno un puntero al flujo de directorio se revisan, de lo contrario se retorna false
    if ( directorio != NULL){

        //se guarda en una lista el nombre de todos los directorios o archivos que se encuentran bajo la ruta almacenada en rutaDirectorios
        while((elemento = readdir(directorio)) !=  NULL){

            //si el nombre de la entrada que esta en elemento es distinta del directorio actual o el directorio padre, se guarda
            if( strcmp (elemento->d_name, ".") != 0 && strcmp (elemento->d_name, "..") != 0)
            dir.push_back( elemento->d_name);
        }

        rewinddir(directorio); // este metodo reinicia el puntero, es decir, vuelve a poner el puntero al inicio de la carpeta que se accede en setRuta

    }

    return dir;
}