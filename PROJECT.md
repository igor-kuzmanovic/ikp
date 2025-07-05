# Load Balancing

Develop a service for storing data received from clients. The service consists of one Load Balancer (LB) component and an arbitrary number (N) of Worker (WR) components.  
The functions of the components are as follows:

● **Load Balancer (LB):**  
○ Listens for requests on port 5059 and receives storage requests from clients.  
○ Directs each request to a WR component using the Round Robin algorithm.  
○ Obtains information about available WRs, as they automatically register upon startup and deregister upon shutdown.  
○ When a new WR is added to the system, the LB redistributes the data so that all WR components have the same data.

● **Worker (WR):**  
○ When it receives data from the LB, it stores it in its local space and sends a notification to all other WR components about the received data.  
○ Based on the notifications it receives from other WRs, each WR updates its space so that it holds the complete set of all data ever received by the system.  
○ Notifies the LB when it has successfully stored the data.
