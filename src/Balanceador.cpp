#include "Balanceador.hpp"

// STATIC CLASS VARIABLES
const string Balanceador::CRLF = "\r\n";
const string Balanceador::DOUBLECRLF = "\r\n\r\n";
const string Balanceador::HTTP_VERSION_1 = "HTTP/1.0";
const string Balanceador::HTTP_VERSION_11 = "HTTP/1.1";

// GLOBAL VARIABLES
Balanceador* balanceador;

//Requiere: un metodo de balanceo, y los puertos a usar
//Retorna: - 
//Modifica: prepara las conexiones del balanceador y los archivos de log
Balanceador::Balanceador(int algoritmo, char* ip, int puerto_cliente_param, int puerto_servidor_param){

    // Defina las variables
    this->puerto_cliente    = puerto_cliente_param;
    this->puerto_servidor   = puerto_servidor_param;
    this->algoritmoBalanceo = algoritmo; 
    this->direccionHost     = ip;
    this->ejecutandose      = true;
    this->filePosition      = 0;

    // Busco si existe la carpeta /log
    fs.setRuta("log");
    if(!fs.existeDirectorio()){
        fs.crearDirectorios("log");
    }

    // Prepare el hilo que se encargara de manejar el sistema de reports
    char buffer[64];
    sprintf(buffer, "log/report %s.log\n", Utilidades::getFecha().c_str());

    // Cree los archivos del reporte
    fs.setRuta(buffer);
    fs.crearArchivo();
    fs.setRuta(buffer);

    // Inicio del programa
    logRed(" * Load Balancer Development Build\n"); 
}

//Requiere: -
//Retorna:  -
//Modifica: La cola de mensajes llamada "log_mensajes"
Balanceador::~Balanceador(){
    balanceador->manejarBitacora();
}

//FUNCIONES ESTATICAS

//Requiere: El valor de la interrupcion
//Retorna:  -
//Modifica: Ingresa los datos correspondientes a la bitacora
void controladorCtrlC(int interrupcion){
    balanceador->log("\n");
    balanceador->ejecutandose = false;
    balanceador->avisarServidores(true);
    balanceador->escuchador_http->apagar(2);
    balanceador->escuchador_http->cerrar();
    balanceador->escuchador_broadcast->apagar(2);
    balanceador->escuchador_broadcast->cerrar();

    
}

//Requiere: El valor de la interrupcion
//Retorna:  -
//Modifica: Metodo que evita que se caiga el balanceador cuando un socket se cierra de forma inesperada
void controladorBrokenPipe(int interrupcion){

}

//Requiere: El valor del puerto
//Retorna:  -
//Modifica: Ingresa cambios en el log sobre errores en la vinculacion/escucha de puertos
void imprimirErrorEscucha(int puerto){
    balanceador->logRed(" * Acceso denegado al puerto ");
    balanceador->logRed(to_string(puerto));
    balanceador->logRed(", intente con otro puerto. \n");
    balanceador->log(" * Uso: ./bin/Balanceador <puertoHttp> <puertoBroadcast> \n");
    balanceador->logRed(" * Abortando el programa... \n");
    balanceador->ejecutandose = false;
    balanceador->manejarBitacora();
    _exit(1);
}

//Requiere: -
//Retorna:  -
//Modifica: El metodo de balanceo que se desee modificar en tiempo de ejecución del balanceador
void hilo_Input(){
    balanceador->log(" * (Escriba metodo <valor> para cambiar el metodo de balanceo)  \n");
    string argumento;
    
    // Se revisa si el metodo de balanceo que se escoge mientras el balanceador este en ejecución, es uno de los 4 que se pueden escoger,
    // de lo contrario, se notifica al usuario
    while(balanceador->ejecutandose){
        getline(std::cin,argumento);
        string funcion=Utilidades::split(&argumento," ");
        if(funcion.compare("metodo") == 0){
            int num = atoi(argumento.c_str());

            if(argumento.compare("") != 0 && argumento.length() > 0 && num >= 0 && num <= 3){
                balanceador->setAlgoritmoBalanceo(atoi(argumento.c_str()));
            }
            else
            {
                balanceador->log(" * Metodo de balanceo invalido.\n");
            }
        }
        else{
            balanceador->log(" * Comando invalido.\n");
        }
        
    }

}

