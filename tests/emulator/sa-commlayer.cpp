/*******************************************************************************
    Copyright (c) 2015, OLogN Technologies AG. All rights reserved.
    Redistribution and use of this file in source and compiled
    forms, with or without modification, are permitted
    provided that the following conditions are met:
        * Redistributions in source form must retain the above copyright
          notice, this list of conditions and the following disclaimer.
        * Redistributions in compiled form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
        * Neither the name of the OLogN Technologies AG nor the names of its
          contributors may be used to endorse or promote products derived from
          this software without specific prior written permission.
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
    DAMAGE
*******************************************************************************/

#include "sa-commlayer.h"
#include <stdio.h> 


#define BUFSIZE 512

#ifdef _MSC_VER

#include <Windows.h>

HANDLE hPipe; 
char* g_pipeName = "\\\\.\\pipe\\mynamedpipe"; 

bool communicationInitializeAsServer()
{
      hPipe = CreateNamedPipeA( 
          g_pipeName,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
		  PIPE_TYPE_BYTE | PIPE_READMODE_BYTE |
          PIPE_WAIT,                // blocking mode 
          PIPE_UNLIMITED_INSTANCES, // max. instances  
          BUFSIZE,                  // output buffer size 
          BUFSIZE,                  // input buffer size 
          0,                        // client time-out 
          NULL);                    // default security attribute 

      if (hPipe == INVALID_HANDLE_VALUE) 
      {
          printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError()); 
          return false;
      }
 
      // Wait for the client to connect; if it succeeds, 
      // the function returns a nonzero value. If the function
      // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 
 
      bool connected = ConnectNamedPipe(hPipe, NULL) ? 
	  TRUE : (GetLastError() == ERROR_PIPE_CONNECTED); 

	  if ( !connected )
		  CloseHandle(hPipe); 

	  return connected;
}

bool communicationInitializeAsClient()
{
   bool waitingForCreation = false;
   while (1) 
   { 
      hPipe = CreateFileA( 
         g_pipeName,   // pipe name 
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
 
      if ( ! WaitNamedPipeA(g_pipeName, 20000)) 
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
   DWORD  cbWritten; 
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

int getMessage( unsigned char * buff, int maxSize ) // returns -1 on error
{
   BOOL   fSuccess = FALSE; 
   DWORD  cbRead; 
   int msgSize = -1; 
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
//			  printf( "\"%s\"\n", buff );
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

#else // _MSC_VER
#error not implemented
#endif
 