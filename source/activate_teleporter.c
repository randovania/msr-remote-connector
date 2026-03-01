#include "activate_teleporter.h"
#include <string.h>

/*  Teleporter lookup table (name → area, minimap cell coordinates). */
static const TeleporterEntry TELEPORTER_TABLE[] = {
    { "LE_Teleporter_00_01",   "s000_surface",  20, 30 },
    { "LE_Teleporter_00b_01",  "s000_surface",  10, 10 },
    { "LE_Teleporter_01_01",   "s010_area1",    20, 20 },
    { "LE_Teleporter_02_01",   "s028_area2c",   -1, -1 },
    { "LE_Teleporter_02_02",   "s020_area2",    -1, -1 },
    { "LE_Teleporter_02_03",   "s020_area2",    -1, -1 },
    { "LE_Teleporter_02_04",   "s025_area2b",   -1, -1 },
    { "LE_Teleporter_03A_001", "s030_area3",    -1, -1 },
    { "LE_Teleporter_03A_002", "s030_area3",    -1, -1 },
    { "LE_Teleporter_03_01",   "s036_area3c",   -1, -1 },
    { "LE_Teleporter_03B_001", "s033_area3b",   -1, -1 },
    { "LE_Teleporter_03B_002", "s033_area3b",   -1, -1 },
    { "LE_Teleporter_04_01",   "s040_area4",    -1, -1 },
    { "LE_Teleporter_05_01",   "s050_area5",    -1, -1 },
    { "LE_Teleporter_06A_001", "s060_area6",    -1, -1 },
    { "LE_Teleporter_06C_001", "s067_area6c",   -1, -1 },
    { "LE_Teleporter_06B_001", "s065_area6b",   -1, -1 },
    { "LE_Teleporter_06A_002", "s060_area6",    -1, -1 },
    { "LE_Teleporter_07_01",   "s070_area7",    -1, -1 },
    { "LE_Teleporter_07_02",   "s070_area7",    -1, -1 },
    { "LE_Teleporter_09_01",   "s090_area9",    -1, -1 },
    { "LE_Teleporter_09_02",   "s090_area9",    -1, -1 },
    { "LE_Teleporter_10_01",   "s100_area10",   -1, -1 },
    { "LE_Teleporter_10_02",   "s100_area10",   -1, -1 },
};
#define TELEPORTER_COUNT ((int)(sizeof(TELEPORTER_TABLE) / sizeof(TELEPORTER_TABLE[0])))

/* --------------------------------------------------------------------------
 * Area section table (sub-area → primaryArea + blackboard sectionStr).
 *
 * sectionStr  = pipe-separated sub-areas sharing a blackboard section;
 *               CRC32(str, len, 0xFFFFFFFF) is the section key.
 * primaryArea = first sub-area; flag name = primaryArea + "_discovered".
 *
 * Area 2 confirmed (Lua: Blackboard.SetProp). Others inferred from naming.
 * -------------------------------------------------------------------------- */
static const AreaSectionEntry AREA_SECTION_TABLE[] = {
    { "s000_surface",  "s000_surface",  "s000_surface"                        },
    { "s010_area1",    "s010_area1",    "s010_area1"                          },
    { "s020_area2",    "s020_area2",    "s020_area2|s025_area2b|s028_area2c"  },
    { "s025_area2b",   "s020_area2",    "s020_area2|s025_area2b|s028_area2c"  },
    { "s028_area2c",   "s020_area2",    "s020_area2|s025_area2b|s028_area2c"  },
    { "s030_area3",    "s030_area3",    "s030_area3|s033_area3b|s036_area3c"  },
    { "s033_area3b",   "s030_area3",    "s030_area3|s033_area3b|s036_area3c"  },
    { "s036_area3c",   "s030_area3",    "s030_area3|s033_area3b|s036_area3c"  },
    { "s040_area4",    "s040_area4",    "s040_area4"                          },
    { "s050_area5",    "s050_area5",    "s050_area5"                          },
    { "s060_area6",    "s060_area6",    "s060_area6|s065_area6b|s067_area6c"  },
    { "s065_area6b",   "s060_area6",    "s060_area6|s065_area6b|s067_area6c"  },
    { "s067_area6c",   "s060_area6",    "s060_area6|s065_area6b|s067_area6c"  },
    { "s070_area7",    "s070_area7",    "s070_area7"                          },
    { "s090_area9",    "s090_area9",    "s090_area9"                          },
    { "s100_area10",   "s100_area10",   "s100_area10"                         },
};
#define AREA_SECTION_COUNT ((int)(sizeof(AREA_SECTION_TABLE) / sizeof(AREA_SECTION_TABLE[0])))

/* --------------------------------------------------------------------------
 * Register teleporter in the blackboard.
 * Mirrors one FUN_006a5358 call from ScanVisitDiscoverEverything.
 * Returns 1 on success, 0 if teleporter_name is not in the table.
 * -------------------------------------------------------------------------- */
int activate_teleporter_blackboard(const char* teleporter_name) {
    const char* area_name = NULL;
    for (int i = 0; i < TELEPORTER_COUNT; i++) {
        if (strcmp(TELEPORTER_TABLE[i].teleporter, teleporter_name) == 0) {
            area_name = TELEPORTER_TABLE[i].area;
            break;
        }
    }
    if (!area_name) return 0;

    GameManager* gm  = *GAMEMANAGER_PPTR;
    void* blackboard = gm_blackboard(gm);

    /* teleport_locations: 0x0024135c → P → TStringPoolEntry → +0x10 for CRC */
    uintptr_t tp_locs_entry = *(uintptr_t*)*TELEPORT_LOCS_PPPTR;
    uint32_t  tp_locs_crc   = *(uint32_t*)(tp_locs_entry + 0x10);

    void* area_str = NULL, *tp_str = NULL;
    hash_string(&area_str, area_name);
    hash_string(&tp_str,   teleporter_name);
    register_teleporter(blackboard, &tp_locs_crc, &tp_str, &area_str);
    trash_tstring(&tp_str);
    trash_tstring(&area_str);
    return 1;
}

