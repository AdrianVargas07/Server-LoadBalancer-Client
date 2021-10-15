#include "syscall.h"

#define BUFFER_SIZE 1024
#define DEBUG 1

// We coded this group of functions to help us
// Call it, the Utility Class, but with all the code here like a mess
// As we dont have much time left and we can only include the syscall.h

int len(char *param){  
    int counter = 0;
    while(param[counter++] != '\0');
    return --counter;
}

void concat(char* str1, char* str2, char* dest){
	int i, j, str1_size, str2_size;
	str1_size = len(str1);
	str2_size = len(str2);

	for (i = 0; i < str1_size; i++) {
    	dest[i] = str1[i];
    }
	for (j = 0; j < str2_size; j++) {
    	dest[i + j] = str2[j];
    }
	dest[i + j] = '\0';
}

void clear(char* buffer, int size){
	int i;
	for (i = 0; i < size; i++) {
    	buffer[i] = 0;
    }
}

int atoi(char* str){ 
    int i, ret = 0; 
    for (i = 0; str[i] != '\0'; i++) {
 		ret = ret * 10 + str[i] - '0'; 
	}
    return ret; 
} 

void lowerCase(char* str) {
	int i, size;
	size = len(str);
	for (i = 0; i < size; i++) {
    	if (str[i] >= 'A' && str[i] <= 'Z') {
        	str[i] += 32;
        } 
    }
}

int find(char* str, char* splitter, int limit){
	int i, j, str_size, splitter_size;
	str_size      = len(str);
	splitter_size = len(splitter);
	for(i = 0; i < limit && i < str_size; i++){
		j = 0;
		while(str[i + j] == splitter[j] && ((i + j) < str_size)){
			j++;

			if(j == splitter_size){
				return i;
			}
		}
	}
	return -1;
}

int find_last_char(char* str, char single_char){
	int i, ret, str_size;
	ret = -1;
	str_size = len(str);
	for(i = 0; i < str_size; i++){
		if(str[i] == single_char){
			ret = i;
		}
	}
	return ret;
}

void newLine(){
	Write("\n",1, 1, 0);
}

