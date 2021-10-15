#include "openfiletable.h"
#define fileLimit 32

//@autor Lester Cordero Murillo

// Initialize 
OpenFilesTable::OpenFilesTable(){
    this->usage = 0;
    this->openFiles    = new int[fileLimit];
    this->openFilesMap = new BitMap(fileLimit);
    Open(-1); // in
    Open(-1); // out
    Open(-1); // err
}       

// De-allocate
OpenFilesTable::~OpenFilesTable(){
    delete this->openFiles;
    delete this->openFilesMap;
}     

// Register the file handle  
// It saves the UnixHandle
int OpenFilesTable::Open( int UnixHandle ){
    openFiles[usage] = UnixHandle;
    openFilesMap->Mark(usage);
    return usage++;

} 

// Close the file
int OpenFilesTable::Close( int NachosHandle ){
    openFiles[NachosHandle] = 0;
    openFilesMap->Clear(NachosHandle);  
    return 0;
}  

bool OpenFilesTable::isOpened(int NachosHandle ){
    return openFilesMap->Test(NachosHandle);
}

int OpenFilesTable::getUnixHandle( int NachosHandle ){
    return openFiles[NachosHandle];
}

void OpenFilesTable::addThread(){
    usage++;
}	

void OpenFilesTable::delThread(){
    usage--;
    if(usage == 0){
        for(int i = 0; i < fileLimit ; i++){
            openFilesMap->Clear(i);
            openFiles[i] = 0;
        }
    }
}

void OpenFilesTable::Print(){
    printf("Open files: %d \n", usage);
    printf("Bitmap: ");
    for(int i = 0; i < fileLimit; i++){
        printf("%d", openFilesMap->Test(i));
    }
    printf("\n");
}  
