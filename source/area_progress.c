/* =============================================================================
 *  The minimap per-area item-% is drawn by UpdateMinimapAreaProgressLabel
 *  (Ghidra 0x00485fd0) as:  percent = GAME.<MapName>_PROGRESS / DENOM * 100
 *  where DENOM is normally a HARDCODED inline per-area constant
 *  (surface=14 area1=14 area2=18 area3=21 area4=21 area6=23 area7=12 area9=13
 *  area10=14).
 * 
 *  This hook replaces that constant with GAME_PROGRESS.<MapName>_PROGRESS — a
 *  blackboard UInt32 the rando sets from Lua
 *
 * ============================================================================= */
#include <stdint.h>
#include <string.h>
#include "game_manager.h"

typedef struct {                                  /* TStringPoolEntry: char* at +0, length at +4        */
    const char* str;
    uint32_t    len;
} TString;

typedef struct {                                  /* Blackboard property slot — 0xc bytes               */
    uint32_t crc;                                 /* +0x0 — key CRC                                     */
    uint32_t arg;                                 /* +0x4 — data offset argument                        */
    uint8_t  type;                                /* +0x8 — dataType id                                 */
    uint8_t  _pad[3];
} BBParam;

typedef void     (*combine_tstrings_fn)(TString* out, const TString* a, const TString* b);
typedef void     (*wipe_tstring_fn)    (TString* str);
typedef void*    (*find_bb_entry_fn)   (void* blackboard, uint32_t* sectionCrc);
typedef void     (*bb_extract_sel_fn)  (void* lpField30, uint32_t arg, int z);
typedef void     (*bb_extract_get_fn)  (void* lpField30, void* out, int size);
typedef uint32_t (*crc32_fn)           (const char* str, int len, uint32_t init);

/* FUN_001b32b8 — CombineTStrings: concatenate two interned strings           */
#define combine_tstrings ((combine_tstrings_fn) 0x001b32b8)
/* FUN_001b28e8 — wipe/release a TString built by CombineTStrings             */
#define wipe_tstring     ((wipe_tstring_fn)     0x001b28e8)
/* FUN_001c5428 — locate a blackboard section entry by section CRC            */
#define find_bb_entry    ((find_bb_entry_fn)    0x001c5428)
/* FUN_001d1240 — select a LuaParameters property by its data-offset arg      */
#define bb_extract_sel   ((bb_extract_sel_fn)   0x001d1240)
/* FUN_001d1230 — copy the selected property's value out                      */
#define bb_extract_get   ((bb_extract_get_fn)   0x001d1230)
/* FUN_001d0248 — CRC32: standard CRC32 with init=0xFFFFFFFF                  */
#define crc32            ((crc32_fn)            0x001d0248)

#define PROGRESS_STRID      (*(TString**)0x00707c38)   /* "_PROGRESS"                                */
#define GAME_PROGRESS_STRID (*(TString**)0x007083a0)   /* GAME_PROGRESS section; +0x10 = section CRC */
#define UINT32_TYPE_ID      0x69


uint32_t get_area_denom(const TString* mapName) {
    GameManager* gm  = *GAMEMANAGER_PPTR;
    void* blackboard = gm_blackboard(gm);

    // create the crc for "<MapName>_PROGRESS"
    TString key = { 0, 0 };
    combine_tstrings(&key, mapName, PROGRESS_STRID);
    uint32_t keyCrc = crc32(key.str, strlen(key.str), 0xFFFFFFFF);
    wipe_tstring(&key);

    // locate the GAME_PROGRESS section entry
    uint32_t sectionCrc = *(uint32_t*)((uint8_t*)GAME_PROGRESS_STRID + 0x10);
    void* entry = find_bb_entry(blackboard, &sectionCrc);
    if (entry == NULL) return 1;

    // walk this section's property array for keyCrc, read it as UInt32
    uint8_t* lua_parameters  = (uint8_t*)entry + 4;
    // lua_parameters + 0 contains a pointer to a contiguous array of BBParam
    BBParam* arr = *(BBParam**)(lua_parameters + 0);
    uint32_t idx = *(uint32_t*)(lua_parameters + 4);
    BBParam* end = arr + idx;
    BBParam* p   = arr;
    while (p != end && p->crc != keyCrc) p++;
    // return if not found, else continue
    if (p == end || p->type != UINT32_TYPE_ID) return 1;


    uint32_t value = 0;
    bb_extract_sel(lua_parameters + 0x30, p->arg, 0);
    bb_extract_get(lua_parameters + 0x30, &value, 4);

    return value ? value : 1;
}