int main () {

	// Define buffers and variables
	char buffer_ip[32];      
	char buffer_port[4];    
	char buffer_response[BUFFER_SIZE];
	char buffer_request[32];
	char buffer_tmp[32]; 

	char buffer_route[32]; 
	char buffer_filename[32]; 

	int  shouldWriteToDisk    = 0;
	int  showHeaders          = 0;
	int  isFilePrintable      = -1;
	int  errNotFound          = -1;
	int  readBytes            = 0;
	int  filePos              = 0;
	int  usedDef              = 0;

	// Buffers and some texts
	char* err1 = "\nError de conexion, abortando maquina nach0S. \n\n";
	char* err2 = "\nError al leer el encabezado, abortando maquina nach0S. \n\n";

	char* msg1 = "\nIngrese la direccion IP que desea conectarse:\n";
	char* msg2 = "\nIngrese el puerto que desea conectarse:\n";
	char* msg3 = "\n¿Desea intentar descargar el archivo? Escriba Y / N\n";
	char* msg4 = "\n¿Desea ver los headers del request? Escriba Y / N\n";
	char* msg5 = "\n\nSe descargo el archivo en la carpeta ./files \n";
	char* msg6 = "\nIngrese la subruta. / para asumir /index.html \n";
	char* msg7 = "\nImposible imprimir datos binarios, solo documentos de texto.\n";

	// Read the parameters from the console
	Write(msg1, len(msg1), 1, 0);
	int k = Read(buffer_ip, 32, 0);
	buffer_ip[k] = '\0';

	Write(msg2, len(msg2), 1, 0);
	k = Read(buffer_port, 4, 0);
	buffer_port[k] = '\0';

	// Connect to the ip address
	int port = atoi(buffer_port);
	int socket = Socket(0, 0);

	if (Connect(socket, buffer_ip, port) == -1){
		Write(err1, len(err1), 1, 0);
		Halt();
	}

	// Ask the route
	buffer_route[0] = '\0';
	Write(msg6, len(msg6), 1, 0);
	k = Read(buffer_route, 32, 0);
	if(buffer_route[0] == '/' && k < 2){ 
		usedDef = 1;
	}
	buffer_route[k] = '\0';

	// Ask if we should download the file
	Write(msg3, len(msg3), 1, 0);
	Read(buffer_tmp, 1, 0);
	lowerCase(buffer_tmp);
	if(buffer_tmp[0] == 'y'){ shouldWriteToDisk = 1; }

	// Ask if we must show the headers or the body
	Write(msg4, len(msg4), 1, 0);
	Read(buffer_tmp, 1, 0);
	lowerCase(buffer_tmp);
	if(buffer_tmp[0] == 'y'){ showHeaders = 1; }

	concat("GET ", buffer_route,  buffer_tmp);
	concat(buffer_tmp, " HTTP/1.1\r\nHost: www.google.com \r\n\r\n",  buffer_request);
	Write(buffer_request, len(buffer_request), socket, 0);

	// Wait for the response
	// First we need to read the headers
	// And split them after \r\n\r\n
	int bytesfromHeader = Read(buffer_response, BUFFER_SIZE, socket);
	int split_pos       = find(buffer_response, "\r\n\r\n", BUFFER_SIZE);

	// If we got no errors, then continue
	if(split_pos == -1){
		Write(err2, len(err2), 1, 0);
		Write(buffer_response, BUFFER_SIZE, 1, 0);
		Halt();
	}

	newLine();

	// Get filename from the route
	int name_pos   = find_last_char(buffer_route, '/');
	if(usedDef > 0){
		concat("files", "/default.html", buffer_filename);
	}else{
		concat("files", &buffer_route[name_pos], buffer_filename);
	}

	int fd = -1;
	if(shouldWriteToDisk){
		Create(buffer_filename);
		fd = Open(buffer_filename);
		if (-1 == fd){
			Halt();
		}
	}

	isFilePrintable = find(buffer_response, "Content-Type: text/", BUFFER_SIZE);
	errNotFound     = find(buffer_response, "404", BUFFER_SIZE);

	// Print headers only if the user wants 
	if(showHeaders){
		Write(buffer_response, split_pos, 1, 0);
		if(isFilePrintable > 0 || errNotFound > 0){
			newLine();
			newLine();
		}
	}


	// The first package its special!
	split_pos += len("\r\n\r\n");
	int split_size = bytesfromHeader - split_pos;

	if(isFilePrintable > 0){ 
		Write(&(buffer_response[split_pos]), split_size, 1, 0); 
	}

	if(shouldWriteToDisk){ 
		//split_size = BUFFER_SIZE - split_size;
		Write(&buffer_response[split_pos], split_size, fd, 0);
		filePos += split_size;
	}

	if(isFilePrintable < 1 && !shouldWriteToDisk && errNotFound < 1){
		Write(msg7, len(msg7), 1, 0);
	}

	// Now, we got the headers, now do a full BODY read from the sockets
	// This will NOT CONTAIN the headers, only body data in binary
	while( (readBytes = Read(buffer_response, BUFFER_SIZE, socket) ) > 0 ){
        
		//buffer_response[readBytes] = '\0'; 
		
		if(isFilePrintable > 0 || errNotFound > 0){
			Write(buffer_response, readBytes, 1, 0);
		}

		if(shouldWriteToDisk){ 
			Write(buffer_response, readBytes, fd, filePos);
		}

        clear(buffer_response, BUFFER_SIZE);
		filePos += readBytes;

	}

	if(shouldWriteToDisk){
		Write(msg5, len(msg5), 1, 0);
	}

	if(shouldWriteToDisk){
		Close(fd);
	}

	// Exit the program
	newLine();
	Halt();
	return 0;
} 