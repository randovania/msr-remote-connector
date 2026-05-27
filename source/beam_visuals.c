#include <stdint.h>
#include <string.h>
#include "beam_visuals.h"
#include "game_manager.h"

extern int has_wave;

// TODO: create a new texture for the custom plasma beam
// TODO: set_spazer_charge_glow_color does not work on the 3DS because we
//       are not allowed to patch code.bin

static inline uint32_t rd32(uint32_t va)             { return *(volatile uint32_t *)va; }
static inline void     wr32(uint32_t va, uint32_t v) { *(volatile uint32_t *)va = v; }

/*
 *  Section A — Spazer visuals with and without wave
 */

// Engine addresses
#define GUN_STR_PTR              ((void**) 0x00708490)
#define SPAZER_VTABLE            0x006CDDE4u
#define NAMED_POOL_DESC_ADDR     0x007705B0u

// Named-pool FX entry CRCs
#define CRC_WAVE_BURST0          0xA621923Eu
#define CRC_WAVE_BURST1          0xD126A2A8u
#define CRC_WAVE_CHARGE_BURST    0xF21BD47Cu
#define CRC_POWER_BURST0         0x5EF6C1EDu
#define CRC_POWER_BURST1         0x29F1F17Bu
#define CRC_POWER_CHARGE_BURST   0xB15539B9u

#define ENTRY_STRIDE             0x20u

// Beam-gun struct offsets
#define GUN_OFF_BURST0_FX        0x2C
#define GUN_OFF_BURST1_FX        0x30
#define GUN_OFF_CHARGE_BURST_FX  0x34
#define GUN_ENTITY_OFF           0x0C    /* CEntity* — UpdateGunModel dereferences this unchecked */
#define GUN_NEEDS_UPDATE_OFF     0x1589  /* u8 GunNeedsUpdate — the gate */
#define GUN_COLOR_OFF            0x158C  /* CVector4D GunColor */

// Game functions
typedef void (*update_gun_model_fn)(uint32_t gun);
#define update_gun_model ((update_gun_model_fn) 0x0051b248)

// Internal types
typedef struct {
    uint32_t vtable;        /* +0x00 — pool descriptor's own vtable */
    uint32_t pool_start;    /* +0x04 — first usable byte in pool    */
    uint32_t pool_size;     /* +0x08 — byte count                   */
    uint32_t pool_base;     /* +0x0C — alloc base                   */
    uint32_t pool_end;      /* +0x10 — one-past-end                 */
    uint32_t unk;           /* +0x14 — 0xFFFFFFFF                   */
    uint32_t initialised;   /* +0x18 — 1 once the pool is live      */
} pool_desc_t;

typedef struct {
    uint32_t wave_b0, wave_b1, wave_cb;
    uint32_t power_b0, power_b1, power_cb;
} fx_ptrs_t;


/*
 * Walks the entity list, finds Samus' CSamusGunComponent, and returns the
 * beam-gun instance whose first u32 (vtable) matches `gun_vtable_address`.
 * Checks the currently-equipped slot first, then scans the beam-gun array.
 */
static uint32_t get_gun(uint32_t gun_vtable_address) {
    GameManager* gm = *GAMEMANAGER_PPTR;
    if (!gm) return 0;
    void** entities = gm_entity_list(gm);
    uint32_t count  = gm_entity_count(gm);
    if (!entities || !count) return 0;

    for (uint32_t i = 0; i < count; i++) {
        uintptr_t* gun_comp = (uintptr_t*) get_component_for_entity(entities[i], GUN_STR_PTR);
        if (!gun_comp) continue;

        uintptr_t cb = (uintptr_t)gun_comp;

        // fast check for the currently equipped beam
        uint32_t cur = rd32(cb + 0x9C);
        if (cur && rd32(cur) == gun_vtable_address) return cur;

        // fall back to scanning the beam-gun array
        uint32_t arr_owner = rd32(cb + 0x6C);
        if (!arr_owner) continue;
        uint32_t arr_head = rd32(arr_owner);
        if (!arr_head) continue;
        for (uint32_t k = 0; k < 8; k++) {
            uint32_t cand = rd32(arr_head + k * 4);
            if (cand && rd32(cand) == gun_vtable_address) return cand;
        }
    }
    return 0;
}

