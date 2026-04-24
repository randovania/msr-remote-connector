#include <string.h>
#include "minimap.h"

int serialize() {
    GameManager* gm = *GAMEMANAGER_PPTR;
    CMinimap* minimap = get_minimap(gm);
    void* blackboard = gm_blackboard(gm);

    MinimapScenarioWrapper* wrapper = minimap->FirstWrapper;
    while (wrapper != NULL) {
        // setting the "dirty" flags seems to be sufficient, and "“"serialize_cells" isn't necessary, 
        // but let's leave it that way for now
        wrapper->Scenario->Dirty = 1;
        serialize_cells(wrapper->Scenario, blackboard);
        wrapper = wrapper->Next;
    }

    return 0;
}