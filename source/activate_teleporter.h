#ifndef ACTIVATE_TELEPORTER_H
#define ACTIVATE_TELEPORTER_H

/*
 * activate_teleporter.h — Metroid: Samus Returns (3DS) ExeFS.elf
 * Game struct layouts, function pointer types, and resolved VAs (Ghidra ARM32).
 */
#pragma once
#include <stdint.h>

/* ---- Forward declarations ------------------------------------------------ */
typedef struct MinimapScenarioWrapper MinimapScenarioWrapper;

/* ---- Game struct layouts -------------------------------------------------
 * Fields confirmed from Ghidra decompilation. _pad* = unconfirmed bytes.
 * All offsets are from the start of the struct.
 * -------------------------------------------------------------------------- */

typedef struct {                                  /* MapCellStruct_ — 8 bytes, fully known              */
    uint8_t Enum;                                 /* 0=hidden  2=visited-corridor  3=visited-content    */
    uint8_t _pad[3];
    void*   Substruct;                            /* NULL=corridor, non-NULL=content room               */
} MapCellStruct;

typedef struct {                                  /* Embedded in CMinimapScenario at +0x13c             */
    MapCellStruct** CellArray;
    void*           RawCoords;
} CoordinatesStruct;

typedef struct {                                  /* CRC at +0x10                                       */
    uint8_t  _pad[0x10];
    uint32_t CRC;
} ScenarioEntry;

typedef struct {                                  /* CurrentScenario at +0x04, Width at +0x6c           */
    uint8_t        _pad0[4];
    ScenarioEntry* Current;                       /* +0x04 — MapInfo->CurrentScenario                   */
    uint8_t        _pad1[0x64];
    uint32_t       Width;                         /* +0x6c                                              */
} MapInfo;

typedef struct {                                  /* Info at +0x134, Dirty at +0x138, Coords at +0x13c  */
    uint8_t           _pad0[0x134];
    MapInfo*          Info;                       /* +0x134                                             */
    uint8_t           Dirty;                      /* +0x138 — set=1 before calling serialize_cells      */
    uint8_t           _pad1[3];
    CoordinatesStruct Coords;                     /* +0x13c — embedded; pass &Coords to map_coordinates */
} CMinimapScenario;

struct MinimapScenarioWrapper {                   /* Scenario at +0x4, Next at +0x8                     */
    uint8_t                  _pad[4];
    CMinimapScenario*        Scenario;            /* +0x4                                               */
    MinimapScenarioWrapper*  Next;                /* +0x8                                               */
};

typedef struct {                                  /* FirstWrapper at +0x1d4                             */
    uint8_t                  _pad[0x1d4];
    MinimapScenarioWrapper*  FirstWrapper;        /* +0x1d4 — CMinimap->CMinimapBase->MinimapScenarioWrapper */
} CMinimap;

typedef struct {                                  /* Discovered at +0x204                               */
    uint8_t _pad[0x204];
    uint8_t Discovered;                           /* +0x204                                             */
} CTeleporterUsableComponent;

typedef struct GameManager GameManager;           /* Opaque — access via helpers below                  */

static inline void*    gm_blackboard  (GameManager* gm) { return (void*)((uintptr_t)gm + 0x4a8); }
static inline void**   gm_entity_list (GameManager* gm) { return *(void***)((uintptr_t)gm + 0x18); }
static inline uint32_t gm_entity_count(GameManager* gm) { return (*(uint32_t*)((uintptr_t)gm + 0x20)) & 0x3FFFFFFF; }

/* ---- Function pointer types (AAPCS / ARM32) ------------------------------ */
typedef void           (*hash_string_fn)            (void** result, const char* text);
typedef void           (*trash_tstring_fn)          (void** str);
typedef void           (*register_teleporter_fn)    (void* blackboard, uint32_t* crc, void** tp, void** area);
typedef void*          (*get_component_for_entity_fn)(void* entity, void** componentType);
typedef void*          (*ref_to_teleporter_class_fn) (void);
typedef int            (*compare_object_to_class_fn) (void* obj, void* classRef);
typedef uint32_t       (*crc32_fn)                  (const char* str, int len, uint32_t init);
typedef void           (*set_blackboard_flag_fn)    (void* section, uint32_t* sectionCRC, void** flagName, char* value);
typedef void           (*discover_scenario_fn)      (CMinimapScenario* scenario);
typedef CMinimap*      (*get_minimap_fn)            (GameManager* gm);
typedef void           (*mark_cell_fn)              (MapCellStruct* cell, int createVisual, CMinimapScenario* scenario);
typedef void           (*serialize_cells_fn)        (CMinimapScenario* scenario, void* blackboard);
typedef void*          (*render_cell_bg_fn)         (MapCellStruct* cell, CMinimapScenario* scenario, int flag);
typedef void           (*spawn_cell_icon_fn)        (CMinimapScenario* scenario, MapCellStruct* cell, void* blackboard);
typedef MapCellStruct* (*map_coordinates_fn)        (CoordinatesStruct* coords, int index);

