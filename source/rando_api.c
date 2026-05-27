#include <string.h>
#include <stdint.h>
#include "rando_api.h"
#include "activate_teleporter.h"
#include "game_manager.h"
#include "beam_visuals.h"

#define HEAT 0x40000
#define LAVA 0x200000
#define FLEECH_SWARM 0x2000000

// defaults don't matter because we immediately call the functions which sets these values after a scenario load
int has_spazer = 0x1;
int has_wave = 0x1;
int has_plasma = 0x1;

int gravity_suit_damage_flags = FLEECH_SWARM | LAVA | HEAT;

uint8_t skip_opening = 0;
uint8_t use_fusion_models = 0;


/*
 * Configures the suit damage flags for the split-suit feature.
 */
int change_suit_values(void* lua_state) {
    int has_varia = lua_toboolean(lua_state, 1);
    int has_gravity = lua_toboolean(lua_state, 2);
    float* tun_dmg_reduction_gravity = (float*) 0x0079b1b0;

    if (has_varia && has_gravity) {
      gravity_suit_damage_flags = FLEECH_SWARM | LAVA | HEAT;
      *tun_dmg_reduction_gravity = 0.5;
    } else if (!has_varia && has_gravity) {
      gravity_suit_damage_flags = FLEECH_SWARM;
      *tun_dmg_reduction_gravity = 0.75;
    }
    // only varia doesn't matter because we only change gravity suit values

    return 0;
}

/*
 * Changes per-beam damage tunables, the shoot-through-walls flags and (when
 * applicable) the Spazer color visuals + plasma texture swap.
 *
 * Lua-bound. Called by the Lua side on every relevant inventory event.
 */
int change_beams(void* lua_state) {
    // game creates only the highest projectiles. no need to change wave.

    // addresses to the fDamageAmount tunable of each beam
    // float* waveDmg = (float*) 0x00790724;
    float* spazerDmg = (float*) 0x00790474;
    float* plasmaDmg = (float*) 0x0078ff28;

    // parse the 7 parameters to the lua function
    has_wave = lua_toboolean(lua_state, 1);
    has_spazer = lua_toboolean(lua_state, 2);
    has_plasma = lua_toboolean(lua_state, 3);
    float dmgSpazer = lua_tonumber(lua_state, 4);
    float dmgPlasma = lua_tonumber(lua_state, 5);
    float dmgPlasmaWave = lua_tonumber(lua_state, 6);
    float dmgPlasmaSpazer = lua_tonumber(lua_state, 7);

    // damage modifications
    if (has_plasma) {
        if (has_wave && has_spazer) *plasmaDmg = 100.0;
        else if (has_spazer) *plasmaDmg = dmgPlasmaSpazer;
        else if (has_wave) *plasmaDmg = dmgPlasmaWave;
        else *plasmaDmg = dmgPlasma;
    } else if (has_spazer) {
        if (has_wave) *spazerDmg = 70.0;
        else *spazerDmg = dmgSpazer;
    } 
    // else *waveDmg = 50.0;


    // shoot through walls modifications
    // game uses address 0x00758518 to store the info if a beam goes through walls
    // if 0x00758518[BeamProjectileType * 2] & 1 is true the beam does not shoot through walls
    // Power = 0x0, Ice = 0x1, Wave = 0x2, Plasma = 0x3, Spazer = 0x4
    uint8_t* power_beam_walls = (uint8_t*) 0x00758518;
    // plasma
    *(power_beam_walls + 3 * 2) = has_wave ? 0x00 : 0x01; 
    // spazer
    *(power_beam_walls + 4 * 2) = has_wave ? 0x00 : 0x01;

    // visual swaps
    if (has_plasma) {
        const char* plasma = "actors/weapons/plasmabeam/models/textures/plasmacust_rgba.bctex";
        plasma_swap_init(plasma);
        if (has_wave) plasma_show_original(); 
        else plasma_show_custom();
    }

    return 0;
}

/*
 * Activates a specific teleporter by name.
 */
int activate_teleporter_lua_wrapper(void* lua_state) {
    size_t lua_string_size = 0;    // length of the lua string response (without \0)
    const char* teleportername = lua_tolstring(lua_state, 1, &lua_string_size);
    activate_teleporter(teleportername);

    return 0;
}

/*
 * Function used as bridge to make the patcher options available in the c and asm code
 */
int save_patcher_options(void* lua_state) {
    skip_opening = lua_toboolean(lua_state, 1);
    use_fusion_models = lua_toboolean(lua_state, 2);
    return 0;
}

/*
 * Updates the spazer beam gun but it has to be called separetly becasue
 * the gun component needs to be already created by the game
 */
int update_spazer_gun(void* lua_state) {
    if (has_spazer && !has_plasma) change_spazer_colors();
    return 0;
}

/* Lua functions -> C functions */
const luaL_Reg rando_api_lib[] = {
    // ChangeSuitValues(has_varia, has_gravity) e.g. RandoApi.ChangeSuitValues(false, true)
    {"ChangeSuitValues", change_suit_values},
    // ChangeBeams(hasWave, hasSpazer, hasPlasma, dmgSpazer, dmgPlasma, dmgPlasmaWave, dmgPlasmaSpazer)
    {"ChangeBeams", change_beams},
    {"UpdateSpazerGun", update_spazer_gun},
    {"SavePatcherOptions", save_patcher_options},
    {"ActivateTeleporter", activate_teleporter_lua_wrapper},
    {NULL, NULL}
};