//Requiere: -
//Retorna:  -
//Modifica: La cola de mensajes
void hilo_Output(){

    // Si a la hora de crearse, ya se cancelo el programa, no ejecute el primer LOG de Control+C
    if(balanceador->ejecutandose){
        balanceador->log(" * (PRESIONE CTRL + C para cerrar el balanceador de forma segura). \n");
        signal(SIGINT, controladorCtrlC);
        signal(SIGPIPE, controladorBrokenPipe);
        balanceador->mostrarEstadisticas();
        while(balanceador->ejecutandose){
            balanceador->manejarBitacora();
        }      
    } 
}  

//Requiere: El socket con el que se ha establecido al conexion con el cliente
//Retorna:  -
//Modifica: vector de servidores | vector de clientes                                  
void hilo_Cliente(Socket* socket_cliente){

    char httpRequest[4096];
    char httpResponse[4096];
    int size, value, bytes, encontrado, sent, indice_ip = 0;

    Socket* socket_servidor = nullptr;

    // si al balanceador le llega una solicitud y el balanceador aun no ha agregado ningun servidor, le notifica al cliente que no hay
    // servicio disponible y cierra la conexion
    if(balanceador->servidores.empty()){
        balanceador->log( " * La lista de servidores está vacía. \n" );

        // Se recupera el header y el cuerpo del mensaje que se le retornará al cliente
        socket_cliente->escribir(balanceador->recuperarHeader(503));
        if( -1 != balanceador->recuperarBodyPaginaError(httpResponse, 4096, 503)){
            socket_cliente->escribir(httpResponse);
        }

        socket_cliente->apagar(2);
        socket_cliente->cerrar();
        return;
    }

    // Se lee el esquema de balanceo que se desea utilizar
    switch (balanceador->algoritmoBalanceo.load()){ 

    case balanceador->ALGORITMO::ROUND_ROBIN:
        
        // Se toman los datos correspodientes para el caso de roundRobin, se bloquean los vectores que podrían ser modificados al mismo
        //tiempo por varios hilos y se modifican las estadisticas
        {
            unique_lock<mutex> guardRoundRobin(balanceador->mutex_roundRobin);
            unique_lock<mutex> guardServidores(balanceador->mutex_servidores); 
            value = balanceador->roundRobin_Actual;
            indice_ip = value;  
            size = balanceador->servidores.size();
            value++;
            if(value >= balanceador->servidores.size()){
                value = 0;
            }
            balanceador->roundRobin_Actual = value;
        }     

        break;

    case balanceador->ALGORITMO::ROUND_ROBIN_PESOS:

        // Se toman los datos correspodientes para el caso de roundRobin con pesos, se bloquean los vectores que podrían ser modificados
        // al mismo tiempo por varios hilos y se modifican las estadisticas
        {
            lock_guard<mutex> lockRoundRobin(balanceador->mutex_roundRobin);
            unique_lock<mutex> lockservidores(balanceador->mutex_servidores); 
            value = 0; 
            size  = balanceador->servidores.size();
            while(balanceador->servidores[value].carga_actual > balanceador->servidores[value].carga_maxima){
                value++;
                if(value >= balanceador->servidores.size()){
                    value = 0;
                }
            }   
            indice_ip = value;
        }

        break;

    case balanceador->ALGORITMO::DIRECCION_CLIENTE:
        
        // Se busca si el cliente que solicita un recurso ya se ha conectado anteriormente a alguno de los servidores registrados
        encontrado = balanceador->buscarCliente(socket_cliente);        

        // Si no lo encuentra se agrega el cliente a un servidor escogido aleatoriamente
        if(encontrado == -1){      
           balanceador->agregarCliente(socket_cliente);
        }
        
        break;

    case balanceador->ALGORITMO::CONEXIONES_ACTIVA:
        //se guarda la ip del servidor que tiene menos conexiones activas en un determinado momento y se almacena su ip
        indice_ip = balanceador->buscarServidorConMenorCarga();
        break;  
    }

    //se controla el acceso a la variable que maneja las estadisticas y se modifica
    {
        unique_lock<mutex> guardServidores(balanceador->mutex_servidores); 
        balanceador->servidores[indice_ip].carga_actual++;
    } 

    //se crea un nuevo socket ipv4 que se conectara con la ip y puerto del servidor que se ha escogido en el switch anterior 
    socket_servidor = new Socket(true, false);
    if(-1 == socket_servidor->conectar(string(balanceador->servidores[indice_ip].ip), balanceador->servidores[indice_ip].port)){
        perror("IP Invalida, revise network.config");
        socket_cliente->apagar(2);
        socket_servidor->apagar(2);
        socket_cliente->cerrar();
        socket_servidor->cerrar();
        delete socket_cliente;
        delete socket_servidor;
        return;
    }

    // Se recibe la solicitud del cliente por el socket correspondiente y se envia al servidor indicado
    bool encontroFinal = false;
    while(!encontroFinal){
          bytes = balanceador->recibirMensaje(httpRequest, 4096, socket_cliente);
          sent = balanceador->enviarMensaje(httpRequest, bytes, socket_servidor);
          string buffer(httpRequest);
          if(buffer.find("\r\n\r\n") != string::npos){
            encontroFinal = true;
        }
    }
        
    bytes = 0;

    //se lee el mensaje devuelto por el servidor y se reenvia al cliente que ha hecho la solicitud
    while((bytes = balanceador->recibirMensaje(httpResponse, 4096, socket_servidor)) > 0){
        balanceador->enviarMensaje(httpResponse, bytes, socket_cliente);
    }
    
    //se modifican la estadisticas
    {
        unique_lock<mutex>lockservidores(balanceador->mutex_servidores);
        balanceador->servidores[indice_ip].carga_actual--;
    }

    //se apaha y cierra los sockets del cliente y del servidor
    socket_cliente->apagar(2);
    socket_servidor->apagar(2);
    socket_cliente->cerrar();
    socket_servidor->cerrar();

    //se guarda la informacion correspondientes en la bitacora
    balanceador->log(" * [" + Utilidades::getFecha() + "] hacia [" + string(balanceador->servidores[indice_ip].ip) +"]\n");

    delete socket_cliente;
    delete socket_servidor;
} 


