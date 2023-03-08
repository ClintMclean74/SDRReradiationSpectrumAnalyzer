#include "GNU_Radio_Utilities.h"
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <errno.h>


const char* GNU_Radio_Utilities::GNU_RADIO_XMLRPC_SERVER_ADDRESS = "127.0.0.1";
const uint32_t GNU_Radio_Utilities::GNU_RADIO_XMLRPC_SERVER_ADDRESS_PORT = 54321;

const uint32_t GNU_Radio_Utilities::GNU_RADIO_DATA_STREAMING_ADDRESS_PORT = 4321;

void GNU_Radio_Utilities::CreateSocket(char* callingFunction, char* result)
{
    ////SOCKET sd;
	////struct sockaddr_in serv_addr;

	struct sockaddr_in serv_addr2;

	if (sd)
        CloseSocket(sd);

	memset((char *) &serv_addr, '\0', sizeof(serv_addr));

    serv_addr.sin_addr.s_addr = inet_addr(GNU_RADIO_XMLRPC_SERVER_ADDRESS);
    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons(GNU_RADIO_XMLRPC_SERVER_ADDRESS_PORT);

	sd = socket(AF_INET, SOCK_STREAM, 0);

	if (sd == -1)
	{
		UnitializeSockets();

		printf(callingFunction);
		printf(": ");

        printf("Could not create socket\n");// to GNU RADIO server\nlaunch GNURadioDeviceFlowgraph.grc for connecting to SDRs other than rtl sdrs\n");

        strcpy(result, "Socket Error");
		////return "Socket Error";
    }

    /*////printf(callingFunction);
    printf(": ");

    printf("CallXMLRPC(): Socket created\n");
    */

    strcpy(result, "");
    ////return "";
}

bool GNU_Radio_Utilities::CallXMLRPC(const char* data, char* result)
{
    CreateSocket("", result);

    int socketResult = connect(sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

    if (socketResult < 0)
    {
        char textBuffer[1000];
        memset(textBuffer, 0, 1000);
        sprintf(textBuffer, "%s\n", strerror(errno));

        UnitializeSockets();

        printf("Could not connect socket to GNU RADIO server\nlaunch GNURadioDeviceFlowgraph.grc for connecting to SDRs other than rtl sdrs\n");

        strcpy(result, "Socket Error");

		////return "Socket Error";
		return false;
    }

    char xmlrpcHTMLstr[1000];
    memset(xmlrpcHTMLstr, 0, 1000);

    char xmlrpcstr[255];
    memset(xmlrpcstr, 0, 255);
    strcpy(xmlrpcstr, data);

    uint32_t xmlrpcstrLength = strlen(xmlrpcstr);
    sprintf(xmlrpcHTMLstr, "POST / HTTP/1.1\nHost: http://169.254.81.236\nContent-Length: %i\nContent-Type: text/plain\r\n\r\n%s", xmlrpcstrLength, xmlrpcstr);

    WriteDataToSocket(sd, xmlrpcHTMLstr, strlen(xmlrpcHTMLstr));

    const int RECEIVE_BUFF_LENGTH = 1000;
    char receivedBuffer[RECEIVE_BUFF_LENGTH];
    memset(receivedBuffer, 0, RECEIVE_BUFF_LENGTH);

    int bytes_received;

    bytes_received = recv(sd, receivedBuffer, RECEIVE_BUFF_LENGTH, 0);


    strcpy(result, receivedBuffer);

    ////std::string receivedBufferStr(receivedBuffer);

    ////return receivedBuffer;
    ////return receivedBufferStr;

    return true;
}
