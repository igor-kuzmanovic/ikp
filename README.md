# LoadBalancing

## To do

- If a client disconnects from the LB and a new one connects, the server rejects it  
- When LB is gracefully stopped the WorkerClientRequestDispatcherThread can still hold an unprocessed message  
- Request serialization/deserialization is insane, fix it later, find all `send` and `recv` functions and remove `BUFFER_SIZE`

## Potential improvements

- The socket functions `send` and `recv` can, respectively, send and receive less bytes than requested.  
  This can be solved by sending the size of the message before sending the message itself.  
  This way the sender/receiver can know how many bytes to send/receive and keep sending/receiving until it has received all of them.  
- `WorkerClientRequestDispatcherThread` can be improved by waiting for 2 signals on each iterator.  
  One for getting the next message and another for getting the next worker.  
  This would require changing the API of both data structures to allow for calling take and get without them using `WaitForSingleObject` internally.  
- We use multiple threads for handling worker notifications because assume that the number of workers will be small and the number of notifications high.  
  This allows us to handle many notifications at the same time.  
  If the number of workers was large, we should use a single thread that uses a `fd_set` to receive notifications from all workers by utilizing non-blocking sockets.  
- We use multiple threads for handling client requests because we assume that the number of clients will be small and the number of requests high.  
  This allows us to handle many requests at the same time.  
  If the number of clients was large, we should use a single thread that uses a `fd_set` to receive requests from all clients by utilizing non-blocking sockets.  