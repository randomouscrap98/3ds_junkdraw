#include "layer_cache.h"
#include "datacontainer.h"
#include "layer.h"
#include "utils.h"

#include <stdlib.h>

int layerwindow_init(LayerWindow * lw, DataContainer * dc, u8 layer_type) {
  lw->master_layers = NULL;
  lw->units = NULL;
  lw->unit_count = 0;
  lw->master_layers_length = 0;
  lw->dc = dc;
  lw->layer_type = layer_type;
  lw->pending_layer = NULL;
  lw->pending_next = 0;
  return linecontainer_init_stroke(&lw->pending);
}

static void layerwindow_free_partial(LayerWindow * lw) {
  if(lw->master_layers) {
    for(size_t i = 0; i < lw->master_layers_length; i++) {
      layer_free(lw->master_layers + i);
    }
    free(lw->master_layers);
    lw->master_layers = NULL;
  }
  if(lw->units) {
    free(lw->units);
    lw->units = NULL;
  }
  lw->pending_next = 0;
  lw->pending.length = 0;
  lw->pending_layer = NULL;
}

void layerwindow_free(LayerWindow * lw) {
  layerwindow_free_partial(lw);
  linecontainer_free(&lw->pending);
}

int layerwindow_reset(LayerWindow * lw, layerdim_t width, layerdim_t height, 
    layer_t layer_count, size_t max_units) {
  // Start by resetting... kinda scary
  layerwindow_free_partial(lw);
  // Calculate total pixels of apparent layers and estimate maximum layer count.
  size_t layerpixels = layer_estimate_pixelcount(lw->layer_type, width, height);
  size_t max_layers = JDLC_MAXPIXELS / layerpixels;
  size_t max_units_by_pixels = max_layers / layer_count;
  if(max_units == 0) {
    lw->unit_count = max_units_by_pixels;
  } else {
    lw->unit_count = JD_MIN(max_units, max_units_by_pixels);
  }
  if(lw->unit_count == 0) {
    LOGERR("NOT ENOUGH SPACE FOR LAYER UNITS!");
    return 1;
  }
  // Allocate junk
  lw->master_layers_length = 0; 
  size_t pending_layer_length = max_units * layer_count;
  lw->master_layers = malloc(sizeof(Layer) * lw->master_layers_length);
  if(!lw->master_layers) {
    LOGERR("NO MEMORY TO ALLOCATE MASTER LAYERS");
    layerwindow_free_partial(lw);
    return 1;
  }
  lw->units = malloc(sizeof(LayerWindowUnit) * lw->unit_count);
  if(!lw->master_layers) {
    LOGERR("NO MEMORY TO ALLOCATE LAYER UNITS");
    layerwindow_free_partial(lw);
    return 1;
  }
  // We increment the master layer count to aid with error'd out free_partial, as it uses
  // that count to free individual layers
  for(lw->master_layers_length = 0; lw->master_layers_length < pending_layer_length; 
      lw->master_layers_length++) {
    int err = layer_init(lw->master_layers + lw->master_layers_length, width, height, lw->layer_type);
    if(err) {
      LOGERR("FAILED TO ALLOCATE LAYER %d", lw->master_layers_length);
      layerwindow_free_partial(lw);
      return err;
    }
  }
  // And finally, setup each unit
  for(size_t i = 0; i < lw->unit_count; i++) {
    lw->units[i].scanner = datacontainer_get_scanner(lw->dc);
    lw->units[i].layer_count = layer_count;
    lw->units[i].layers = lw->master_layers + (i * layer_count);
  }
  return 0;
}

