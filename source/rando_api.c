#include <string.h>
#include "rando_api.h"

#define HEAT 0x40000
#define LAVA 0x200000
#define FLEECH_SWARM 0x2000000

int has_spazer = 0x1;
int gravity_suit_damage_flags = FLEECH_SWARM | LAVA | HEAT;

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

int change_beams(void* lua_state) {
    // game creates only the highest projectiles. no need to change wave.
    
    // addresses to the fDamageAmount tunable of each beam
    // float* waveDmg = (float*) 0x00790724;
    float* spazerDmg = (float*) 0x00790474;
    float* plasmaDmg = (float*) 0x0078ff28;

    // parse the 7 parameters to the lua function
    int has_wave = lua_toboolean(lua_state, 1);
    has_spazer = lua_toboolean(lua_state, 2);
    int has_plasma = lua_toboolean(lua_state, 3);
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

    return 0;
}


/* Lua functions -> C functions */
const luaL_Reg rando_api_lib[] = {
    // ChangeSuitValues(has_varia, has_gravity) e.g. RandoApi.ChangeSuitValues(false, true)
    {"ChangeSuitValues", change_suit_values}, 
    // ChangeBeams(hasWave, hasSpazer, hasPlasma, dmgSpazer, dmgPlasma, dmgPlasmaWave, dmgPlasmaSpazer)
    {"ChangeBeams", change_beams}, 
    {NULL, NULL}  
};
