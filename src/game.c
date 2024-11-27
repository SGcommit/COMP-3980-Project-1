#include "../include/game.h"
#include <stdio.h>

static Context               context;          // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t quit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// Signal handler for graceful termination on SIGINT (Ctrl+C)
void handle_signal(int signal)
{
    (void)signal;
    quit_flag = 1;
}

// Sleeps for a delay of 3 seconds
static void sleepForDelay(void)
{
    sleep(3);
}
// Main entry point of the program
// Parses the command-line arguments, initializes the socket and game loop
// Handles communication between host and client based on the mode (host or client)
int main(int argc, char *argv[])
{
    struct sockaddr_storage addr;
    socklen_t               addr_len = sizeof(addr);
    int                     sockfd;
    char                    buffer[BUFFER_SIZE];
    const char             *ip_address = NULL;
    char                   *port       = NULL;
    uint32_t                randomSeed;

    randomSeed = arc4random();
    srand(randomSeed);

    // Parse arguments
    parse_arguments(argc, argv, &ip_address, &port);

    // Initialize addr structure
    memset(&addr, 0, sizeof(addr));    // Zero out the addr structure

    // Create socket
    sockfd = socket_create(AF_INET, SOCK_DGRAM, 0);

    // Set up the connection
    setupConnection(&sockfd, &addr, ip_address, port);

    signal(SIGINT, handle_signal);    // Handle Ctrl+C to quit

    while(!quit_flag)
    {
        ssize_t     bytes;
        const char *packet = NULL;
        int         x;
        int         y;

        if(context.is_host)
        {
            // Host waits for a packet before responding
            bytes = receiveUDPMessage(sockfd, &addr, &addr_len, buffer, sizeof(buffer));
            if(bytes > 0)
            {
                printf("Host received packet: %s\n", buffer);

                // Generate random coordinates for the host
                generateRandomCoordinates(&x, &y);
                printf("Host sending coordinates: x = %d, y = %d\n", x, y);

                // Respond to sender with coordinates
                packet = createPacket(x, y, "response");
                sendUDPMessage(sockfd, &addr, addr_len, packet);
            }
        }
        else
        {
            // Client sends first
            generateRandomCoordinates(&x, &y);
            printf("Client sending coordinates: x = %d, y = %d\n", x, y);

            packet = createPacket(x, y, "game_active");
            sendUDPMessage(sockfd, &addr, addr_len, packet);

            // Wait for server response
            bytes = receiveUDPMessage(sockfd, &addr, &addr_len, buffer, sizeof(buffer));
            if(bytes > 0)
            {
                printf("Client received packet: %s\n", buffer);
            }
        }

        sleepForDelay();    // Delay before the next update
    }

    // Close the socket
    printf("Exiting...\n");
    socket_close(sockfd);
    return 0;
}

// Sets up the connection between the host and client
// Initializes the address structure and binds to the port if hosting or sets up connection if connecting
int setupConnection(const int *sockfd, struct sockaddr_storage *addr, const char *ip_address, const char *port)
{
    memset(addr, 0, sizeof(*addr));    // Zero out address structure

    if(ip_address == NULL)
    {
        in_port_t parsed_port;
        // Hosting mode
        printf("No IP address provided. Hosting the game...\n");
        context.is_host = true;

        // Bind to the port
        parsed_port     = parse_in_port_t("game", port);
        addr->ss_family = AF_INET;
        socket_bind(*sockfd, addr, parsed_port);
    }
    else
    {
        // Connecting mode
        printf("Attempting to connect to %s:%s...\n", ip_address, port);
        context.is_host = false;

        // Convert address and set as destination
        convert_address(ip_address, addr);

        // Set the port
        if(addr->ss_family == AF_INET)
        {
            ((struct sockaddr_in *)addr)->sin_port = htons(parse_in_port_t("game", port));
        }
        else if(addr->ss_family == AF_INET6)
        {
            ((struct sockaddr_in6 *)addr)->sin6_port = htons(parse_in_port_t("game", port));
        }
        else
        {
            fprintf(stderr, "Unsupported address family\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

// void initializeGame()
//{
//     const char *ip_address = "127.0.0.1";    // Replace with actual IP
//     const char *port       = "12345";        // Replace with actual port
//
//     initializeNetwork(ip_address, port);
//     setStartingPositions();
//     setupNcurses();
// }
//
// void setupNcurses()
//{
//     // Initialize ncurses for GUI
// }

// Initializes the network connection with the provided IP address and port
// Converts IP address to sockaddr_storage and binds the socket to the specified port for UDP communication
void initializeNetwork(const char *ip_address, const char *port)
{
    struct sockaddr_storage addr;
    in_port_t               parsed_port;

    // Parse and convert the port string
    parsed_port = parse_in_port_t("game", port);

    // Convert the provided IP address to sockaddr_storage
    convert_address(ip_address, &addr);

    // Create the UDP socket
    context.socket = socket_create(addr.ss_family, SOCK_DGRAM, 0);

    // Bind to the specified address and port
    socket_bind(context.socket, &addr, parsed_port);

    printf("Network initialized on %s:%s\n", ip_address, port);
}

// Parses the command-line arguments to extract the IP address and port
// Handles options and validates the number of arguments to correctly assign values for the IP address and port
void parse_arguments(int argc, char *argv[], const char **ip_address, char **port)
{
    int opt;
    int remaining_args;

    opterr = 0;

    while((opt = getopt(argc, argv, "h")) != -1)
    {
        switch(opt)
        {
            case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
            case '?':
                usage(argv[0], EXIT_FAILURE, "Unknown option.");
            default:
                usage(argv[0], EXIT_FAILURE, NULL);
        }
    }

    remaining_args = argc - optind;

    if(remaining_args == 1)
    {
        // Only a port is provided
        *ip_address = NULL;
        *port       = argv[optind];
    }
    else if(remaining_args == 2)
    {
        // Both IP and port are provided
        *ip_address = argv[optind];
        *port       = argv[optind + 1];
    }
    else
    {
        usage(argv[0], EXIT_FAILURE, "Provide either a port to host or an IP and port to connect.");
    }
}

// Handles the parsed arguments for the network connection
// Ensures both IP address and port are provided for a successful connection
void handle_arguments(const char *binary_name, const char *ip_address, const char *port_str, in_port_t *port)
{
    if(ip_address == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "The ip address is required.");
    }

    if(port_str == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "The port is required.");
    }

    *port = parse_in_port_t(binary_name, port_str);
}

// Parses the port string to convert it to a valid `in_port_t` type
// Converts the port string to a numerical `in_port_t` type, ensuring the value is within valid range
in_port_t parse_in_port_t(const char *binary_name, const char *str)
{
    char     *endptr;
    uintmax_t parsed_value;

    parsed_value = strtoumax(str, &endptr, BASE_TEN);
    errno        = 0;
    if(errno != 0)
    {
        perror("Error parsing in_port_t");
        exit(EXIT_FAILURE);
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        usage(binary_name, EXIT_FAILURE, "Invalid characters in input.");
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_value > UINT16_MAX)
    {
        usage(binary_name, EXIT_FAILURE, "in_port_t value out of range.");
    }

    return (in_port_t)parsed_value;
}

// Converts an IP address (IPv4/IPv6) string into a sockaddr_storage structure
// Translates a given IP address to a network address structure for later socket communication
void convert_address(const char *address, struct sockaddr_storage *addr)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        addr->ss_family = AF_INET6;
    }
    else
    {
        fprintf(stderr, "%s is not an IPv4 or an IPv6 address\n", address);
        exit(EXIT_FAILURE);
    }
}

