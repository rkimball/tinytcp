Introduction:
tinytcp is designed primarily for use in an embedded environment. It is designed for simplicity and deterministic memory usage.
All memory used is preallocated in static arrays.

Usage:
The protocol stack has three main functions required:
1) NetworkInterface.RxData
2) NetworkInterface.TxData
3) ProtocolTCP::Tick()

Create a new listener:
   ListenerConnection = ProtocolTCP::NewServer( port );
   while( 1 )
   {
      connection = ListenerConnection->Listen();

      // Spawn off a thread to handle this connection
      page = (HTTPPage*)PagePool.Get();
      if( page )
      {
         page->Initialize( connection );
         page->Thread.Create( ConnectionHandlerEntry, "Page", 1024, 100, page );
      }
      else
      {
         printf( "Error: Out of pages\n" );
      }
   }