//Requiere: -
//Retorna: -
//Modifica: variable escuchador del objeto Balanceador y la bitacora
void hilo_Escuchador_Clientes(){

    while(balanceador->ejecutandose){
        Socket* clienteEntrante = balanceador->escuchador_http->aceptar();

        //mientras el socket no sea un nullpointer se trabaja con el socket que se ha aceptado
        if(nullptr != clienteEntrante){
            char direccion[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clienteEntrante->in_IPv4.sin_addr), direccion, INET_ADDRSTRLEN);

            thread hilo(hilo_Cliente, clienteEntrante);
            hilo.detach();
        }
        
    }

    delete balanceador->escuchador_http;

}  

//Requiere:  -
//Retorna:   -
//Modifica:  Utiliza el socket de broadcast, pero no lo modifica
void hilo_Escuchador_Servidores(){

    while(balanceador->ejecutandose){
        char bufferBalanceadorRespuesta[64];
        int leidosBroadcast = balanceador->recibirDatagram(bufferBalanceadorRespuesta);
        bufferBalanceadorRespuesta[leidosBroadcast] = '\0';

        if(leidosBroadcast > 0 && bufferBalanceadorRespuesta[0] == 'S'){

            if (bufferBalanceadorRespuesta[2] == 'C'){
                balanceador->agregarServidor(bufferBalanceadorRespuesta);
            }

            if (bufferBalanceadorRespuesta[2] == 'D'){
                balanceador->eliminarServidor(bufferBalanceadorRespuesta);
            }

        }

    }
}

// FUNCIONES DE CLASE

