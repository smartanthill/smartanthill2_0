#include <windows.h> 
#include <stdio.h> 
//#include <strsafe.h>

#define BUFSIZE 512

HANDLE hPipe; 
char* lpszPipename = "\\\\.\\pipe\\mynamedpipe"; 

bool communicationInitializeAsServer()
{
      hPipe = CreateNamedPipeA( 
          lpszPipename,             // pipe name 
          PIPE_ACCESS_DUPLEX,       // read/write access 
//          PIPE_TYPE_MESSAGE |       // message type pipe 
//          PIPE_READMODE_MESSAGE |   // message-read mode 
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
 


















DWORD WINAPI InstanceThread(LPVOID); 
VOID GetAnswerToRequest(unsigned char*, unsigned char*, LPDWORD); 
 
int main(VOID) 
{ 
	printf( "starting server...\n" );

   BOOL   fConnected = FALSE; 
   DWORD  dwThreadId = 0; 
 
// The main loop creates an instance of the named pipe and 
// then waits for a client to connect to it. When the client 
// connects, a thread is created to handle communications 
// with that client, and this loop is free to wait for the
// next client connect request. It is an infinite loop.
 
      printf( "\nPipe Server: Main thread awaiting client connection on %s\n", lpszPipename);
	  if ( !communicationInitializeAsServer() )
          return -1;
 
         printf("Client connected.\n"); 

		 unsigned char rwBuff[ BUFSIZE ];
      
   for (;;) 
   { 
		int msgSize = getMessage( rwBuff, BUFSIZE );

		printf( "Message from client received:\n" );
		printf( "\"%s\"\n", rwBuff );

		// do some nonsence to imitate processing
		unsigned char temp;
		for ( int i=0; i<msgSize/2-1; i++ )
		{
			temp = rwBuff[ i ];
			rwBuff[ i ] = rwBuff[ msgSize - 2 - i ];
			rwBuff[ msgSize - i ] = temp;
		}
		rwBuff[ msgSize - 1 ] = 0;

		// reply
	   bool fSuccess = sendMessage( (unsigned char *)rwBuff, msgSize );
	   if ( ! fSuccess) 
	   {
		  printf( "WriteFile to pipe failed. GLE=%d\n", GetLastError() ); 
		  return -1;
	   }
	   printf("\nMessage replied to client, reply is:\n");
		printf( "\"%s\"\n", rwBuff );

   } 

   return 0; 
} 
 


#if 0
DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{ 
   HANDLE hHeap      = GetProcessHeap();
   unsigned char* pchRequest = (unsigned char*)HeapAlloc(hHeap, 0, BUFSIZE*sizeof(unsigned char));
   unsigned char* pchReply   = (unsigned char*)HeapAlloc(hHeap, 0, BUFSIZE*sizeof(unsigned char));

   DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0; 
   BOOL fSuccess = FALSE;
   HANDLE hPipe  = NULL;

   // Do some extra error checking since the app will keep running even if this
   // thread fails.

   if (lpvParam == NULL)
   {
       printf( "\nERROR - Pipe Server Failure:\n");
       printf( "   InstanceThread got an unexpected NULL value in lpvParam.\n");
       printf( "   InstanceThread exitting.\n");
       if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
       if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
       return (DWORD)-1;
   }

   if (pchRequest == NULL)
   {
       printf( "\nERROR - Pipe Server Failure:\n");
       printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
       printf( "   InstanceThread exitting.\n");
       if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
       return (DWORD)-1;
   }

   if (pchReply == NULL)
   {
       printf( "\nERROR - Pipe Server Failure:\n");
       printf( "   InstanceThread got an unexpected NULL heap allocation.\n");
       printf( "   InstanceThread exitting.\n");
       if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
       return (DWORD)-1;
   }

   // Print verbose messages. In production code, this should be for debugging only.
   printf("InstanceThread created, receiving and processing messages.\n");

// The thread's parameter is a handle to a pipe object instance. 
 
   hPipe = (HANDLE) lpvParam; 

// Loop until done reading
   while (1) 
   { 
   // Read client requests from the pipe. This simplistic code only allows messages
   // up to BUFSIZE characters in length.
      fSuccess = ReadFile( 
         hPipe,        // handle to pipe 
         pchRequest,    // buffer to receive data 
         BUFSIZE*sizeof(unsigned char), // size of buffer 
         &cbBytesRead, // number of bytes read 
         NULL);        // not overlapped I/O 

      if (!fSuccess || cbBytesRead == 0)
      {   
          if (GetLastError() == ERROR_BROKEN_PIPE)
          {
              printf("InstanceThread: client disconnected.\n", GetLastError()); 
          }
          else
          {
              printf("InstanceThread ReadFile failed, GLE=%d.\n", GetLastError()); 
          }
          break;
      }

   // Process the incoming message.
      GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes); 
 
   // Write the reply to the pipe. 
      fSuccess = WriteFile( 
         hPipe,        // handle to pipe 
         pchReply,     // buffer to write from 
         cbReplyBytes, // number of bytes to write 
         &cbWritten,   // number of bytes written 
         NULL);        // not overlapped I/O 

      if (!fSuccess || cbReplyBytes != cbWritten)
      {   
          printf("InstanceThread WriteFile failed, GLE=%d.\n", GetLastError()); 
          break;
      }
  }

// Flush the pipe to allow the client to read the pipe's contents 
// before disconnecting. Then disconnect the pipe, and close the 
// handle to this pipe instance. 
 
   FlushFileBuffers(hPipe); 
   DisconnectNamedPipe(hPipe); 
   CloseHandle(hPipe); 

   HeapFree(hHeap, 0, pchRequest);
   HeapFree(hHeap, 0, pchReply);

   printf("InstanceThread exitting.\n");
   return 1;
}

VOID GetAnswerToRequest( unsigned char* pchRequest, 
                         unsigned char* pchReply, 
                         LPDWORD pchBytes )
// This routine is a simple function to print the client request to the console
// and populate the reply buffer with a default data string. This is where you
// would put the actual client request processing code that runs in the context
// of an instance thread. Keep in mind the main thread will continue to wait for
// and receive other client connections while the instance thread is working.
{
    printf( "Client Request String:\"%s\"\n", pchRequest );

    // Check the outgoing message to make sure it's not too long for the buffer.
	int size = sizeof("default answer from server");
	pchReply[0] = size & 0xFF;
	pchReply[1] = size >> 8;
    memcpy( pchReply + 2, "default answer from server", size );
    *pchBytes = size + 2;
}

#endif // 0