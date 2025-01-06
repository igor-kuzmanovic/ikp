# LoadBalancing

## To do

- If a client disconnects from the LB and a new one connects, the server rejects it  
- When LB is gracefully stopped the WorkerClientRequestDispatcherThread can still hold an unprocessed message  

## Potential improvements

- The socket functions `send` and `recv` can, respectively, send and receive less bytes than requested.  
  This can be solved by sending the size of the message before sending the message itself.  
  This way the sender/receiver can know how many bytes to send/receive and keep sending/receiving until it has received all of them.  
- 