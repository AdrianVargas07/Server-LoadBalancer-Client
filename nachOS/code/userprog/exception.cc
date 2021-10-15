// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "openfiletable.h"
#include "bitmap.h"
#include "console.h"
#include "synch.h"
#include <string.h>
#include <stdio.h>

void returnFromSystemCall() {
   int pc, npc;

   pc = machine->ReadRegister( PCReg );
   npc = machine->ReadRegister( NextPCReg );
   machine->WriteRegister( PrevPCReg, pc );        // PrevPC <- PC
   machine->WriteRegister( PCReg, npc );           // PC <- NextPC
   machine->WriteRegister( NextPCReg, npc + 4 );   // NextPC <- NextPC + 4
} 

char* systemToNachos(int virtual_address, int size){

   // get virtual address 
   char* buffer = new char[size+1];
   int   buffer_local;

   // traslate to physical memory
   for (int i = 0; i < size; i++)	{
		machine->ReadMem(virtual_address + i, 1, &buffer_local);
		buffer[i] = buffer_local;
	}

   buffer[size] = '\0';
   return buffer;
}

int nachosToSystem(int virtual_address, char* buffer, int size){

   if (size <  0)   return -1;
   if (size == 0) return size;

   int i = 0;
   int buffer_local = 0;

   // do it at least one time
   do{
      buffer_local = (int) buffer[i];
      machine->WriteMem(virtual_address + i, 1, buffer_local);
      i++;
   }while (i<size);
   
   return i;

}

/*
 *  System call interface: Halt()
 */
void NachOS_Halt() {		// System call 0

	DEBUG('a', "Shutdown, initiated by user program.\n");
   interrupt->Halt();


}

/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {		// System call 1
   
   delete currentThread->space;
   currentThread->Finish();

}

/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {		// System call 2
}


/*
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {		// System call 3
}


/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() {		// System call 4

   char* buffer = systemToNachos(machine->ReadRegister( 4 ), 64);
   machine->WriteRegister(2, open (buffer, O_TRUNC | O_CREAT, 0777));
   delete [] buffer;

}

/*
 *  System call interface: OpenFileId Open( char * )
 */
void NachOS_Open() {		// System call 5

   char* buffer = systemToNachos(machine->ReadRegister( 4 ), 64);

   int unixFD = open (buffer, O_RDWR );
   int nachosFD = OFT.Open(unixFD);

   OFT.addThread();

   machine->WriteRegister(2, nachosFD);


   delete [] buffer;
   
}


/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId , int)
 */
void NachOS_Write() {		// System call 6

   int size      = machine->ReadRegister( 5 );	      
   OpenFileId id = machine->ReadRegister( 6 );	
   int position  = machine->ReadRegister( 7 );	

   char* buffer  = systemToNachos(machine->ReadRegister( 4 ), size);

	switch (id) {

		case  ConsoleInput:

			machine->WriteRegister( 2, -1 );
			break;

		case  ConsoleOutput:
         
			printf("%s", buffer); 
         machine->WriteRegister( 2, strlen(buffer) );
         stats->numConsoleCharsWritten += strlen(buffer);
		   break;

		case ConsoleError:      

			printf("%d", machine->ReadRegister( 4 )); 
         machine->WriteRegister( 2, 0 );
		   break;

		default:	

         int handle = OFT.getUnixHandle(id);
         lseek (handle, position, SEEK_SET);
         int written = write(handle, buffer, size);
         //DEBUG('z', "written to disk: %d\n", written);
         machine->WriteRegister( 2, written );
         stats->numConsoleCharsWritten += written;
			break;

	}

   delete[] buffer;

}  


/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() {		// System call 7

   int      size = machine->ReadRegister( 5 );	      
   OpenFileId id = machine->ReadRegister( 6 );	
   char* buffer  = new char[1024];

	switch (id) {

		case  ConsoleInput:

         scanf("%s",buffer); 
         machine->WriteRegister( 2, nachosToSystem(machine->ReadRegister( 4 ), buffer, size));
         stats->numConsoleCharsRead += strlen(buffer);
		   break;
	
		case  ConsoleOutput:
         
         machine->WriteRegister( 2, -1 );
			break;			

		case ConsoleError:      

			printf("%d\n", machine->ReadRegister( 4 )); 
         machine->WriteRegister( 2, 0 );
		   break;

		default:	

         int handle = OFT.getUnixHandle(id);
         int readed = recv(handle, buffer, size, MSG_WAITALL);
         nachosToSystem(machine->ReadRegister( 4 ), buffer, readed);
         machine->WriteRegister( 2, readed );
         DEBUG('z', "%d, ", readed);
         stats->numConsoleCharsRead += readed;
			break;

	}

   delete[] buffer;
}


/*
 *  System call interface: void Close( OpenFileId )
 */
