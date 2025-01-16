#include <string.h>
#include "remote_connector_api.h"
#include "lua_defines.h"
#include "rando_api.h"



// forward declaration
int send_inventory(void* lua_state);
int remote_connector_init(void* lua_state);
int remote_update(void* lua_state);
int send_gamelog(void* lua_state);
int send_indices(void* lua_state);
int send_new_game_state(void* lua_state);
int send_recv_pickups(void* lua_state);
int is_connected(void* lua_state);

/* Lua functions -> C functions */
static const luaL_Reg multiworld_lib[] = {
    {"Init", remote_connector_init},
    {"Update", remote_update},
    {"SendLog", send_gamelog},
    {"SendInventory", send_inventory},
    {"SendIndices", send_indices},
    {"SendReceivedPickups", send_recv_pickups},
    {"SendNewGameState", send_new_game_state},
    {"Connected", is_connected},
    {NULL, NULL}  
};


/* Schedules the next update on the next frame */
void multiworld_schedule_update(void* lua_state) {
    // +1
    lua_getglobal(lua_state, "Game");
    // +1
    lua_getfield(lua_state, -1, "AddGUISF");

    // +1
    lua_pushinteger(lua_state, 0);
    // +1
    lua_pushstring(lua_state, "RL.Update");
    // +1
    lua_pushstring(lua_state, "");

    // -4, +0
    lua_call(lua_state, 3, 0);
    // -1
    lua_pop(lua_state, 1);
}


/* Inits the remote connector. Called by RL.Update */
int remote_connector_init(void* lua_state) {
    create_remote_connector_thread();
    multiworld_schedule_update(lua_state);
    return 0;
}


/* This functions is called perodically from the game and processes the remote lua code */
int remote_update(void* lua_state) {
    if (ready_for_game_thread.load()) {
        pthread_mutex_lock(&mutex);
        /* Callback is executing the recv_buffer as lua code */
        size_t result_size = 0;           // length of the lua string response (without \0)
        bool output_success = false;      // was the lua function call sucessfully
        int lua_string_length = 0;
        const char* lua_result;

        // +1; use lua's tostring so we properly convert all types
        lua_getglobal(lua_state, "tostring");
        memcpy(&lua_string_length, recv_buffer + 1, 4);
        const char* lua_string = (const char*) (recv_buffer + 5);
        // +1
        int load_result = luaL_loadbuffer(lua_state, lua_string, lua_string_length, "remote lua");

        if (load_result == 0) {
            // -1, +1 - call the code we just loaded
            int pcall_result = lua_pcall(lua_state, 0, 1, 0);
            // -2, +1 - call tostring with the result of that
            lua_call(lua_state, 1, 1);
            // +0
            lua_result = lua_tolstring(lua_state, 1, &result_size);
            
            if (pcall_result == 0) {
                // success! top string is the entire result
                output_success = true;
            } 
        } else {
            const char* error_as_c_string = "error parsing buffer";
            lua_result = error_as_c_string;
            result_size = strlen(error_as_c_string);
        }
        handle_remote_lua_exec(lua_result, result_size, output_success);
        lua_pop(lua_state, 1);
        ready_for_game_thread.store(false);
        pthread_mutex_unlock(&mutex);
        pthread_cond_signal(&lua_done);
    }
    // Register calling update again
    multiworld_schedule_update(lua_state);
    return 0;
}


/* Generic function which reads the lua string from the stack and send it as provided packet_type */
int get_lua_string_and_send(void* lua_state, uint8_t packet_type) {
    size_t lua_string_size = 0;    // length of the lua string response (without \0)
    const char* lua_string_message = lua_tolstring(lua_state, 1, &lua_string_size);
    handle_generic_message(packet_type, lua_string_message, lua_string_size);
    return 0;
}


/* Gets called by lua to send the inventory */
int send_inventory(void* lua_state) {
    if (client_subs.multiworld) {
        return get_lua_string_and_send(lua_state, PACKET_NEW_INVENTORY);
    }
    return 0;
}


/* Gets called by lua to send the indices of the already collected locations */
int send_indices(void* lua_state) {
    if (client_subs.multiworld) {
        return get_lua_string_and_send(lua_state, PACKET_COLLECTED_INDICES);
    }
    return 0;
}


/* Gets called by lua to send current game state (main menu or in game) */
int send_new_game_state(void* lua_state) {
    if (client_subs.multiworld) {
        return get_lua_string_and_send(lua_state, PACKET_GAME_STATE);
    }
    return 0;
}


/* Gets called by lua to send the current amount of received pickups */
int send_recv_pickups(void* lua_state) {
    if (client_subs.multiworld) {
        return get_lua_string_and_send(lua_state, PACKET_RECEIVED_PICKUPS);
    }
    return 0;
}


/* Gets called by lua to send a log message from Game.LogWarn */
int send_gamelog(void* lua_state) {
    if (client_subs.logging) {
        return get_lua_string_and_send(lua_state, PACKET_LOG_MESSAGE);
    }
    return 0;
}


/* Gets called by lua to get current connection state */
int is_connected(void* lua_state) {
    lua_pushboolean(lua_state, client_sock != -1);
    return 1;
}


/* The lua hook functions used by the hks file */
void lua_hook(void* lua_state) {
    luaL_register(lua_state, "RL", multiworld_lib);

    lua_pushinteger(lua_state, 1);
    lua_setfield(lua_state, -2, "Version");

    lua_pushinteger(lua_state, SIZE_RECV_BUFFER);
    lua_setfield(lua_state, -2, "BufferSize");

    luaL_register(lua_state, "RandoApi", rando_api_lib);

    return;
}