//Requiere: Booleana que indica a los servidores si estamos desconectandonos o conectandonos
//Retorna:  -
//Modifica: Los sockets si es necesario, y envia mensajes de broadcast por ellos
void Balanceador::avisarServidores(bool desconectar){
    
    if(balanceador->escuchador_broadcast != nullptr){
        balanceador->escuchador_broadcast->apagar(2);
        balanceador->escuchador_broadcast->cerrar();
    }

    balanceador->escuchador_broadcast = new Socket(false, false);
    escuchador_broadcast->vincularBroadcast(Utilidades::getBroadcastIP(Utilidades::recuperarSubnet(), 28).c_str(), puerto_servidor, BIND);
    string bufferBroadcast = "B/C/";

    if(desconectar){
        bufferBroadcast = "B/D/";
        log(" * El balanceador envia DISCONNECT al servidor. \n");
    }

    bufferBroadcast += Utilidades::recuperarSubnet();//string(this->direccionHost);
    bufferBroadcast += "/" + to_string(this->puerto_cliente);
    enviarDatagram(bufferBroadcast.c_str());
}

//Requiere:  -
//Retorna:   -
//Modifica:  Modifica los sockets de las clases, o bien construye nuevos de necesitarlo
void Balanceador::vincularPuertos(){

    // Verifique que sea posible la conexion
    // Intente primero con el socket HTTP
    this->escuchador_http = new Socket(true, false);
    int estado = escuchador_http->vincular(nullptr, puerto_cliente);
    
    if(-1 == estado){
        imprimirErrorEscucha(puerto_cliente);
        this->ejecutandose = false;
        return;
    }else{
        escuchador_http->escuchar(128);
    }
    
    // Intente ahora con el socket broadcast
    this->escuchador_broadcast = new Socket(false, false);
    estado = escuchador_broadcast->vincularBroadcast(
    Utilidades::getBroadcastIP(Utilidades::recuperarSubnet(), 28).c_str(), puerto_servidor, BIND);

    if(-1 == estado){
        imprimirErrorEscucha(puerto_servidor);
        this->ejecutandose = false;
        return;
    }
    
    if(this->ejecutandose){
        log(" * Puerto de Requests: " + to_string(puerto_cliente) + "\n");
        log(" * Puerto de Broadcast: " + to_string(puerto_servidor) + "\n");
        log(" * Direccion IP Local: " + string(this->direccionHost) + "\n");
        log(" * Direccion IP Subnet: " + string(Utilidades::recuperarSubnet()) + "\n");
        log(" * Direccion IP Broadcast: " + Utilidades::getBroadcastIP(Utilidades::recuperarSubnet(), 28) + "\n");
        sleep(1);
        setAlgoritmoBalanceo(0);
        avisarServidores(false);
    }
        
}

//Requiere: Puntero del buffer donde se leerá los datos
//Retorna:  Numero de bytes leidos o -1 en caso de error
//Modifica: buffer
int Balanceador::recibirDatagram(char* buffer){ 
    return escuchador_broadcast->leerDe(buffer, 64);
} 

//Requiere: Puntero del buffer que contiene el mensaje
//Retorna:  Cantidad de bytes escritos o -1 en caso de error
//Modifica: -
int Balanceador::enviarDatagram(const char* buffer){
    return escuchador_broadcast->escribirA(buffer, strlen(buffer));
}

//Requiere: Puntero del buffer que contiene el mensaje | tamaño del buffer | socket de destino
//Retorna:  Cantidad de bytes escritos o -1 en caso de error
//Modifica: -
int Balanceador::enviarMensaje(const char* buffer, int bufferSize, Socket* destino){ 
    return destino->escribir(buffer, bufferSize); 
}  

//Requiere: Puntero del buffer donde se leerá los datos | tamaño del buffer | socket de fuente 
//Retorna:  Cantidad de bytes leidos o -1 en caso de error
//Modifica: -
int Balanceador::recibirMensaje(char* buffer, int bufferSize, Socket* fuente){ 
    return fuente->leer(buffer, bufferSize, 0, 0);
}  

