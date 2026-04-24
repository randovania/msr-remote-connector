#ifndef MINIMAP_H
#define MINIMAP_H

#include <stdint.h>
#include "game_manager.h"

/* ---- Forward declarations ------------------------------------------------ */
typedef struct MinimapScenarioWrapper MinimapScenarioWrapper;

/* ---- Game struct layouts -------------------------------------------------
 * Fields confirmed from Ghidra decompilation. _pad* = unconfirmed bytes.
 * All offsets are from the start of the struct.
 * -------------------------------------------------------------------------- */

typedef struct {                                  /* Info at +0x134, Dirty at +0x138                    */
    uint8_t           _pad0[0x138];
    uint8_t           Dirty;                      /* +0x138 — set=1 before calling serialize_cells      */
    uint8_t           _pad1[3];
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

/* ---- Function pointer types (AAPCS / ARM32) ------------------------------ */
typedef CMinimap* (*get_minimap_fn)      (GameManager* gm);
typedef void      (*serialize_cells_fn)  (CMinimapScenario* scenario, void* blackboard);

/* ---- Resolved function pointers (Ghidra VAs) ----------------------------- */
/* FUN_001ae614 — GameManager → CIngameMenu → CMinimap (two Y_ dereferences) */
#define get_minimap      ((get_minimap_fn)     0x001ae614)
/* FUN_0018d56c — flush cell enum array and marker grid to blackboard         */
#define serialize_cells  ((serialize_cells_fn) 0x0018d56c)

#endif // MINIMAP_H
