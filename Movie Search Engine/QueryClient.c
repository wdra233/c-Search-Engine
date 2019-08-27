#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "includes/QueryProtocol.h"
#include "QueryClient.h"

char *port_string = "1501";
unsigned int port;
char *ip = "127.0.0.1";
#define BUFFER_SIZE 1000
#define LIMIT_QUERY 100
#define ADDR_BUFFER 100
#define ARG_NUM 3
char ipBuffer[ADDR_BUFFER];
char portBuffer[ADDR_BUFFER];

// Edited by Yawang Wang since Apr 9.
// Completed on Apr 21.

void RunQuery(char *query) {
    if (strlen(query) > LIMIT_QUERY) {
        printf("You should limit the query to 100 characters.\n");
        exit(1);
    }
    char buffer[BUFFER_SIZE];
    int str_len;

    // Find the address

    // Create the socket
    int s;
    struct addrinfo address, *result;
    memset(&address, 0, sizeof(struct addrinfo));
    address.ai_family = AF_INET; /* IPv4 only */
    address.ai_socktype = SOCK_STREAM; /* TCP */

    s = getaddrinfo(ipBuffer, portBuffer, &address, &result);
    if (s != 0) {
        printf("Invalid IP address, please check your IP address again.\n");
        printf("Valid IP address example: XXX.XXX.XXX.XXX\n");
        exit(1);
    }
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Connect to the server
    // First, set timeout to socket connection
    struct timeval timeout;
    // After 3 seconds connect() will timeout.
    timeout.tv_sec = 3;
    timeout.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    printf("Connecting....Please wait patiently\n\n");
    if (connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1) {
        printf("Connection timeout, the following might be the reason, "
        "please check carefully.\n");
        printf("1. Invalid IP address or port number.\n");
        printf("2. Server might break down. "
        "Please wait patiently until everything comes back to normal.\n");
        freeaddrinfo(result);
        exit(1);
    }
    freeaddrinfo(result);
    printf("Connected to movie server.\n");
    // Do the query-protocol
    str_len = read(sock_fd, buffer, sizeof(buffer) - 1);
    if (str_len == -1) {
        printf("No message from server! Please checkout your network!\n");
        exit(1);
    }
    buffer[str_len] = '\0';
    if (CheckAck(buffer) == -1) {
        printf("No Acknowledgement from server. "
        "Sorry, you're not allowed to connect to the server.\n");
        exit(1);
    }
    if (send(sock_fd, query, strlen(query), 0) < 0) {
        printf("Fail to send the query to the server\n");
        exit(1);
    }
//    memset(buffer,0,BUFFER_SIZE);
    str_len = read(sock_fd, buffer, sizeof(buffer) - 1);
    if (str_len == -1) {
        printf("No message from server!\n");
        exit(1);
    }
    buffer[str_len] = '\0';
    int num = atoi(buffer);
    if (SendAck(sock_fd) == -1) {
        printf("Unable to send message to the server, "
        "please check your network!\n");
        exit(1);
    }
    printf("%d items found in the query: \n", num);
    if (num == 0) {
        printf("Item not found in the server! "
        "Please try again with another query.\n\n");
        printf("FOR EXAMPLE: If you want to get all movie "
               "titles that contain Seattle, type \"seattle\" to query!\n\n");
    }
    while (num > 0) {
        str_len = read(sock_fd, buffer, sizeof(buffer) - 1);
        if (str_len == -1) {
            printf("No message from server! Please checkout your network!\n");
            exit(1);
        }
        buffer[str_len] = '\0';
        printf("%s\n", buffer);
        if (SendAck(sock_fd) == -1) {
            printf("Unable to send message to the server, "
            "please check your network!\n");
            exit(1);
        }
        num--;
    }
    // Close the connection
    str_len = read(sock_fd, buffer, sizeof(buffer) - 1);
    if (str_len == -1) {
        printf("No message from server! Please checkout your network!\n");
        exit(1);
    }
    buffer[str_len] = '\0';
    if (CheckGoodbye(buffer) == -1) {
        printf("Fail to receive GoodBye message from the server!\n");
    }
    close(sock_fd);
}

void RunPrompt() {
    char input[BUFFER_SIZE];

    while (1) {
        printf("Enter a term to search for, or q to quit: ");
        scanf("%s", input);

        printf("input was: %s\n", input);

        if (strlen(input) == 1) {
            if (input[0] == 'q') {
                printf("Thanks for playing! \n");
                return;
            }
        }
        printf("\n\n");
        RunQuery(input);
    }
}

int main(int argc, char **argv) {
    // Check/get arguments
    if (argc != ARG_NUM) {
        printf("Incorrect number of arguments! "
        "The number of arguments should be 3.\n");
        printf("Server is started by calling: "
        "./queryclient [ipaddress] [port]\n");
        exit(1);
    }
    // Get info from user
    if (strcmp(argv[1], "localhost")) {
        snprintf(ipBuffer, sizeof(ipBuffer), "%s", argv[1]);
    } else {
        snprintf(ipBuffer, sizeof(ipBuffer), "%s", ip);
    }
    snprintf(portBuffer, sizeof(portBuffer), "%s", argv[2]);
    // Run Query
    RunPrompt();
    return 0;
}