void NachOS_Close() {		// System call 8

   int nachosID = machine->ReadRegister( 4 );
   int unixID = OFT.getUnixHandle(nachosID);

   close ( unixID );
   OFT.delThread();
}


/*
 *  System call interface: void Fork( void (*func)() )
 */
void NachOS_Fork() {		// System call 9


}


/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() {		// System call 10
}


/*
 *  System call interface: Sem_t SemCreate( int )
 */
void NachOS_SemCreate() {		// System call 11
}


/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() {		// System call 12
}


/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() {		// System call 13
}


/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() {		// System call 14
}


/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate() {		// System call 15
}


/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() {		// System call 16
}


/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() {		// System call 17
}


/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() {		// System call 18
}


/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() {		// System call 19
}


/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() {		// System call 20
}


/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() {		// System call 21
}


/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() {		// System call 22
}


/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() {		// System call 23
}


/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() {			// System call 30
   
   int family          = machine->ReadRegister(4);
   int socketType      = machine->ReadRegister(5);

   int socketFD         = -1;
   int unixSocketFamily = -1;
   int unixSocketType   = -1;

   // Define socket type
   if(socketType == SOCK_STREAM_NachOS)  {
      unixSocketType = SOCK_STREAM;
   } else if(socketType == SOCK_DGRAM_NachOS) {
      unixSocketType = SOCK_DGRAM;
   }else{
      machine->WriteRegister(2, -1);
      return;
   }

   // Define socket family
   if(family == AF_INET_NachOS){
      unixSocketFamily = AF_INET;
   }else if(family == AF_INET6_NachOS){
      unixSocketFamily = AF_INET6;
   }else{
      machine->WriteRegister(2, -1);
      return;
   }

   socketFD = socket(unixSocketFamily, unixSocketType, 0);
   int nachosFD = OFT.Open(socketFD);

   machine->WriteRegister(2, nachosFD);
}

/*
 *  System call interface: int Connect( OpenFileID, char *, int )
 */
void NachOS_Connect() {		// System call 31

   int status        = -1;
   int socketFD      = machine->ReadRegister(4);
   char* socketAddr  = systemToNachos(machine->ReadRegister(5), 32);

   int port          = machine->ReadRegister(6);
   socklen_t length  = sizeof( sockaddr );

   sockaddr     addressFamily;
   sockaddr_in  addressInfo;
   sockaddr_in6 addressInfo6;

   //DEBUG('s', "socketFD: %d, socketAddr: %s port: %d\n\n", socketFD, socketAddr, port);

   getsockname(socketFD, (struct sockaddr*) &addressFamily, &length);

   if(addressFamily.sa_family == AF_INET){
      
      addressInfo.sin_port         = htons(port);
      addressInfo.sin_addr.s_addr  = inet_addr(socketAddr);
      addressInfo.sin_family       = AF_INET;

      inet_pton(AF_INET, socketAddr, &addressInfo.sin_addr); 
      
      status = connect(socketFD, (struct sockaddr *)&addressInfo, sizeof(addressInfo));
      if(status != -1){
         machine->WriteRegister(2, socketFD);
         return;
      }
      
   }else if(addressFamily.sa_family == AF_INET6){
      
      addressInfo6.sin6_port   = htons(port);
      addressInfo6.sin6_family = AF_INET6;

      status = connect(socketFD, (struct sockaddr *)&addressInfo6, sizeof(addressInfo6));
      if(status != -1){
         machine->WriteRegister(2, socketFD);
         return;
      }

   }

   machine->WriteRegister(2, -1);
         
}

/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() {		

   int socketFD      = machine->ReadRegister(4);
   int port          = machine->ReadRegister(5);
   int status        = -1;
   socklen_t length  = sizeof( sockaddr );

   sockaddr     addressFamily;
   sockaddr_in  addressInfo;
   sockaddr_in6 addressInfo6;

   getsockname(socketFD, (struct sockaddr*) &addressFamily, &length);

   if(addressFamily.sa_family == AF_INET){

      addressInfo.sin_addr.s_addr = INADDR_ANY;
      addressInfo.sin_family      = AF_INET;
      addressInfo.sin_port        = htons(port);
      status = bind(socketFD, (struct sockaddr *)&addressInfo, sizeof(addressInfo));
      machine->WriteRegister(2, status);
      return;

   }else if(addressFamily.sa_family == AF_INET6){

      addressInfo6.sin6_addr         = in6addr_any;
      addressInfo6.sin6_family       = AF_INET6;
      addressInfo6.sin6_port         = htons(port);
      status = bind(socketFD, (struct sockaddr *)&addressInfo6, sizeof(addressInfo6));
      machine->WriteRegister(2, status);
      return;

   }
    
   machine->WriteRegister(2, -1);
}