//Requiere: Hilera que contiene el mensaje a registrar en el logger
//Retorna:  -
//Modifica: La cola de mensajes "log_mensajes"
void Balanceador::log(string mensaje){ 
   unique_lock<mutex> lock(mutex_mensajes); 
   log_mensajes.push(mensaje);   
} 

//Requiere: El mensaje que se desea ingresar a la cola
//Retorna:  -
//Modifica: La cola de mensajes "log_mensajes" a la que le ingresa "mensaje" de color rojo
void Balanceador::logRed(string mensaje){
   unique_lock<mutex> lock(mutex_mensajes); 
   log_mensajes.push(RED);        // Coloca el color de los mensajes en rojo  
   log_mensajes.push(mensaje);   
   log_mensajes.push(WHITE);      // Devuelve el color de los mensajes a blanco   
} 

//Requiere: El valor del algoritmo de balanceo
//Retorna:  -
//Modifica: El valor almacenado en el "algoritmoBalanceo"
void Balanceador::setAlgoritmoBalanceo(int algoritmo){
    
    // Se revisa que el valor recibido es un valor que se encuentre dentro de los esquemas de balanceo que se manejan
    if(algoritmo >= ALGORITMO::ROUND_ROBIN && algoritmo <= ALGORITMO::CONEXIONES_ACTIVA)
    {
        this->algoritmoBalanceo.store(algoritmo);
        log(" * (METODO DE BALANCEO ACTUAL: ");
        switch (algoritmo)
        {
         case ALGORITMO::ROUND_ROBIN:
            log("Round Robin)\n");
            break;

         case ALGORITMO::ROUND_ROBIN_PESOS:
            log("Round Robin con pesos)\n");
            break;

         case ALGORITMO::CONEXIONES_ACTIVA:
            log("Conexiones Activas)\n");
            break;

         case ALGORITMO::DIRECCION_CLIENTE:
            log("Direccion Cliente)\n");
            break;   
        }
        
    }
}

//Requiere: -
//Retorna:  -
//Modifica: La cola de mensajes del logger
void Balanceador::mostrarEstadisticas(){
    log(" * (SERVIDORES CONECTADOS: "); 
    log(to_string(this->stats.servidoresActuales));
    log("/");
    log(to_string(MAX_SERVIDORES));
    log(")\n");
}

//Requiere: -
//Retorna:  -
//Modifica: La cola de mensajes "log_mensajes"
void Balanceador::manejarBitacora(){

    string hilera;

    lock_guard<mutex> lock(mutex_mensajes);

    // Mientras la cola de mensajes no este vacia se van sacando y se van escribiendo en el archivo de bitacora
    while(!log_mensajes.empty()){
        hilera = log_mensajes.front();
        printf("%s", hilera.c_str());
        if(hilera.compare(RED) != 0 && hilera.compare(WHITE) != 0){
            fs.escribirArchivo(hilera.c_str(), this->filePosition, hilera.size());
            this->filePosition += hilera.size(); 
        }
        log_mensajes.pop();
    }    

}

//Requiere: Puntero a hilera que contiene el ID del servidor
//Retorna:  -
//Modifica: Si no esta duplicado dicho ID, se agrega como servidor nuevo
void Balanceador::agregarServidor(char* ip){
    ip = &ip[4];
    int strpos = string(ip).find('/') - 1;
    string addr = string(ip);
    addr = addr.substr(0, strpos+1);
    string port = string(ip);
    port = port.substr(strpos+2);

    if((buscarServidor(ip) == -1)){
        ServidorRegistrado s;
        s.carga_actual = 0;
        s.carga_maxima = Utilidades::getNumeroRandom(5,10);
        strncpy(s.ip, addr.c_str(), addr.length());
        s.port = atoi(port.c_str());
        strcpy(s.comparador, ip);
        {
            unique_lock<mutex>lockservidores(mutex_servidores);
            servidores.push_back(s);
        }
        log(" * Se ha agregado un servidor: " + addr + ":" + string(port) + "\n");
        stats.servidoresActuales++;
        balanceador->avisarServidores(false);
        this->mostrarEstadisticas();
    }
}

