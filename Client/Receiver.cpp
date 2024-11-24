#include "Receiver.h"

DWORD WINAPI StartReceiver(LPVOID lpParam) {
    // Variable used to store function return value
    int iResult = 0;

    // Expected result
    const char* expectedResult = "success";

    // Buffer used for receiving messages
    char* receiveBuffer = (char*)malloc(BUFFER_SIZE + sizeof(char));

    // Socket used for communicating with server
    Connection connection = *(Connection*)lpParam;

    for (int i = 0; i < 5; i++) {
        // Receive data until the server shuts down the connection
        iResult = ReceiveData(&connection, receiveBuffer, BUFFER_SIZE);
        if (iResult > 0) {
            printf("Message received from server: %s.\n", receiveBuffer);
        }
        else if (iResult == 0) {
            // Connection was closed gracefully
            printf("Connection with server closed.\n");

            break;
        }
        else {
            break;
        }
    }

    // Free the memory used by the receive buffer
    free(receiveBuffer);

    return 0;
}