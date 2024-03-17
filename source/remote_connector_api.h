#ifndef REMOTECONNECTOR_API_H
#define REMOTECONNECTOR_API_H

#include <3ds.h>
#include <atomic>
#include<pthread.h>

#define SIZE_RECV_BUFFER 4096
#define SIZE_SEND_BUFFER 4096
#define SERVER_PORT 42069

extern s32 client_sock;
extern uint8_t recv_buffer[SIZE_RECV_BUFFER];
extern void handle_inventory(const char* lua_string, size_t string_size);
extern void handle_remote_lua_exec(const char* lua_result, size_t result_size, bool output_success);
extern void handle_log_message(const char* log_message, size_t log_size);
extern void handle_send_inventory(const char* log_message, size_t log_size);
extern void create_remote_connector_thread(void);
extern std::atomic_bool ready_for_game_thread;
extern pthread_cond_t lua_done;
extern pthread_mutex_t mutex;
/**
 * Packet structure:
 * PACKET_HANDSHAKE:
 * 0: PACKET_HANDSHAKE
 * 1: request_number
 * 2-5: payload length
 * 6-9: payload (SIZE_RECV_BUFFER)
 * 
 * TOOD: REST ;)
*/
enum Packet {
  PACKET_HANDSHAKE = 1,
  PACKET_LOG_MESSAGE = 2,
  PACKET_REMOTE_LUA_EXEC = 3,
  PACKET_NEW_INVENTORY = 5,
};

// Client's interest. e.g. logging is only forwarded to client if it was set in handshake
struct ClientSubscriptions {
    bool logging;
    bool multiworld;
};

extern struct ClientSubscriptions client_subs;


#endif // REMOTECONNECTOR_API_H