//Requiere: Puntero a hilera que contiene el ID del servidor
//Retorna:  -
//Modifica: Si se encontro el servidor, se eliminara
void Balanceador::eliminarServidor(char* ip){\
    ip = &ip[4];
    int strpos = string(ip).find('/') - 1;
    string addr = string(ip);
    addr = addr.substr(0, strpos + 1);
    string port = string(ip);
    port = port.substr(strpos+2);
    int posicion;
    if((posicion = buscarServidor(ip)) >= 0){
        {
            unique_lock<mutex>lockServidor(mutex_servidores);
            servidores.erase(servidores.begin() + posicion);
        }
        stats.servidoresActuales--;
        log(" * Se ha eliminado el servidor: " + addr + ":" + string(port) + "\n");
        this->mostrarEstadisticas();
    }
}             

//Requiere: Puntero a hilera que contiene el ID del servidor
//Retorna:  Posicion del servidor en el vector
//Modifica: -
int Balanceador::buscarServidor(const char* hilera){
    {
        lock_guard<mutex>lockservidor(mutex_servidores);
        for(int i = 0; i < servidores.size(); i++){
            if(servidores[i].comparador != nullptr && strcmp(hilera, servidores[i].comparador) == 0){
                return i;
            }
        }
    }
    return -1;
} 

//Requiere: Socket asociado al cliente
//Retorna:  -
//Modifica: Agrega el cliente como un nuevo usuario
void Balanceador::agregarCliente(Socket* socket){
    if((buscarCliente(socket) == -1)){
        ClienteRegistrado c;
        c.IP = socket->in_IPv4;
        int random = Utilidades::getNumeroRandom(0, servidores.size()-1);
        c.servidor_asociado = &servidores[random];
        {
            unique_lock<mutex>lockCliente(mutex_clientes);
            clientes.push_back(c);
        }
        log(" * Cliente nuevo registrado para el metodo de Balanceo. \n");
    }
} 
 
  
//Requiere: Socket asociado al cliente
//Retorna:  Posicion del cliente en el vector
//Modifica: -
int Balanceador::buscarCliente(Socket* socket){
    {
        lock_guard<mutex>lockcliente(mutex_clientes);
        for(int i = 0; i < clientes.size(); i++){
            if(inet_ntoa(socket->in_IPv4.sin_addr) == inet_ntoa(clientes[i].IP.sin_addr)){
                return i;
            }
        }
    }
    return -1;
}

//Requiere: -
//Retorna:  La posicion del servidor
//Modifica: -
int Balanceador::buscarServidorConMenorCarga(){
    int posicion = -1;
    int carga_menor = INT32_MAX;
    {
        lock_guard<mutex> guardServidor(mutex_servidores);
        if(this->servidores.size() > 0){
            for(int i = 0; i < servidores.size(); i++){
                if(servidores.at(i).carga_actual < carga_menor)
                {
                    carga_menor = servidores.at(i).carga_actual;
                    posicion = i;
                }
            }
        }
    }
    return posicion;
}
      
// Requiere: Un numero cuyo valor coincida con los valores en el enum STATUS_CODES.
// Modifica: -
// Retorna:  Una hilera con la representacion textual del estado.
string Balanceador::recuperarEstado(int numero){
    string retorno;

    switch(numero){
        case STATUS_INTERNAL_SRV_ERR_500:
            retorno = "INTERNAL SERVER ERROR";
        break;
        case STATUS_NOT_IMPLMTD_501:
            retorno = "NOT IMPLEMENTED";
        break;
        case STATUS_BD_GTWY_502:
            retorno = "BAD GATEWAY";
        break;
        case STATUS_SRV_UNAVLBL_503:
            retorno = "SERVICE UNAVAILABLE";
        break;
        case STATUS_GTWY_TIMEOUT_504:
            retorno = "GATEWAY TIMEOUT";
        break;
        case STATUS_HTTP_VRSN_NOT_SPPRTD_505:
            retorno = "HTTP VERSION NOT SUPPORTED";
        break;
    }
    return retorno;
}

