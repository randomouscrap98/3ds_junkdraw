#include "layer_cache.h"
#include "datacontainer.h"
#include "layer.h"
#include "utils.h"

#include <stdlib.h>

// Which unit to store page into. Modulo, thus cache is always uniform no matter 
// which page you start at
#define JDLC_UNIT(lw, page) ((page) % (lw)->unit_count)

VECTOR_DEFINE(RenderLine);

int layerwindow_init(LayerWindow * lw, DataContainer * dc, u8 layer_type) {
  lw->master_layers = NULL;
  lw->units = NULL;
  lw->unit_count = 0;
  lw->master_layers_length = 0;
  lw->dc = dc;
  lw->layer_type = layer_type;
  lw->pending_unit = NULL;
  lw->pending_next = 0;

  int err = vector_vector_RenderLine_init(&lw->drawlines);
  if(err) return err;
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
  // Free the inner vectors
  for(size_t i = 0; i < lw->drawlines.length; i++) {
    vector_RenderLine_free(lw->drawlines.array + i);
  }
  // Not necessary but... makes it a little nicer (the inner vectors 
  // are invalid)
  vector_vector_RenderLine_clear(&lw->drawlines);
}

static inline void layerwindow_reset_pending(LayerWindow * lw) {
  // Clear out pending...
  lw->pending_next = 0;
  lw->pending.length = 0;
  lw->pending_unit = NULL;
}

void layerwindow_free(LayerWindow * lw) {
  layerwindow_free_partial(lw);
  linecontainer_free(&lw->pending);
  // Free the vector container (free_partial frees inner)
  vector_vector_RenderLine_free(&lw->drawlines);
}

int layerwindow_reset(LayerWindow * lw, layerdim_t width, layerdim_t height, 
    layer_t layer_count, size_t max_units) {
  // Start by resetting... kinda scary
  layerwindow_free_partial(lw);
  layerwindow_reset_pending(lw);
  // The inner vectors are freed, so just rebuild from scratch
  vector_vector_RenderLine_reserve(&lw->drawlines, layer_count);
  for(layer_t i = 0; i < layer_count; i++) {
    size_t index;
    vector_vector_RenderLine_increment(&lw->drawlines, &index);
    vector_RenderLine_init(lw->drawlines.array + index);
  }
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
  // Allocate contiguous arrays (master_layers and units)
  lw->master_layers_length = 0; 
  size_t total_layer_count = max_units * layer_count;
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
  for(lw->master_layers_length = 0; lw->master_layers_length < total_layer_count; 
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

static void layerwindow_reset_unit(LayerWindow * lw, size_t unit_id) {
  LayerWindowUnit * unit = lw->units + unit_id;
  datascanner_reset(&unit->scanner);
  for(int i = 0; i < unit->layer_count; i++) {
    layer_clear(&unit->layers[i], JDLC_CLEAR);
  }
  if(lw->pending_unit == unit) {
    // Clear out the pending line in the master window, it was for this unit
    // and is no longer needed
    lw->pending_unit = NULL;
    lw->pending_next = 0;
    lw->pending.length = 0;
  }
}

static inline void layerwindow_clear_drawlines(LayerWindow * lw, size_t reserve) {
  for(size_t i = 0; i < lw->drawlines.length; i++) {
    vector_RenderLine_clear(lw->drawlines.array + i);
    vector_RenderLine_reserve(lw->drawlines.array + i, reserve);
  }
}

static void layerwindow_render_unit(LayerWindow * lw, size_t unit_id, vector_vector_RenderLine * lines) {
  // Only render the minimum of whichever layers are thing
  LayerWindowUnit * unit = lw->units + unit_id;
  size_t layer_count = JD_MIN(lines->length, unit->layer_count);
  for(size_t i = 0; i < layer_count; i++) {
    layer_drawlines(unit->layers + i, lines->array[i].array, lines->array[i].length);
  }
}

// Convert a region of lines from the line container into a renderline vector.
static inline void linecontainer_to_renderlines(
    LineContainer * lc, size_t start, size_t count, vector_RenderLine * rl) {
  // TODO: is this the correct conversion? The output is correct but is 5551 correct?
  u32 color = rgba5551_to_abgr8(lc->color);
  for(size_t i = 0; i < count; i++) {
    size_t rlidx;
    vector_RenderLine_increment(rl, &rlidx);
    rl->array[rlidx].width = lc->width;
    rl->array[rlidx].color = color;
    rl->array[rlidx].x1 = lc->lines[i + start].x1;
    rl->array[rlidx].x2 = lc->lines[i + start].x2;
    rl->array[rlidx].y1 = lc->lines[i + start].y1;
    rl->array[rlidx].y2 = lc->lines[i + start].y2;
  }
}

// Pull up to max_draw out of the current pending line into the drawlines vector. Returns
// the amount pulled into the array. Sorts the lines into the proper layer vector
static inline size_t layerwindow_dump_pending(LayerWindow * lw, size_t max_draw) {
  size_t pull_count = JD_MIN(max_draw, lw->pending.length - lw->pending_next);
  linecontainer_to_renderlines(&lw->pending, lw->pending_next, pull_count, 
                               lw->drawlines.array + lw->pending.layer);
  lw->pending_next += pull_count;
  max_draw -= pull_count;
  if(lw->pending_next >= lw->pending.length) {
    layerwindow_reset_pending(lw);
  }
  return pull_count;
}

int layerwindow_pull(LayerWindow * lw, size_t max_scan, size_t max_draw, page_t page, page_t offset) {
  page_t increment = offset < 0 ? -1 : 1;
  // Can't wrap around multiple times, that's bad. Offset is inclusive! 0 = pull one page!
  if(abs(offset) >= lw->unit_count) offset = increment * (lw->unit_count - 1);
  page_t end = page + offset + increment; // EXCLUSIVE (to make loops simpler)
  // First, need to go through and clear out any invalid pages
  for(page_t pg = page; pg != end; pg += increment) {
    size_t unit = JDLC_UNIT(lw, pg);
    // Fill all the scanners with max_draw. Maybe kinda bad?
    lw->units[unit].scanner.max_scan = max_scan;
    if(lw->units[unit].scanner.page != pg) {
      LOGTRC("Resetting page unit %d (pg %d)", unit, pg);
      layerwindow_reset_unit(lw, unit);
    }
  }
  // Clear all the layer drawlines
  layerwindow_clear_drawlines(lw, max_draw);
  // For simplicity: if we still have a pending unit, clear that out first
  if(lw->pending_unit) {
    max_draw -= layerwindow_dump_pending(lw, max_draw);
  }
  // Now draw as much as possible from pages in order from user's current page then going backwards.
  while(max_draw > 0) {
    if(!lw->pending_unit) {
    }
  }
  // Now that we know the units are correct, if there happens to still be a pending
  // unit, that gets first dibs on drawing
    // while(lw->pending_next < lw->pending.length) {
    // }
  return 0;
}