/*
 * Walks the named-entry pool, resolves the 6 FX-entry pointers we need, and
 * binds either the wave or power set onto the Spazer gun's burst-fx slots.
 */
static void apply_spazer_muzzle_fx() {
    uint32_t gun = get_gun(SPAZER_VTABLE);
    if (!gun) return;

    const pool_desc_t *desc = (const pool_desc_t *)NAMED_POOL_DESC_ADDR;
    if (desc->initialised != 1) return;
    uint32_t start = desc->pool_start, end = desc->pool_end;
    if (start == 0 || end <= start) return;

    fx_ptrs_t fx = {0};
    for (uint32_t e = start; e + ENTRY_STRIDE <= end; e += ENTRY_STRIDE) {
        if (rd32(e + 0x0C) != NAMED_POOL_DESC_ADDR) continue;
        uint32_t p = e + 4;   /* gun fields expect entry+4 */
        switch (rd32(e)) {
            case CRC_WAVE_BURST0:        fx.wave_b0  = p; break;
            case CRC_WAVE_BURST1:        fx.wave_b1  = p; break;
            case CRC_WAVE_CHARGE_BURST:  fx.wave_cb  = p; break;
            case CRC_POWER_BURST0:       fx.power_b0 = p; break;
            case CRC_POWER_BURST1:       fx.power_b1 = p; break;
            case CRC_POWER_CHARGE_BURST: fx.power_cb = p; break;
            default: break;
        }
    }
    if (!fx.wave_b0 || !fx.wave_b1 || !fx.wave_cb ||
        !fx.power_b0 || !fx.power_b1 || !fx.power_cb) return;

    if (has_wave) {
        wr32(gun + GUN_OFF_BURST0_FX,       fx.wave_b0);
        wr32(gun + GUN_OFF_BURST1_FX,       fx.wave_b1);
        wr32(gun + GUN_OFF_CHARGE_BURST_FX, fx.wave_cb);
    } else {
        wr32(gun + GUN_OFF_BURST0_FX,       fx.power_b0);
        wr32(gun + GUN_OFF_BURST1_FX,       fx.power_b1);
        wr32(gun + GUN_OFF_CHARGE_BURST_FX, fx.power_cb);
    }
}

/*
 * The gun body color is stored twice we only write the first occurences
 * which reloads to the "active" one once we call "update_gun_model"
 */
static void set_spazer_gun_color() {
    uint32_t gun = get_gun(SPAZER_VTABLE);
    if (!gun) return;

    float r, g, b, a;
    if (!has_wave) { r = 1.0f; g = 0.7289999723434448f; b = 0.0f; a = 1.0f; } /* power yellow */
    else           { r = 1.0f; g = 0.20000000298023224f; b = 1.0f; a = 1.0f; } /* wave magenta */

    float *gun_color = (float *)(gun + GUN_COLOR_OFF);
    gun_color[0] = r; gun_color[1] = g; gun_color[2] = b; gun_color[3] = a;

    // Arm the gate, then run the game's own robust apply function
    if (rd32(gun + GUN_ENTITY_OFF)) {
        *(uint8_t *)(gun + GUN_NEEDS_UPDATE_OFF) = 1;
        update_gun_model(gun);
    }
}


/*
 * Changes the charge glow effect, the gun color and the muzzle effect at the
 * gun when shooting projectiles.
 */
void change_spazer_colors() {
    set_spazer_gun_color();
    apply_spazer_muzzle_fx();
}


/*
 *  Section B — Plasma texture swap
 */
// Engine addresses (ARM mode, no Thumb bit)
#define G_PRECUTSCENE_MGR_PTR    0x0076f31cu   /* -> manager singleton           */
#define G_TEXTURE_CACHE_PTR      0x0076f2d8u   /* -> _xc0Struct resource cache    */

// Manager → texture descriptor offsets
#define PLASMA_BCTEX_OFF         0xCCu         /* Manager     -> Plasma BctexStruct* */
#define BCTEX_SUBSUB_OFF         0xE8u         /* BctexStruct -> BctexSubSub*        */
#define SUBSUB_VIEW_OFF          0x264u        /* BctexSubSub -> descriptor          */
#define DESC_DATA_OFF            0x20u         /* descriptor  -> pixel buffer ptr    */
#define DESC_SIZE_OFF            0x04u         /* descriptor  -> pixel buffer size   */

