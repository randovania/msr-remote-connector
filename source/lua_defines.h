#ifndef LUA_DEFINES_H
#define LUA_DEFINES_H

#include <3ds.h>

#define LUAL_REGISTER_MEM 0x0012e02c
#define LUAL_TOLSTRING_MEM 0x001c3e88
#define LUA_GETFIELD  0x0015c150
#define LUA_POP 0x001a3444
#define LUA_PUSH_VALUE 0x001a3764
#define LUA_CALL 0x0012e670
#define LUA_PUSHINTEGER 0x0017f304
// should be lua_pushnumber but only takes one param??? FUN_0017f168
#define LUA_PUSHSTRING 0x0017f188
#define LUA_PCALL 0x001a39ac
#define LUAL_LOADBUFFER 0x00227138
#define LUA_SETFIELD 0x0014234c
#define LUA_PUSH_BOOLEAN 0x0017f2d8
#define LUA_TOBOOLEAN 0x00215868
#define LUA_TONUMBER 0x001c3cd8

// lua defines
#define lua_getglobal(L,s)lua_getfield(L, 0xffffd8ee, (s))
#define lua_pop(L,n)		lua_settop(L, -(n)-1)

// lua functions
extern void (*luaL_register)(void* lua_state, const char* libname, const void* lua_reg);
extern const char* (*lua_tolstring)(void* lua_state, int idx, size_t* len);
extern void (*lua_getfield) (void* lua_state, int idx, const char* k);
extern void (*lua_settop) (void* lua_state, int idx);
extern void (*lua_call) (void* lua_state, int nargs, int nresults);
extern void (*lua_pushinteger) (void* lua_state, int n);
extern void (*lua_pushstring) (void* lua_state, const char* s);
extern int (*lua_pcall) (void* lua_state, int nargs, int nresults, int errfunc);
extern int (*luaL_loadbuffer) (void* lua_state, const char* buff, size_t size, const char* name);
extern void (*lua_setfield) (void* lua_state, int idx, const char* k);
extern void (*lua_pushboolean) (void* lua_state, int b);
extern int (*lua_toboolean)(void* lua_state, int idx);
extern float (*lua_tonumber)(void* lua_state, int idx);

typedef int (*lua_CFunction) (void* lua_state);

typedef struct luaL_Reg {
    const char* name;
    lua_CFunction func;
} luaL_Reg;


#endif // LUA_DEFINES_H
