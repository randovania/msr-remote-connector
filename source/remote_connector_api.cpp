#include <string.h>
#include <malloc.h>
#include <arpa/inet.h>
#include "remote_connector_api.h"
#include <cstdlib>


// aligned section for soc:u service
u32 __attribute__((section (".socServiceBufferSection"))) socServiceBuffer[4096];

// no idea why the recv_thread_stack needs to be on the heap but otherwise
// the socket functions just fails
u8* recv_thread_stack;
uint8_t recv_buffer[SIZE_RECV_BUFFER];
uint8_t send_buffer[SIZE_SEND_BUFFER];

Handle recv_thread = -1;

s32 server_sock = -1;
s32 client_sock = -1;
uint8_t request_number = 0;

std::atomic_bool ready_for_game_thread = { false };
pthread_cond_t lua_done = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct ClientSubscriptions client_subs;

// forward declaration
void parse_client_packet(int length);



/* Inits the server by using socInit and create server socket by using standard Berkeley socket api calls */
int init_server() {
    int ret;

    struct sockaddr_in server;
    memset(&server, 0, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;

    // init soc services
    if ((ret = socInit(socServiceBuffer, 4096*4)) != 0) {
        return -1;
    }

    // create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0) {
        return -1;
    }

    setsockopt((s32)&server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ret = bind(server_sock, (struct sockaddr *) &server, sizeof (server));
    if (ret != 0) {
        return -1;
    }

    ret = listen(server_sock, 2);
    if (ret != 0) {
        return -1;
    }
    return 0;
}


/* Listen and receive loop. Also the entrypoint of the thread */
void listen_and_receive_function(void* argv) {
    int ret;
    socklen_t clientlen;
    struct sockaddr_in client;
    memset(recv_buffer, 0, SIZE_RECV_BUFFER);
    
    // nothing we can do
    if (init_server() != 0) return;

    clientlen = sizeof(client);
    memset(&client, 0, sizeof (client));

    while (server_sock != -1) {
        pthread_mutex_lock(&mutex);
        client_subs.logging = false;
        client_subs.multiworld = false;
        client_sock = accept(server_sock, (struct sockaddr *)&client, &clientlen);
        request_number = 0;
        while ((ret = recv (client_sock, recv_buffer, SIZE_RECV_BUFFER, 0)) > 0) {
            parse_client_packet(ret);

            while (ready_for_game_thread.load()) {
                pthread_cond_wait(&lua_done, &mutex);
            }
            memset(recv_buffer, 0, SIZE_RECV_BUFFER);
        }
        close(client_sock);
        client_sock = -1;
        pthread_mutex_unlock(&mutex);
    }
    socExit();
    srvExit();
    svcExitThread();
    return;
}


/* Sends the "buffer" via the socket, iff it is connected */
void send_packet(uint8_t* buffer, int length) {
    if (client_sock != -1) {
        send(client_sock, buffer, length, 0);
    }
}


/* Handles handshake from the client */
void handle_handshake() {
    const char interest_byte = recv_buffer[1];
    client_subs.logging = interest_byte & 0x1;
    client_subs.multiworld = (interest_byte & 0x2) >> 1;

    memset(send_buffer, 0, SIZE_SEND_BUFFER);
    // packet type
    memset(send_buffer, 1, 1);
    // request_number
    memset(send_buffer + 1, request_number, 1);
    send_packet(send_buffer, 2);
    request_number++;
}


/* Handles remote lua execution */
void handle_remote_lua_exec(const char* lua_result, size_t result_size, bool output_success) {
    memset(send_buffer, PACKET_REMOTE_LUA_EXEC, 1);
    memset(send_buffer + 1, request_number, 1);
    memset(send_buffer + 2, output_success, 1);
    memcpy(send_buffer + 3, &result_size, 4);
    memcpy(send_buffer + 7, lua_result, result_size);
    send_packet(send_buffer, 7 + result_size);
    request_number++;
}


/* Handles nearly all messages which structure is: packet_type, message_size, message */
void handle_generic_message(uint8_t packet_type, const char* message, size_t message_size) {
    memset(send_buffer, packet_type, 1);
    memcpy(send_buffer + 1, &message_size, 4);
    memcpy(send_buffer + 5, message, message_size);
    send_packet(send_buffer, 5 + message_size);
}


/* Handles malformed packets */
void handle_malformed_packet(uint8_t packet_type, int received_bytes, int should_bytes) {
    memset(send_buffer, PACKET_MALFORMED, 1);
    memset(send_buffer + 1, packet_type, 1);
    memcpy(send_buffer + 2, &received_bytes, 4);
    memcpy(send_buffer + 6, &should_bytes, 4);
    send_packet(send_buffer, 10);
}


/* Parses packets from the client. Client actively only sends a handshake or a remote lua execution */
void parse_client_packet(int length) {
    if (ready_for_game_thread.load() || length == 0) return;
    switch (recv_buffer[0]) {
        case PACKET_HANDSHAKE:
            if (length == 2) handle_handshake();
            else handle_malformed_packet(PACKET_HANDSHAKE, length, 2);
        break;
        case PACKET_REMOTE_LUA_EXEC:
            // lua strings can be long, we may receive it in chunks
            // ^ true but a FIXME for now
            if (length < 5) handle_malformed_packet(PACKET_REMOTE_LUA_EXEC, length, 5);
            int lua_string_length = 0;
            memcpy(&lua_string_length, recv_buffer + 1, 4);
            if (length != lua_string_length + 5) handle_malformed_packet(PACKET_REMOTE_LUA_EXEC, length, lua_string_length + 5);
            else ready_for_game_thread.store(true);
        break;
    
    }
    return;
}


/* Shutsdown the soc service. Doesn't do anything on citra because "atexit" isn't called */
void soc_shutdown() {
    if (client_sock != -1) close(client_sock);
    if (server_sock != -1) close(server_sock);
    server_sock = -1;
    client_sock = -1;
}


/* Creates the remote connector thread. Is called by lua via RL.Init -> "remote_connector_init" */
void create_remote_connector_thread() {
    // init service api from libctru
    srvInit();
    recv_thread_stack = (u8*) memalign(8, SIZE_RECV_BUFFER);
    u8* top = recv_thread_stack + 4096;
    svcCreateThread(&recv_thread, listen_and_receive_function, 0, (u32*)top, 30, 1);
    atexit(soc_shutdown);
}