/* ---- Resolved function pointers (Ghidra VAs) ----------------------------- */
/* FUN_001b2d94 — HashString: intern string in ref-counted pool              */
#define hash_string              ((hash_string_fn)             0x001b2d94)
/* FUN_001b3180 — TrashTStringInstancePlus___: decrement refcount, free if 0 */
#define trash_tstring            ((trash_tstring_fn)           0x001b3180)
/* FUN_006a5358 — write teleporter+area into blackboard "teleport_locations"  */
#define register_teleporter      ((register_teleporter_fn)     0x006a5358)
/* FUN_001e9878 — GetComponentForEntity_: find component by type string       */
#define get_component_for_entity ((get_component_for_entity_fn)0x001e9878)
/* FUN_00125dec — RefToCTeleporterUsableComponent: returns vtable class ref   */
#define ref_to_teleporter_class  ((ref_to_teleporter_class_fn) 0x00125dec)
/* FUN_001eb828 — CompareObjectToClass_: runtime vtable type check            */
#define compare_object_to_class  ((compare_object_to_class_fn) 0x001eb828)
/* FUN_001d0248 — CRC32: standard CRC32 with init=0xFFFFFFFF                 */
#define crc32                    ((crc32_fn)                    0x001d0248)
/* FUN_001db19c — SetBlackboardFlag: write UInt8 (bool) to blackboard key     */
#define set_blackboard_flag      ((set_blackboard_flag_fn)      0x001db19c)
/* FUN_004e6de4 — mark all cells in scenario as visited, set _discovered flag */
#define discover_scenario        ((discover_scenario_fn)        0x004e6de4)
/* FUN_001ae614 — GameManager → CIngameMenu → CMinimap (two Y_ dereferences) */
#define get_minimap              ((get_minimap_fn)              0x001ae614)
/* FUN_004a355c — set cell Enum=2 or 3; spawns visual if createVisual=1       */
#define mark_cell                ((mark_cell_fn)                0x004a355c)
/* FUN_0018d56c — flush cell enum array and marker grid to blackboard         */
#define serialize_cells          ((serialize_cells_fn)          0x0018d56c)
/* FUN_001af310 — render tile background color (must call after mark_cell)    */
#define render_cell_bg           ((render_cell_bg_fn)           0x001af310)
/* FUN_001f5aec — spawn content icon; for teleporters: "teleporter" or "teleporteroff" */
#define spawn_cell_icon          ((spawn_cell_icon_fn)          0x001f5aec)
/* FUN_001edf40 — MapCoordinatesFunction_: cell at linear index (y*w+x)       */
#define map_coordinates          ((map_coordinates_fn)          0x001edf40)

/* ---- Global data references ----------------------------------------------
 * GAMEMANAGER_PPTR:    0x0076f314 → GameManager* (one indirection; inner address confirmed)
 * TELEPORT_LOCS_PPPTR: 0x0024135c → P → TStringPoolEntry; +0x10 = CRC32("teleport_locations")
 * USABLE_STR_PTR:      address of CTeleporterUsableComponent type TStringInstance*
 * -------------------------------------------------------------------------- */
#define GAMEMANAGER_PPTR    ((GameManager**) 0x0076f314)
#define TELEPORT_LOCS_PPPTR ((uintptr_t*)    0x0024135c)
#define USABLE_STR_PTR      ((void**)        0x00708f3c)

/* ---- Table entry types --------------------------------------------------- */
typedef struct {
    const char* teleporter;
    const char* area;
    int         cellX;    /* minimap column; -1 = not yet known */
    int         cellY;    /* minimap row;    -1 = not yet known */
} TeleporterEntry;

typedef struct {
    const char* subArea;
    const char* primaryArea;
    const char* sectionStr;  /* pipe-separated sub-areas; CRC32'd as blackboard section key */
} AreaSectionEntry;

/* ---- Public API ---------------------------------------------------------- */
int activate_teleporter(const char* teleporterName);

#endif // ACTIVATE_TELEPORTER_H