/* --------------------------------------------------------------------------
 * Activate CTeleporterUsableComponent entities in the current scene.
 * Mirrors the entity-walk loop from ScanVisitDiscoverEverything.
 * Only affects teleporters whose area is currently loaded.
 * -------------------------------------------------------------------------- */
void activate_teleporter_entities(void) {
    GameManager* gm    = *GAMEMANAGER_PPTR;
    void**  entities   = gm_entity_list(gm);
    uint32_t count     = gm_entity_count(gm);
    if (!count) return;

    void* class_ref = ref_to_teleporter_class();
    for (uint32_t i = 0; i < count; i++) {
        void* comp = get_component_for_entity(entities[i], USABLE_STR_PTR);
        if (comp && compare_object_to_class(comp, class_ref)) {
            ((CTeleporterUsableComponent*)comp)->Discovered = 1;
        }
    }
}

/* --------------------------------------------------------------------------
 * Set the "{primaryArea}_discovered" blackboard flag for the given sub-area.
 * Mirrors the tail of FUN_004e6de4.
 * Equivalent Lua: Blackboard.SetProp(sectionStr, primaryArea.."_discovered", "b", true)
 * -------------------------------------------------------------------------- */
void mark_area_discovered(const char* sub_area_name) {
    const AreaSectionEntry* entry = NULL;
    for (int i = 0; i < AREA_SECTION_COUNT; i++) {
        if (strcmp(AREA_SECTION_TABLE[i].subArea, sub_area_name) == 0) {
            entry = &AREA_SECTION_TABLE[i];
            break;
        }
    }
    if (!entry) return;

    char flag_name[64];
    strcpy(flag_name, entry->primaryArea);
    strcat(flag_name, "_discovered");

    void*    flag_str    = NULL;
    uint32_t section_crc = crc32(entry->sectionStr, (int)strlen(entry->sectionStr), 0xFFFFFFFF);
    hash_string(&flag_str, flag_name);

    char value = '\x01';
    set_blackboard_flag(gm_blackboard(*GAMEMANAGER_PPTR), &section_crc, &flag_str, &value);
    trash_tstring(&flag_str);
}

/* --------------------------------------------------------------------------
 * Mark the specific teleporter's minimap cell
 *
 * Walks the loaded MinimapScenarioWrapper chain, matches the scenario by CRC,
 * then calls mark_cell + render_cell_bg + serialize_cells for cell (cellX, cellY).
 * cellX/cellY must be filled in TELEPORTER_TABLE; returns early if -1.
 * -------------------------------------------------------------------------- */
void mark_teleporter_cell(const char* teleporter_name) {
    const TeleporterEntry* tp = NULL;
    for (int i = 0; i < TELEPORTER_COUNT; i++) {
        if (strcmp(TELEPORTER_TABLE[i].teleporter, teleporter_name) == 0) {
            tp = &TELEPORTER_TABLE[i];
            break;
        }
    }
    if (!tp || tp->cellX < 0 || tp->cellY < 0) return;

    const AreaSectionEntry* sec = NULL;
    for (int i = 0; i < AREA_SECTION_COUNT; i++) {
        if (strcmp(AREA_SECTION_TABLE[i].subArea, tp->area) == 0) {
            sec = &AREA_SECTION_TABLE[i];
            break;
        }
    }
    if (!sec) return;

    uint32_t     target_crc = crc32(sec->sectionStr, (int)strlen(sec->sectionStr), 0xFFFFFFFF);
    GameManager* gm         = *GAMEMANAGER_PPTR;
    CMinimap*    minimap    = get_minimap(gm);
    if (!minimap) return;

    for (MinimapScenarioWrapper* w = minimap->FirstWrapper; w; w = w->Next) {
        CMinimapScenario* scenario = w->Scenario;
        if (!scenario) continue;

        MapInfo* info = scenario->Info;
        if (info->Current->CRC != target_crc) continue;

        void*    blackboard = gm_blackboard(gm);
        uint32_t map_width  = info->Width;
        for (uint32_t x = 0; x < 30; x++) {
            for (uint32_t y = 0; y < 30; y++) {
                MapCellStruct* cell = map_coordinates(&scenario->Coords, (int)y * (int)map_width + (int)x);
                if (!cell) break;
                mark_cell(cell, 1, scenario);
                render_cell_bg(cell, scenario, 1);
                serialize_cells(scenario, blackboard);
            }
        }
        return;
    }
}

/* --------------------------------------------------------------------------
 * Full activation — blackboard, entity, global map flag, and minimap cell.
 * Usage: activate_teleporter("LE_Teleporter_03A_001");
 * -------------------------------------------------------------------------- */
int activate_teleporter(const char* teleporter_name) {
    int ok = activate_teleporter_blackboard(teleporter_name);
    if (ok) {
        activate_teleporter_entities();
        for (int i = 0; i < TELEPORTER_COUNT; i++) {
            if (strcmp(TELEPORTER_TABLE[i].teleporter, teleporter_name) == 0) {
                mark_area_discovered(TELEPORTER_TABLE[i].area);
                break;
            }
        }
        mark_teleporter_cell(teleporter_name);
    }
    return ok;
}