// Creates a UDP socket for communication
// Creates a socket of the specified type and domain (IPv4/IPv6) for communication
int socket_create(int domain, int type, int protocol)
{
    int sockfd;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}
// Binds the socket to the specified address and port
void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        fprintf(stderr, "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: %d\n", addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

// UDP communication functions
void sendUDPMessage(int sockfd, const struct sockaddr_storage *dest_addr, socklen_t addr_len, const char *message)
{
    if(sendto(sockfd, message, strlen(message), 0, (const struct sockaddr *)dest_addr, addr_len) == -1)
    {
        perror("Failed to send message");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

ssize_t receiveUDPMessage(int sockfd, struct sockaddr_storage *source_addr, socklen_t *addr_len, char *buffer, size_t buffer_size)
{
    ssize_t bytes_received;

    // Initialize addr_len if needed
    if(addr_len && *addr_len == 0)
    {
        *addr_len = sizeof(struct sockaddr_storage);
    }

    // Wait to receive a message
    bytes_received = recvfrom(sockfd, buffer, buffer_size - 1, 0, (struct sockaddr *)source_addr, addr_len);

    if(bytes_received == -1)
    {
        perror("recvfrom");
        return -1;    // Return an error instead of exiting the program
    }

    // Null-terminate the received data
    buffer[bytes_received] = '\0';

    // Print and process the received message
    printf("Received packet: %s\n", buffer);

    // Example: Call a function to process the message
    updateRemoteDot(buffer);

    return bytes_received;
}

#pragma GCC diagnostic pop

char *createPacket(int x, int y, const char *game_state)
{
    static char packet[BUFFER_SIZE];
    snprintf(packet, sizeof(packet), "%d,%d|%s", x, y, game_state);
    return packet;
}

void handleReceivedPacket(const char *packet)
{
    printf("Processing packet: %s\n", packet);
    updateRemoteDot(packet);    // Example of how packet might be handled
}

void updateRemoteDot(const char *packet)
{
    char *endptr;
    long  x = strtol(packet, &endptr, BASE_TEN);

    if(*endptr == ',')
    {
        long y = strtol(endptr + 1, &endptr, BASE_TEN);
        if(*endptr == '\0')
        {
            context.remotex = (int)x;
            context.remotey = (int)y;
            return;
        }
    }
}

// Utility function
_Noreturn void usage(const char *program_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }
    fprintf(stderr, "Usage: %s [-h] <IP address> <port>\n", program_name);
    fprintf(stderr, "Options:\n  -h  Display this help message\n");
    exit(exit_code);
}

// Function to generate random coordinates
void generateRandomCoordinates(int *x, int *y)
{
    *x = rand() % GAME_GRID_SIZE;    // Random x between 0 and 99
    *y = rand() % GAME_GRID_SIZE;    // Random y between 0 and 99
}

// void setStartingPositions()
//{
//     // Initialize starting positions for local and remote players
// }
//
// void handleInput()
//{
//     // Process user input and update local position
// }
//
// int getUserInput()
//{
//     // Read user input
//     return 0;
// }
//
// void updateLocalDot(int ch)
//{
//     // Update local dot's position based on input
// }
//
// void sendPositionUpdate()
//{
//     // Send local position to the remote player
// }

// void receivePositionUpdate()
//{
//     // Receive position update from the remote player
// }
//
// void updateScreen()
//{
//    // Refresh the screen to show updated positions
//}
//
// void clearScreen()
//{
//    // Clear the screen
//}
//
// void drawDot(int x, int y, char symbol)
//{
//    // Draw a dot at the given coordinates
//}
//
// void cleanup()
//{
//    // Free resources and exit
//}
//
// void errorMessage(const char *msg)
//{
//    // Print error message and exit
//}
