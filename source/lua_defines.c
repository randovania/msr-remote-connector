#include "lua_defines.h"

void (*luaL_register)(void* lua_state, const char* libname, const void* lua_reg) = (void (*)(void*, const char*, const void*)) LUAL_REGISTER_MEM;
const char* (*lua_tolstring)(void* lua_state, int idx, size_t* len) = (const char* (*)(void*, int, size_t*)) LUAL_TOLSTRING_MEM;
void (*lua_getfield) (void* lua_state, int idx, const char* k) = (void(*)(void*, int, const char*)) LUA_GETFIELD;
void (*lua_settop) (void* lua_state, int idx) = (void(*)(void*, int)) LUA_POP;
void (*lua_call) (void* lua_state, int nargs, int nresults) = (void(*)(void*, int, int)) LUA_CALL;
void (*lua_pushinteger) (void* lua_state, int n) = (void(*)(void*, int)) LUA_PUSHINTEGER;
void (*lua_pushstring) (void* lua_state, const char* s) = (void(*)(void*, const char*)) LUA_PUSHSTRING;
int (*lua_pcall) (void* lua_state, int nargs, int nresults, int errfunc) = (int(*)(void*, int, int, int)) LUA_PCALL;
int (*luaL_loadbuffer) (void* lua_state, const char* buff, size_t size, const char* name) = (int(*)(void*, const char*, size_t, const char*)) LUAL_LOADBUFFER;
void (*lua_setfield) (void* lua_state, int idx, const char* k) = (void(*)(void*, int, const char*)) LUA_SETFIELD;
void (*lua_pushboolean) (void* lua_state, int b) = (void(*)(void*, int)) LUA_PUSH_BOOLEAN;
int (*lua_toboolean)(void* lua_state, int idx) = (int (*)(void*, int)) LUA_TOBOOLEAN;
float (*lua_tonumber)(void* lua_state, int idx) = (float (*)(void*, int)) LUA_TONUMBER;