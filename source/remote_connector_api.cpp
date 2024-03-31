#include <string.h>
#include <malloc.h>
#include <arpa/inet.h>
#include "remote_connector_api.h"


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

void parse_client_packet();

// TODO: Replace while(true)
void init_server() {
    int ret;

    struct sockaddr_in server;
    memset(&server, 0, sizeof (server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;

    // init soc services
    if ((ret = socInit(socServiceBuffer, 4096)) != 0) {
        while(true){}
    }

    // create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock < 0) {
        while(true){}
    }

    setsockopt((s32)&server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ret = bind(server_sock, (struct sockaddr *) &server, sizeof (server));
    if (ret != 0) {
        while(true){}
    }

    ret = listen(server_sock, 2);
    if (ret != 0) {
        while(true){}
    }

}

void listen_and_receive_function(void* argv) {
    int ret;
    socklen_t clientlen;
    struct sockaddr_in client;
    memset(recv_buffer, 0, SIZE_RECV_BUFFER);
    
    init_server();

    clientlen = sizeof(client);
    memset(&client, 0, sizeof (client));

    while (true) {
        pthread_mutex_lock(&mutex);
        client_subs.logging = false;
        client_subs.multiworld = false;
        client_sock = accept(server_sock, (struct sockaddr *)&client, &clientlen);
        request_number = 0;
        while ((ret = recv (client_sock, recv_buffer, SIZE_RECV_BUFFER, 0)) > 0) {
            parse_client_packet();

            while (ready_for_game_thread.load()) {
			    pthread_cond_wait(&lua_done, &mutex);
            }
            memset(recv_buffer, 0, SIZE_RECV_BUFFER);
        }
        close(client_sock);
        client_sock = -1;
        pthread_mutex_unlock(&mutex);
    }

    return;
}


void send_packet(uint8_t* buffer, int length) {
    if (client_sock != -1) {
        request_number++;
        send(client_sock, buffer, length, 0);
    }
}


void handle_handshake() {
    const char interest_byte = recv_buffer[1];
    client_subs.logging = interest_byte & 0x1;
    // TODO: Use it 
    client_subs.multiworld = (interest_byte & 0x2) >> 1;

    memset(send_buffer, 0, SIZE_SEND_BUFFER);
    // packet type
    memset(send_buffer, 1, 1);
    // request_number
    memset(send_buffer + 1, request_number, 1);
    send_packet(send_buffer, 2);
}


void handle_inventory(const char* lua_string, size_t string_size) {
    memset(send_buffer, 0, SIZE_SEND_BUFFER);
    // packet type
    memset(send_buffer, PACKET_NEW_INVENTORY, 1);
    // request_number
    memset(send_buffer + 1, request_number, 1);
    memcpy(send_buffer + 2, &string_size, 4);
    memcpy(send_buffer + 6, lua_string, string_size);
    send_packet(send_buffer, 6 + string_size);
}


void handle_remote_lua_exec(const char* lua_result, size_t result_size, bool output_success) {
    memset(send_buffer, PACKET_REMOTE_LUA_EXEC, 1);
    memset(send_buffer + 1, request_number, 1);
    memset(send_buffer + 2, output_success, 1);
    memcpy(send_buffer + 3, &result_size, 4);
    memcpy(send_buffer + 7, lua_result, result_size);
    send_packet(send_buffer, 7 + result_size);
}


void handle_generic_message(uint8_t packet_type, const char* message, size_t message_size) {
    memset(send_buffer, packet_type, 1);
    memcpy(send_buffer + 1, &message_size, 4);
    memcpy(send_buffer + 5, message, message_size);
    send_packet(send_buffer, 5 + message_size);
}


// TODO: Add length checks
void parse_client_packet() {
    if (ready_for_game_thread.load()) return;
    switch (recv_buffer[0]) {
        case PACKET_HANDSHAKE:
            handle_handshake();
        break;
        case PACKET_REMOTE_LUA_EXEC:
            // lua strings can be long, we may receive it in chunks
            // ^ true but a FIXME for now
            ready_for_game_thread.store(true);
        break;
    
    }
    return;
}


void create_remote_connector_thread() {
    // init service api from libctru
    srvInit();
    recv_thread_stack = (u8*) malloc(SIZE_RECV_BUFFER);
	svcCreateThread(&recv_thread, listen_and_receive_function, 0, (u32*)recv_thread_stack, 30, 1);
}
