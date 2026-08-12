#include "layer_cache.h"

#include "datacontainer.h"
#include "layer.h"
#include "lineconversion.h"
#include "utils.h"

#include <stdlib.h>

// Which unit to store page into. Modulo, thus cache is always uniform no matter 
// which page you start at
#define JDLC_UNIT(lw, page) ((page) % (lw)->unit_count)

// #define JDLC_DEBUG


int layerwindow_init(LayerWindow * lw, DataContainer * dc, u8 layer_type) {
  lw->master_layers = NULL;
  lw->units = NULL;
  lw->unit_count = 0;
  lw->master_layers_length = 0;
  lw->dc = dc;
  lw->layer_type = layer_type;
  lw->pending_unit = NULL;
  return lineconverter_init(&lw->pending);
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
}

void layerwindow_free(LayerWindow * lw) {
  layerwindow_free_partial(lw);
  lineconverter_free(&lw->pending);
}

int layerwindow_reset(LayerWindow * lw, layerdim_t width, layerdim_t height, 
    layer_t layer_count, size_t max_units) {
  // Start by resetting... kinda scary
  lw->pending_unit = NULL;
  layerwindow_free_partial(lw);
  lineconverter_reset(&lw->pending);
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
  size_t total_layer_count = lw->unit_count * layer_count;
  lw->master_layers = malloc(sizeof(Layer) * total_layer_count);
  if(!lw->master_layers) {
    LOGERR("NO MEMORY TO ALLOCATE MASTER LAYERS");
    layerwindow_free_partial(lw);
    return 1;
  }
  lw->units = malloc(sizeof(LayerWindowUnit) * lw->unit_count);
  if(!lw->units) {
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
    lineconverter_reset(&lw->pending);
  }
}

static void layerwindowunit_render(LayerWindowUnit * unit, LineConverter * pending,
                                   int clear_lines) {
  size_t layer_count = JD_MIN(unit->layer_count, pending->lines.length);
  for(size_t i = 0; i < layer_count; i++) {
    layer_drawlines(unit->layers + i, pending->lines.array[i].array, pending->lines.array[i].length);
  }
  if(clear_lines) { // So common, easier to do it in render
    lineconverter_reset_converted(pending);
  }
}

int layerwindow_pull(LayerWindow * lw, size_t max_scan, size_t max_draw, PageRange range) {
  page_t increment = range.offset < 0 ? -1 : 1;
  // Can't wrap around multiple times, that's bad. Offset is inclusive! 0 = pull one page!
  if(abs(range.offset) >= lw->unit_count) range.offset = increment * (lw->unit_count - 1);
  page_t end = range.page + range.offset + increment; // EXCLUSIVE (to make loops simpler)
  // Oops, don't go past the start!
  if(-range.offset > range.page) { 
    if(range.loop_point > 0) { // looping
      // very non performant but only if you pass in stupid values (nobody will)
      while(end < 0) { end += range.loop_point; }
    } else {
      end = 0;  // just clamp
    }
  }
  // First, need to go through and clear out any invalid pages
  for(page_t pg = range.page; pg != end; pg += increment) {
    if(pg < 0) pg += range.loop_point;
    size_t unit = JDLC_UNIT(lw, pg);
    // Fill all the scanners with max_draw. Maybe kinda bad?
    lw->units[unit].scanner.max_scan = max_scan;
    if(lw->units[unit].scanner.page != pg) {
      LOGTRC("Resetting page unit %d (pg %d)", unit, pg);
      layerwindow_reset_unit(lw, unit);
      lw->units[unit].scanner.page = pg;
    }
  }
  // Clear all the layer drawlines for safety (shouldn't be anything left but...)
  lineconverter_reset_converted(&lw->pending);
  // For simplicity: if we still have a pending unit, clear that out first
  if(lw->pending_unit) {
#ifdef JDLC_DEBUG
    LOGTRC("Pending lines: %d/%d", lw->pending.pending_next, lw->pending.pending.length);
#endif
    max_draw -= lineconverter_convert(&lw->pending, max_draw);
    // ALWAYS render what we pulled out (also clears converted lines)
    layerwindowunit_render(lw->pending_unit, &lw->pending, 1);
    // No use continuing... we already ran out of draw calls
    if(max_draw == 0) {
      LOGTRC("Overloaded pre-window pending lines");
      // Don't leave garbage behind for next time (not necessarily required?)
      if(lineconverter_done(&lw->pending)) { lw->pending_unit = NULL; } 
      return 0;
    }
  }
// #ifdef JDLC_DEBUG
//   else {
//     LOGTRC("NO PENDING LINES (normal?)");
//   }
// #endif
  // Now iterate over the user's requested pages again!
  DataScannerResult dsr;
  for(page_t pg = range.page; pg != end; pg += increment) {
    if(pg < 0) pg += range.loop_point;
    // Set which unit we're working on (in case we exit early)
    lw->pending_unit = lw->units + JDLC_UNIT(lw, pg);
    // Skip a lot of work if we literally have nothing...
    if(datascanner_at_end(&lw->pending_unit->scanner)) { continue; }
    // Begin pulling out whole strokes on given page
    while(datascanner_next_loop(&lw->pending_unit->scanner, &dsr)) {
      lineconverter_reset_pending(&lw->pending);
      if(datascannerresult_parseline(&dsr, &lw->pending.pending)) {
        LOGWRN("BAD STROKE, exit render early");
        break;
      }
      // This will either convert the entire stroke or not. We exit
      // early if it could not convert the whole thing (otherwise we
      // lose lines!)
      max_draw -= lineconverter_convert(&lw->pending, max_draw);
      if(!lineconverter_done(&lw->pending)) { break; }
    }
    // Render what we got before moving on to the next page
    layerwindowunit_render(lw->pending_unit, &lw->pending, 1);
    // NEED to break out early to preserve pending unit!
    if(max_draw <= 0) { break; }
  }
  return 0;
}

