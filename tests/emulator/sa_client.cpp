// SAProject.cpp : Defines the entry point for the console application.
//

#include "common.h"

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUFSIZE 512

HANDLE hPipe; 
char* lpszPipename = "\\\\.\\pipe\\mynamedpipe"; 

bool communicationInitializeAsClient()
{
   bool waitingForCreation = false;
   while (1) 
   { 
      hPipe = CreateFileA( 
         lpszPipename,   // pipe name 
         GENERIC_READ |  // read and write access 
         GENERIC_WRITE, 
         0,              // no sharing 
         NULL,           // default security attributes
         OPEN_EXISTING,  // opens existing pipe 
         0,              // default attributes 
         NULL);          // no template file 
 
   // Break if the pipe handle is valid. 
 
      if (hPipe != INVALID_HANDLE_VALUE) 
         break; 

	  if ( GetLastError() == ERROR_FILE_NOT_FOUND )
	  {
		  waitingForCreation = true;
		  printf(".");
		  Sleep(1000);
		  continue;
	  }
 
      // Exit if an error other than ERROR_PIPE_BUSY occurs. 
 
      if (GetLastError() != ERROR_PIPE_BUSY) 
      {
         printf( "Could not open pipe. GLE=%d\n", GetLastError() ); 
         return false;
      }
 
      // All pipe instances are busy, so wait for 20 seconds. 
 
      if ( ! WaitNamedPipeA(lpszPipename, 20000)) 
      { 
         printf("Could not open pipe: 20 second wait timed out."); 
         return false;
      } 
   } 
   if ( waitingForCreation )
	   printf( "\n\n" );

   return true;
}

void communicationTerminate()
{
   CloseHandle(hPipe); 
   hPipe = INVALID_HANDLE_VALUE;
}

bool sendMessage( const unsigned char * buff, int size )
{
   BOOL   fSuccess = FALSE; 
   DWORD  cbToWrite, cbWritten; 
	unsigned char sizePacked[2];
	sizePacked[0] = size & 0xFF;
	sizePacked[1] = (size>>8) & 0xFF;

   fSuccess = WriteFile( 
      hPipe,                  // pipe handle 
      sizePacked,             // message 
      2,              // message length 
      &cbWritten,             // bytes written 
      NULL);                  // not overlapped 

   if ( ! fSuccess) 
   {
      printf( "WriteFile to pipe failed. GLE=%d\n", GetLastError() ); 
      return false;
   }

   fSuccess = WriteFile( 
      hPipe,                  // pipe handle 
      buff,             // message 
      size,              // message length 
      &cbWritten,             // bytes written 
      NULL);                  // not overlapped 

   if ( ! fSuccess) 
   {
      printf( "WriteFile to pipe failed. GLE=%d\n", GetLastError() ); 
      return false;
   }

   return true;
}

int getMessage( unsigned char * buff, int maxSize )
{
   BOOL   fSuccess = FALSE; 
   DWORD  cbRead, cbToWrite, cbWritten, dwMode; 
   int msgSize = 0; 
   int byteCnt = 0;

   do
   {
	   do 
	   { 
	   // Read from the pipe. 
 
		  fSuccess = ReadFile( 
			 hPipe,    // pipe handle 
			 buff + byteCnt,    // buffer to receive reply 
			 1,  // size to read 
			 &cbRead,  // number of bytes read 
			 NULL);    // not overlapped 
 
		  if ( !fSuccess )
			 break; 
 
		  byteCnt ++; 

		  if ( byteCnt == 2 )
		  {
			  msgSize = buff[1];
			  msgSize <<= 8;
			  msgSize += buff[0];
			  break;
		  }
	   } 
	   while (1); 

		if ( !fSuccess )
		break; 

		byteCnt = 0;
	   do 
	   { 
	   // Read from the pipe. 
		  if ( byteCnt >= msgSize )
		  {
			  printf( "\"%s\"\n", buff );
			  break;
		  }
 
		  fSuccess = ReadFile( 
			 hPipe,    // pipe handle 
			 buff + byteCnt,    // buffer to receive reply 
			 1,  // size to read 
			 &cbRead,  // number of bytes read 
			 NULL);    // not overlapped 
 
		  if ( !fSuccess )
			 break; 
 
		  byteCnt ++; 

	   } 
	   while (1); 
	   break;
   }
   while (1);

   return msgSize;
}
 





int main(int argc, char *argv[]) 
{ 
	printf( "starting CLIENT...\n" );
	printf( "==================\n\n" );

   char* lpvMessage="Default message from client."; 
//   TCHAR  chBuf[BUFSIZE]; 
   BOOL   fSuccess = FALSE; 
   DWORD  cbRead, cbToWrite, cbWritten, dwMode; 

   if( argc > 1 )
      lpvMessage = argv[1];
 
// Try to open a named pipe; wait for it, if necessary. 

   fSuccess = communicationInitializeAsClient();
   if ( !fSuccess )
	   return -1;
 
 
// Send a message to the pipe server. 
   cbToWrite = strlen(lpvMessage)+1;
 
   printf( "Sending %d byte message: \"%s\"\n", cbToWrite, lpvMessage); 

   fSuccess = sendMessage( (unsigned char *)lpvMessage, cbToWrite );

/*   fSuccess = WriteFile( 
      hPipe,                  // pipe handle 
      lpvMessage,             // message 
      cbToWrite,              // message length 
      &cbWritten,             // bytes written 
      NULL);                  // not overlapped 
*/
   if ( ! fSuccess) 
   {
      printf( "WriteFile to pipe failed. GLE=%d\n", GetLastError() ); 
      return -1;
   }

   printf("\nMessage sent to server, receiving reply as follows:\n");

   unsigned char rwBuff[BUFSIZE];
   int msgSize = getMessage( rwBuff, BUFSIZE );




   if ( ! fSuccess)
   {
      printf( "ReadFile from pipe failed. GLE=%d\n", GetLastError() );
      return -1;
   }

   printf("\n<End of message, press ENTER to terminate connection and exit>");
   _getch();
 
   communicationTerminate(); 
 
   return 0; 
}

