#ifndef __HEADER_3DSJUNKDRAW_LAYERCACHE__
#define __HEADER_3DSJUNKDRAW_LAYERCACHE__

#include "layer.h"
#include "datacontainer.h"
#include "vector.h"

// Kind of arbitrary but whatever
#define JDLC_MAXPIXELS (1024 * 1024 * 2)
#define JDLC_CLEAR 0
// #define JDLC_MAXUNITS 256

VECTOR_DECLARE(RenderLine);
VECTOR_DECLARE(vector_RenderLine);

typedef struct {
  Layer * layers;       // No storage, just points to master layer list in parent
  layer_t layer_count;  // Perfunctory: all units should all have the same count
  DataScanner scanner;  // Each unit is tightly coupled with a scanner (texture + scan pos coupled)
} LayerWindowUnit;

// A system for rendering pages and caching them. The system can allow throttled, ordered
// drawing into pages from N to offset, and allows exports. Setup to automatically pull from
// a DataContainer source. Layers can be software or hardware rendered.
typedef struct {
  Layer * master_layers;    // To cut down on spaced out mallocs, all unit layers here
  LayerWindowUnit * units;  // Each unit is a page
  LayerWindowUnit * pending_unit;    // Which unit holds onto the pending linecontainer (must finish!)
  DataContainer * dc;       // The container to pull decoded draw commands from
  vector_vector_RenderLine drawlines; // Pending draw lines for layerwindow_pull. One vector per layer
  size_t unit_count;        // Number of units allocated
  size_t master_layers_length;  // Length of master layer list (total)
  u8 layer_type;            // When resetting, which type of layer to use
  LineContainer pending;    // Save last scanned stroke
  size_t pending_next;      // Position within pending
} LayerWindow;

int layerwindow_init(LayerWindow * lw, DataContainer * dc, u8 layer_type);
void layerwindow_free(LayerWindow * lw);
int layerwindow_pull(LayerWindow * lw, size_t max_scan, size_t max_draw, page_t page, page_t offset);

// pass 0 for max_units if you don't want to limit the number of units except by max pixel
// (system requirement). Amount of actual units may be smaller than max_units
int layerwindow_reset(LayerWindow * lw, layerdim_t width, layerdim_t height, 
    layer_t layer_count, size_t max_units);

//void linecontainer_to_renderline(LineContainer * lc, size_t line, RenderLine * rl);

// Special function you could use
// void linecontainer_to_renderlines(LineContainer * lc, size_t start, size_t count, RenderLine * rl);
#endif
