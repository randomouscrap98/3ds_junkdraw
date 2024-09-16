#include "my3ds.h"

bool myAptMainLoop() {
    static bool running = true;
    // This shortcuts calling aptMainLoop
    if(running && !aptMainLoop())
        running = false;
    return running;
}