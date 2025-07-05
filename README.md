# LoadBalancing

## To do

- If a client disconnects from the LB and a new one connects, the server rejects it  
- When LB is gracefully stopped the WorkerClientRequestDispatcherThread can still hold an unprocessed message  

## Potential improvements

- `WorkerClientRequestDispatcherThread` can be improved by waiting for 2 signals on each iterator.  
  One for getting the next message and another for getting the next worker.  
  This would require changing the API of both data structures to allow for calling take and get without them using `WaitForSingleObject` internally.  
- We use multiple threads for handling worker notifications because assume that the number of workers will be small and the number of notifications high.  
  This allows us to handle many notifications at the same time.  
  If the number of workers was large, we should use a single thread that uses a `fd_set` to receive notifications from all workers by utilizing non-blocking sockets.  
- We use multiple threads for handling client requests because we assume that the number of clients will be small and the number of requests high.  
  This allows us to handle many requests at the same time.  
  If the number of clients was large, we should use a single thread that uses a `fd_set` to receive requests from all clients by utilizing non-blocking sockets.  