/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() {		// System call 33

   int socketFD  = machine->ReadRegister(4);
   int backlog   = machine->ReadRegister(5);
   machine->WriteRegister(2, listen(socketFD, backlog));

}


/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() {		// System call 34

   int socketFD     = machine->ReadRegister(4);
   int status       = -1;
   socklen_t length = sizeof( sockaddr );

   sockaddr     addressFamily;
   sockaddr_in  addressInfo;
   sockaddr_in6 addressInfo6;

   getsockname(socketFD, (struct sockaddr*) &addressFamily, &length);

   if(addressFamily.sa_family == AF_INET){

      status = accept(socketFD, (struct sockaddr *)&addressInfo, &length);
      machine->WriteRegister(2, status);
      return;  

   }else if(addressFamily.sa_family == AF_INET6){

      status = accept(socketFD, (struct sockaddr *)&addressInfo6, &length);
      machine->WriteRegister(2, status);
      return;  

   }
    
   machine->WriteRegister(2, -1);

}


/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {	// System call 25

   int socketFD = machine->ReadRegister(4);
   int mode     = machine->ReadRegister(5);
   int status   = -1;
   if(mode == SHUT_RD || mode == SHUT_WR || mode == SHUT_RDWR){
      status = shutdown(socketFD, mode);   
   }

   machine->WriteRegister(2, status);
   
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    switch ( which ) {

      case SyscallException:
          switch ( type ) {
             case SC_Halt:		   // System call # 0
               NachOS_Halt();
               break;
             case SC_Exit:		   // System call # 1
               NachOS_Exit();
               break;
             case SC_Exec:		   // System call # 2
               NachOS_Exec();
               break;
             case SC_Join:		   // System call # 3
               NachOS_Join();
               break;
             case SC_Create:		// System call # 4
               NachOS_Create();
               break;
             case SC_Open:		// System call # 5
               NachOS_Open();
               break;
             case SC_Read:		// System call # 6
               NachOS_Read();
               break;
             case SC_Write:		// System call # 7
               NachOS_Write();
               break;
             case SC_Close:		// System call # 8
               NachOS_Close();
               break;
             case SC_Fork:		// System call # 9
               NachOS_Fork();
               break;
             case SC_Yield:		// System call # 10
               NachOS_Yield();
               break;

             case SC_SemCreate:         // System call # 11
               NachOS_SemCreate();
               break;
             case SC_SemDestroy:        // System call # 12
               NachOS_SemDestroy();
               break;
             case SC_SemSignal:         // System call # 13
               NachOS_SemSignal();
               break;
             case SC_SemWait:           // System call # 14
               NachOS_SemWait();
               break;
             case SC_LckCreate:         // System call # 11
               NachOS_LockCreate();
               break;
             case SC_LckDestroy:        // System call # 12
               NachOS_LockDestroy();
               break;
             case SC_LckAcquire:         // System call # 13
               NachOS_LockAcquire();
               break;
             case SC_LckRelease:           // System call # 14
               NachOS_LockRelease();
               break;
             case SC_CondCreate:         // System call # 11
               NachOS_CondCreate();
               break;
             case SC_CondDestroy:        // System call # 12
               NachOS_CondDestroy();
               break;
             case SC_CondSignal:         // System call # 13
               NachOS_CondSignal();
               break;
             case SC_CondWait:           // System call # 14
               NachOS_CondWait();
               break;
             case SC_CondBroadcast:           // System call # 14
               NachOS_CondBroadcast();
               break;
             case SC_Socket:	// System call # 20
		         NachOS_Socket();
               break;
             case SC_Connect:	// System call # 21
		         NachOS_Connect();
               break;
             case SC_Bind:	// System call # 22
		         NachOS_Bind();
               break;
             case SC_Listen:	// System call # 23
		         NachOS_Listen();
               break;
             case SC_Accept:	// System call # 22
		         NachOS_Accept();
               break;
             case SC_Shutdown:	// System call # 23
		         NachOS_Shutdown();
               break;
             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT( false );
                break;
         }
         returnFromSystemCall();
         break;

      case PageFaultException: {
         break;
      }

      case ReadOnlyException:
         printf( "Read Only exception (%d)\n", which );
         ASSERT( false );
         break;

      case BusErrorException:
         printf( "Bus error exception (%d)\n", which );
         ASSERT( false );
         break;

      case AddressErrorException:
         printf( "Address error exception (%d)\n", which );
         ASSERT( false );
         break;

      case OverflowException:
         printf( "Overflow exception (%d)\n", which );
         ASSERT( false );
         break;

      case IllegalInstrException:
         printf( "Ilegal instruction exception (%d)\n", which );
         ASSERT( false );
         break;

      default:
         printf( "Unexpected exception %d\n", which );
         ASSERT( false );
         break;
    }

}
