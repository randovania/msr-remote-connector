#include <string.h>
#include "remote_connector_api.h"
#include "lua_hook.h"

// lua functions
void (*luaL_register)(void *lua_state, const char *libname, const void *lua_reg) = (void (*)(void*, const char*, const void*)) LUAL_REGISTER_MEM;
const char* (*lua_tolstring)(void *lua_state, int idx, size_t* len) = (const char* (*)(void*, int, size_t*)) LUAL_TOLSTRING_MEM;
void (*lua_getfield) (void *lua_state, int idx, const char *k) = (void(*)(void*, int, const char*)) LUA_GETFIELD;
void (*lua_settop) (void* lua_state, int idx) = (void(*)(void*, int)) LUA_POP;
// LUA_API void lua_pushvalue (lua_State *L, int idx) {
void (*lua_call) (void* lua_state, int nargs, int nresults) = (void(*)(void*, int, int)) LUA_CALL;
void (*lua_pushinteger) (void* lua_state, int n) = (void(*)(void*, int)) LUA_PUSHINTEGER;
void (*lua_pushstring) (void* lua_state, const char* s) = (void(*)(void*, const char*)) LUA_PUSHSTRING;
int (*lua_pcall) (void* lua_state, int nargs, int nresults, int errfunc) = (int(*)(void*, int, int, int)) LUA_PCALL;
int (*luaL_loadbuffer) (void* lua_state, const char* buff, size_t size, const char* name) = (int(*)(void*, const char*, size_t, const char*)) LUAL_LOADBUFFER;
void (*lua_setfield) (void* lua_state, int idx, const char *k) = (void(*)(void*, int, const char*)) LUA_SETFIELD;

// lua defines
#define lua_getglobal(L,s)lua_getfield(L, 0xffffd8ee, (s))
#define lua_pop(L,n)		lua_settop(L, -(n)-1)

// forward declaration
int send_inventory(void* lua_state);
int remote_connector_init(void* lua_state);
int remote_update(void* lua_state);
int send_gamelog(void* lua_state);
int send_indices(void* lua_state);

static const luaL_Reg multiworld_lib[] = {
  {"Init", remote_connector_init},
  {"Update", remote_update},
  {"SendLog", send_gamelog},
  {"SendInventory", send_inventory},
  {"SendIndices", send_indices},
  {NULL, NULL}  
};


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


int remote_connector_init(void* lua_state) {
    create_remote_connector_thread();
    multiworld_schedule_update(lua_state);
    return 0;
}


/* This functions is called perodically from the game and calls RemoteApi::ProcessCommand with a callback function */
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


/* Gets called by lua to send the inventory */
int send_inventory(void* lua_state) {
    // TODO: combine reading string to one function?
    size_t inventory_size = 0;    // length of the lua string response (without \0)
    const char* inventory_message = lua_tolstring(lua_state, 1, &inventory_size);
    handle_send_inventory(inventory_message, inventory_size);
    return 0;
}


/* Gets called by lua to send the indices of the already collected locations */
int send_indices(void* lua_state) {
    size_t indices_size = 0;    // length of the lua string response (without \0)
    const char* indices_message = lua_tolstring(lua_state, 1, &indices_size);
    handle_send_indices(indices_message, indices_size);
    return 0;
}


/* Gets called by lua to send a log message from Game.LogWarn */
int send_gamelog(void* lua_state) {
    if (client_subs.logging) {
        size_t log_size = 0;    // length of the lua string response (without \0)
        const char* log_message = lua_tolstring(lua_state, 1, &log_size);
        handle_log_message(log_message, log_size);
    }
    return 0;
}


void lua_hook(void* lua_state) {
    luaL_register(lua_state, "RL", multiworld_lib);

    lua_pushinteger(lua_state, 1);
    lua_setfield(lua_state, -2, "Version");

    lua_pushinteger(lua_state, SIZE_RECV_BUFFER);
    lua_setfield(lua_state, -2, "BufferSize");

    return;
}