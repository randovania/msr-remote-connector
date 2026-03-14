#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include <stdint.h>

typedef struct GameManager GameManager;           /* Opaque — access via helpers below                  */

static inline void* gm_blackboard(GameManager* gm) { return (void*)((uintptr_t)gm + 0x4a8); }

/* GAMEMANAGER_PPTR: 0x0076f314 → GameManager*                                */
#define GAMEMANAGER_PPTR ((GameManager**) 0x0076f314)

#endif // GAME_MANAGER_H
