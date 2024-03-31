#ifndef LUA_HOOK_H
#define LUA_HOOK_H

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


typedef int (*lua_CFunction) (void* lua_state);

typedef struct luaL_Reg {
  const char* name;
  lua_CFunction func;
} luaL_Reg;


#endif // LUA_HOOK_H