typedef int      (*resolve_fn)(void* cache, void** name, int p3,
                               uint32_t flags, void* p5, uint32_t p6);
typedef uint32_t (*texview_fn)(int handle);

#define ResolveTex    ((resolve_fn) 0x001b17b8)
#define GetTexView    ((texview_fn) 0x0017207c)


// Helpers
static uint32_t plasma_view() {
    uint32_t mgr = rd32(G_PRECUTSCENE_MGR_PTR); if (!mgr) return 0;
    uint32_t bx  = rd32(mgr + PLASMA_BCTEX_OFF); if (!bx)  return 0;
    uint32_t ss  = rd32(bx + BCTEX_SUBSUB_OFF);  if (!ss)  return 0;
    return rd32(ss + SUBSUB_VIEW_OFF);
}

static uint32_t plasma_lookup_flags() {
    uint32_t mgr = rd32(G_PRECUTSCENE_MGR_PTR); if (!mgr) return 0;
    uint32_t bx  = rd32(mgr + PLASMA_BCTEX_OFF); if (!bx)  return 0;
    return (rd32(bx + 0x4) & 0xffffff) >> 16;
}

static uint32_t resolve_texture(const char* path, uint32_t flags) {
    void* cache = *(void**)G_TEXTURE_CACHE_PTR; if (!cache) return 0;
    void* name = NULL;
    hash_string(&name, path);
    uint32_t handle = ResolveTex(cache, &name, 1, flags, 0, 0);
    trash_tstring(&name);
    return handle;
}

// Buffer holding whichever texture is currently NOT live
enum { BUF_CUSTOM, BUF_ORIGINAL };
static uint8_t  g_buf[0x5540];
static uint32_t g_buf_n     = 0;
static int      g_buf_holds = BUF_CUSTOM;

/* 
 * Call once in InitScenario while the custom texture is still resident.
 * Returns 0 on success, <0 on error. 
 */
int plasma_swap_init(const char* custom_path) {
    if (g_buf_n != 0) return 0;
    uint32_t handle = resolve_texture(custom_path, plasma_lookup_flags());
    if (!handle) return -1;
    uint32_t cv = GetTexView(handle);
    if (!cv) return -2;                          // not resident — wait one frame and retry
    uint32_t cs = rd32(cv + DESC_DATA_OFF);
    uint32_t cn = rd32(cv + DESC_SIZE_OFF);
    if (!cs || cn > sizeof g_buf) return -3;
    memcpy(g_buf, (const void*)cs, cn);
    g_buf_n     = cn;
    g_buf_holds = BUF_CUSTOM;                    // buffer = custom, live = original
    return 0;
}

/*
 * Swap g_buf <-> live plasma buffer, flip the flag. 
 */
static void plasma_exchange() {
    uint32_t v = plasma_view();           if (!v) return;
    uint32_t d = rd32(v + DESC_DATA_OFF); if (!d) return;
    uint32_t cap = rd32(v + DESC_SIZE_OFF);
    uint32_t n = g_buf_n < cap ? g_buf_n : cap;

    uint8_t tmp[512];                                // scratch for in-place swap
    for (uint32_t off = 0; off < n; off += sizeof tmp) {
        uint32_t k = (n - off) < sizeof tmp ? (n - off) : sizeof tmp;
        memcpy(tmp, (void*)(d + off), k);            // live -> tmp
        memcpy((void*)(d + off), g_buf + off, k);    // buf  -> live
        memcpy(g_buf + off, tmp, k);                 // tmp  -> buf
    }
    g_buf_holds = (g_buf_holds == BUF_CUSTOM) ? BUF_ORIGINAL : BUF_CUSTOM;
}

/* 
 *Idempotent, may be called any number of times mid-game. 
 */
void plasma_show_custom() {
     if (g_buf_holds == BUF_CUSTOM)   plasma_exchange(); 
}

/* 
 *Idempotent, may be called any number of times mid-game. 
 */
void plasma_show_original() {
    if (g_buf_holds == BUF_ORIGINAL) plasma_exchange();
}
