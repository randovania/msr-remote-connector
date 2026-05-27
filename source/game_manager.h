#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <stdint.h>

typedef struct GameManager GameManager;           /* Opaque — access via helpers below                  */
typedef void* (*get_component_for_entity_fn)(void* entity, void** componentType);

static inline void* gm_blackboard(GameManager* gm) { return (void*)((uintptr_t)gm + 0x4a8); }
static inline void**   gm_entity_list (GameManager* gm) { return *(void***)((uintptr_t)gm + 0x18); }
static inline uint32_t gm_entity_count(GameManager* gm) { return (*(uint32_t*)((uintptr_t)gm + 0x20)) & 0x3FFFFFFF; }

/* GAMEMANAGER_PPTR: 0x0076f314 → GameManager*                                */
#define GAMEMANAGER_PPTR ((GameManager**) 0x0076f314)
/* FUN_001e9878 — GetComponentForEntity_: find component by type string       */
#define get_component_for_entity ((get_component_for_entity_fn)0x001e9878)

/* TString pool helpers (shared engine functions) */
typedef void (*hash_string_fn)  (void** result, const char* text);
typedef void (*trash_tstring_fn)(void** str);

/* FUN_001b2d94 — HashString: intern string in ref-counted pool              */
#define hash_string   ((hash_string_fn)   0x001b2d94)
/* FUN_001b3180 — TrashTStringInstancePlus___: decrement refcount, free if 0 */
#define trash_tstring ((trash_tstring_fn) 0x001b3180)

#endif // GAME_MANAGER_H