//Requiere: Codigo del header (200, 400, 404, etc)
//Retorna:  Hilera que contiene un header valido
//Modifica: -
string Balanceador::recuperarHeader(int estadoCodigo){
    string stringBuffer;
    stringBuffer.append(Balanceador::HTTP_VERSION_11 + " ");
    stringBuffer.append(to_string(estadoCodigo) + " ");
    stringBuffer.append(this->recuperarEstado(estadoCodigo) + " ");
    stringBuffer.append(CRLF);
    stringBuffer.append("Server: ").append(SERVER_NAME);
    stringBuffer.append(CRLF);
    stringBuffer.append("Date: ").append(Utilidades::getFechaGMT());
    stringBuffer.append(DOUBLECRLF);
    return stringBuffer;
}

//Esta funcion devuelve un mensaje HTML con el cuerpo de un codigo de error
//Requiere: Puntero al buffer de almacenamiento | tamaño del buffer | codigo de error HTTP
//Retorna:  bytesEscritos
//Modifica: -
int Balanceador::recuperarBodyPaginaError(char* buffer, int buffersize, int error){
    string respuestaError;

    string estado = this->recuperarEstado(error);
    string mensajeError = "\tError Processing Request: ";
    mensajeError.append(estado);
    mensajeError.append(" ");
    mensajeError.append(to_string(error));
    mensajeError.append("\n");

    char* cpath = getcwd(nullptr, 0);
    string spath(cpath);
    delete cpath;
    spath.append(ERRORPAGE_HTML_PATH);
    
    // Cambiamos el place holder por un string con nuestro mensaje personalizado
    respuestaError = Utilidades::leerArchivo(spath);
    Utilidades::replacePlaceHolder(string(ERRORPAGE_PLACEHOLDER), respuestaError, mensajeError);
    
    memset(buffer, 0, buffersize);
    int bytesEscritos = snprintf(buffer, buffersize, "%s", respuestaError.c_str());
    
    return bytesEscritos;
    
}

//Requiere:  -
//Retorna:   -
//Modifica:  -
int Balanceador::ejecutarBalanceador(){

    // Asigne las estadisticas por defecto
    stats.servidoresActuales = 0;

    // Inicie los Hilos
    thread _hilo_OU(hilo_Output);
    thread _hilo_IN(hilo_Input);

    // Este hilo debe ser matado ya que el call getline/scanf bloquea IO
    thread::native_handle_type handle = _hilo_IN.native_handle();

    // Hilos Escuchadores de Request/Broadcast respectivamente
    thread _hilo_EC(hilo_Escuchador_Clientes);
    thread _hilo_ES(hilo_Escuchador_Servidores);

    // Este lo mataremos manualmente
    _hilo_IN.detach();

    // Unificar los hilos
    _hilo_OU.join();
    _hilo_EC.join();
    _hilo_ES.join();

    // Cuando el programa muere, matamos el hilo Input
    pthread_cancel(handle);
    
    // Cierre normal del programa
    return 0;
}

//Requiere:  -
//Retorna:   -
//Modifica:  -
int main(int argc, char **argv){

    char* direccionHost = new char[64];
    int puerto_cliente  = PORT_HTTP;
    int puerto_servidor = PORT_BROADCAST;

    if(argc < 4){
        Utilidades::colorError();
        printf(" * Balanceador fallo en abrir.\n"); 
        Utilidades::colorReset();
        printf(" * Parametros insuficientes. Se necesitan tres parametros.\n"); 
        printf(" * Uso: ./bin/Balanceador <direccion-ip> <puertoHttp> <puertoBroadcast>\n");
        Utilidades::colorError();
        printf(" * Abortando el programa... \n");
        Utilidades::colorReset();
        _exit(1);
    }
    strcpy(direccionHost, argv[1]);
    puerto_cliente  = atoi(argv[2]);
    puerto_servidor = atoi(argv[3]);

    balanceador = new Balanceador(0, direccionHost, puerto_cliente, puerto_servidor);
    balanceador->vincularPuertos();
    balanceador->ejecutarBalanceador();

    delete balanceador;
    delete[] direccionHost;

    return 0;

}

