#ifndef __HEADER_3DSJUNKDRAW_LAYERCACHE__
#define __HEADER_3DSJUNKDRAW_LAYERCACHE__

#include "layer.h"
#include "datacontainer.h"

// Kind of arbitrary but whatever
#define JDLC_MAXPIXELS (1024 * 1024 * 2)
// #define JDLC_MAXUNITS 256

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
  Layer * pending_layer;    // Which layer holds onto the pending line (must finish!)
  LayerWindowUnit * units;  // Each unit is a page
  DataContainer * dc;       // The container to pull decoded draw commands from
  size_t unit_count;        // Number of units allocated
  size_t master_layers_length;  // Length of master layer list (total)
  u8 layer_type;            // When resetting, which type of layer to use
  LineContainer pending;    // Save last scanned stroke
  size_t pending_next;      // Position within pending
} LayerWindow;

int layerwindow_init(LayerWindow * lw, DataContainer * dc, u8 layer_type);
void layerwindow_free(LayerWindow * lw);

// pass 0 for max_units if you don't want to limit the number of units except by max pixel
// (system requirement). Amount of actual units may be smaller than max_units
int layerwindow_reset(LayerWindow * lw, layerdim_t width, layerdim_t height, 
    layer_t layer_count, size_t max_units);

#endif
