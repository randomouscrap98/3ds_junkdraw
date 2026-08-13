#include <3ds.h>

u32 __stacksize__ = 512 * 1024;

#include "utils.h"

#include <citro2d.h>
#include <citro3d.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "color.h"
#include "layer.h"
#include "edit.h"
#include "datacontainer.h"
#include "lineconversion.h"
#include "layer_cache.h"

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

// #define LWP_SOFT
// #define LWP_SKIP
// #define LSH_NOCOPY
// #define DC_SKIP
// #define EDIT_SKIP
// #define LC_SKIP
// #define UTILS_SKIP

#define LWP_WIDTH 64 //(64 - JDL_EDGEBUF)
#define LWP_HEIGHT 64 //(64 - JDL_EDGEBUF)


void start_frame() {
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);
  C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE,
                 GPU_ZERO);
}

void end_frame() {
  C2D_Flush();
  C3D_FrameEnd(0);
}

LogBuffer logbuf;

void LOGERR(const char *fmt, ...) {
  JDU_LOGBUFFER_STD(&logbuf, JDU_LOGCOLOR_ERR, 1, fmt);
}
void LOGWRN(const char *fmt, ...) {
  JDU_LOGBUFFER_STD(&logbuf, JDU_LOGCOLOR_WRN, 1, fmt);
}
void LOGINF(const char *fmt, ...) {
  JDU_LOGBUFFER_STD(&logbuf, JDU_LOGCOLOR_INF, 1, fmt);
}
void LOGDBG(const char *fmt, ...) {
  JDU_LOGBUFFER_STD(&logbuf, JDU_LOGCOLOR_DBG, 1, fmt);
}
void LOGTRC(const char *fmt, ...) {
  JDU_LOGBUFFER_STD(&logbuf, JDU_LOGCOLOR_TRC, 1, fmt);
}


int run_datacontainer_test_suite(void) {
  LOGINF("Starting DataContainer Test Suite...");

  // Initialize Containers
  DataContainer dc;
  if (datacontainer_init(&dc, 2048) != 0) {
    LOGERR("Failed to initialize DataContainer");
    return 1;
  }

  // Make sure the fill is just the header
  size_t filled = datacontainer_filled(&dc);
  if(filled != JDDC_FHEADER_LEN) {
    LOGERR("Init fill not header size! Fill: %zu", filled);
    return 1;
  }
  LOGDBG("Header filled test passed");

  LineContainer input_lc;
  if (linecontainer_init_stroke(&input_lc) != 0) {
    LOGERR("Failed to initialize input LineContainer");
    datacontainer_free(&dc);
    return 1;
  }

  LineContainer output_lc;
  if (linecontainer_init_stroke(&output_lc) != 0) {
    LOGERR("Failed to initialize output LineContainer");
    linecontainer_free(&input_lc);
    datacontainer_free(&dc);
    return 1;
  }

  // Test Header Management
  DataHeader write_header = {
    .resolution_id = 1,
    .bgcolor = 0x1234,
    .layer_count = 3,
    .onion_count = 2
  };
  datacontainer_setheader(&dc, &write_header);

  DataHeader read_header;
  datacontainer_getheader(&dc, &read_header);

  if (read_header.resolution_id != write_header.resolution_id ||
    read_header.bgcolor != write_header.bgcolor ||
    read_header.layer_count != write_header.layer_count ||
    read_header.onion_count != write_header.onion_count) {
    LOGERR("Header mismatch");
    goto error;
  }
  LOGDBG("Header test passed");

  // Make sure the fill is just the header still after header set
  filled = datacontainer_filled(&dc);
  if(filled != JDDC_FHEADER_LEN) {
    LOGERR("Post header fill not header size! Fill: %zu", filled);
    return 1;
  }
  LOGDBG("Header filled test (post) passed");

  // Oops, forgot to test "enough"
  // Test datacontainer_enough boundary conditions
  size_t initial_header_offset = dc.end - dc.container; // Header offset
  size_t exact_remaining = dc.capacity - initial_header_offset;

  if (!datacontainer_enough(&dc, exact_remaining)) {
    LOGERR("datacontainer_enough reported false for exact remaining space");
    goto error;
  }

  if (datacontainer_enough(&dc, exact_remaining + 1)) {
    LOGERR("datacontainer_enough reported true for space exceeding capacity");
    goto error;
  }
  LOGDBG("datacontainer_enough bounds test passed");

  // Prepare Line Input (Stroke style)
  input_lc.page = 1;
  input_lc.style = JDDC_LINESTYLE_STROKE;
  input_lc.layer = 2;
  input_lc.width = 5;
  input_lc.color = 0xABCD;
  input_lc.length = 2;

  input_lc.lines[0].x1 = 10;
  input_lc.lines[0].y1 = 20;
  input_lc.lines[0].x2 = 15;
  input_lc.lines[0].y2 = 25;

  input_lc.lines[1].x1 = 15;
  input_lc.lines[1].y1 = 25;
  input_lc.lines[1].x2 = 30;
  input_lc.lines[1].y2 = 40;

  // Add Line to DataContainer
  if (datacontainer_addline(&dc, &input_lc) != 0) {
    LOGERR("Failed to add line to DataContainer");
    goto error;
  }
  LOGDBG("Line added successfully");

  // Test Scanning and Parsing
  DataScanner scanner = datacontainer_get_scanner(&dc);
  DataScannerResult result;

  if (!datascanner_next_loop(&scanner, &result)) {
    LOGERR("Scanner failed to retrieve added stroke");
    goto error;
  }

  if (result.stroke_start == NULL) {
    LOGERR("Scanner returned NULL stroke pointer");
    goto error;
  }

  if (datascannerresult_parseline(&result, &output_lc) != 0) {
    LOGERR("Failed to parse line from scan result");
    goto error;
  }

  // Validate Decoded Line
  if (output_lc.style != input_lc.style ||
    output_lc.layer != input_lc.layer ||
    output_lc.width != input_lc.width ||
    output_lc.color != input_lc.color ||
    output_lc.length != input_lc.length) {
    LOGERR("Parsed line metadata mismatch");
    goto error;
  }

  for (u16 i = 0; i < input_lc.length; i++) {
    if (output_lc.lines[i].x1 != input_lc.lines[i].x1 ||
      output_lc.lines[i].y1 != input_lc.lines[i].y1 ||
      output_lc.lines[i].x2 != input_lc.lines[i].x2 ||
      output_lc.lines[i].y2 != input_lc.lines[i].y2) {
      LOGERR("Parsed line segment coordinate mismatch at index %d", i);
      goto error;
    }
  }

  // Test Scanner End condition
  if (datascanner_next_loop(&scanner, &result)) {
    LOGERR("Scanner found unexpected extra data");
    goto error;
  }

  // Clean up resources on success
  linecontainer_free(&output_lc);
  linecontainer_free(&input_lc);
  datacontainer_free(&dc);

  LOGINF("PASS: DataContainer test suite completed successfully");
  return 0;

error:
  linecontainer_free(&output_lc);
  linecontainer_free(&input_lc);
  datacontainer_free(&dc);
  return 1;
}



// Helper function to append a sample stroke to a container on a specified page
static int helper_add_sample_stroke(DataContainer *dc, page_t page, u16 color) {
  LineContainer lc;
  if (linecontainer_init_stroke(&lc) != 0) return 1;

  lc.page = page;
  lc.style = JDDC_LINESTYLE_STROKE;
  lc.layer = 0;
  lc.width = 1;
  lc.color = color;
  lc.length = 1;
  lc.lines[0].x1 = 10;
  lc.lines[0].y1 = 10;
  lc.lines[0].x2 = 20;
  lc.lines[0].y2 = 20;

  int res = datacontainer_addline(dc, &lc);
  linecontainer_free(&lc);
  return res;
}

// Helper function to count strokes and sum colors on a given page
static int helper_check_page_strokes(DataContainer *dc, page_t page, 
                                     u32 *out_count, u16 *out_color_sum) {
  LineContainer lc;
  if (linecontainer_init_stroke(&lc) != 0) return 1;

  *out_count = 0;
  *out_color_sum = 0;

  DataScanner scanner = datacontainer_get_scanner(dc);
  scanner.page = page;
  DataScannerResult result;

  while (datascanner_next_loop(&scanner, &result)) {
    if (datascannerresult_parseline(&result, &lc) != 0) {
      linecontainer_free(&lc);
      return 1;
    }
    (*out_count)++;
    *out_color_sum += lc.color;
  }

  linecontainer_free(&lc);
  return 0;
}

int run_edit_test_suite(void) {
  LOGINF("Starting Edit Test Suite...");

  DataContainer dc;
  if (datacontainer_init(&dc, 4096) != 0) {
    LOGERR("Failed to initialize DataContainer");
    return 1;
  }

  u32 count = 0;
  u16 color_sum = 0;

  // Populate initial data: Page 0 (color 0x1111), Page 1 (color 0x2222)
  if (helper_add_sample_stroke(&dc, 0, 0x1111) != 0 ||
    helper_add_sample_stroke(&dc, 1, 0x2222) != 0) {
    LOGERR("Failed to add initial strokes");
    goto error;
  }

  LOGTRC("Fill after init strokes: %zu", datacontainer_filled(&dc));

  // Test edit_copy_page (Copy page 0 to page 2)
  if (edit_copy_page(&dc, 0, 2) != 0) {
    LOGERR("edit_copy_page failed");
    goto error;
  }

  if (helper_check_page_strokes(&dc, 2, &count, &color_sum) != 0 || count != 1 || color_sum != 0x1111) {
    LOGERR("Page copy validation failed");
    goto error;
  }
  LOGDBG("edit_copy_page test passed");

  // Test edit_swap_pages (Swap page 0 [0x1111] and page 1 [0x2222])
  if (edit_swap_pages(&dc, 0, 1) != 0) {
    LOGERR("edit_swap_pages failed");
    goto error;
  }

  if (helper_check_page_strokes(&dc, 0, &count, &color_sum) != 0 || count != 1 || color_sum != 0x2222) {
    LOGERR("Page swap validation failed for page 0");
    goto error;
  }
  if (helper_check_page_strokes(&dc, 1, &count, &color_sum) != 0 || count != 1 || color_sum != 0x1111) {
    LOGERR("Page swap validation failed for page 1");
    goto error;
  }
  LOGDBG("edit_swap_pages test passed");

  // Test edit_move_page (Move page 0 -> page 2, shifting page 1 and 2 left)
  // Current setup: Page 0 (0x2222), Page 1 (0x1111), Page 2 (0x1111)
  if (edit_move_page(&dc, 0, 2) != 0) {
    LOGERR("edit_move_page failed");
    goto error;
  }

  // After moving page 0 to 2: Page 0 should have 0x1111 (old pg 1), Page 2 should have 0x2222 (old pg 0)
  if (helper_check_page_strokes(&dc, 0, &count, &color_sum) != 0 || count != 1 || color_sum != 0x1111) {
    LOGERR("Page move validation failed for page 0");
    goto error;
  }
  if (helper_check_page_strokes(&dc, 2, &count, &color_sum) != 0 || count != 1 || color_sum != 0x2222) {
    LOGERR("Page move validation failed for page 2");
    goto error;
  }
  LOGDBG("edit_move_page test passed");

  // Test edit_delete_page (Delete page 0)
  if (edit_delete_page(&dc, 0) != 0) {
    LOGERR("edit_delete_page failed");
    goto error;
  }

  if (helper_check_page_strokes(&dc, 0, &count, &color_sum) != 0 || count != 0) {
    LOGERR("Page delete failed to remove strokes from page 0");
    goto error;
  }
  LOGDBG("edit_delete_page test passed");

  datacontainer_free(&dc);
  LOGINF("PASS: Edit test suite completed successfully");
  return 0;

error:
  datacontainer_free(&dc);
  return 1;
}




int test_layer_sw_to_hw_display(C3D_RenderTarget *bottom_target) {
  LOGINF("Starting Layer Software-to-Hardware Display Test...");

  Layer sw_layer;
  Layer hw_layer;
  int ret = 0;

  // Initialize Software Layer (320x240 for 3DS Bottom Screen)
  if (layer_init(&sw_layer, 320, 240, JDL_TYPE_SOFTWARE) != 0) {
    LOGERR("Failed to initialize software layer");
    return 1;
  }

  if (layer_init(&hw_layer, 320, 240, JDL_TYPE_HARDWARE) != 0) {
    LOGERR("Failed to initialize hardware layer");
    layer_free(&sw_layer);
    return 1;
  }

  // Clear software layer to fully opaque dark gray
  LOGTRC("Clearing software layer");
  layer_clear(&sw_layer, rgb24_to_rgba32c(0x444444));

  // Define lines to draw numbers "1" and "2"
  RenderLine lines[] = {
    // Digit "1" (Cyan) - X offset ~ 100
    { .x1 = 110, .y1 = 80,  .x2 = 110, .y2 = 160, .width = 4, 
      .color = rgb24_to_rgba32c(0x00FFFF) },

    // Digit "2" (inverted top to bottom, oops) (Yellow) - X offset ~ 180
    { .x1 = 180, .y1 = 80,  .x2 = 220, .y2 = 80,  .width = 4, 
      .color = rgb24_to_rgba32c(0xFFFF00) }, // Top
    { .x1 = 220, .y1 = 80,  .x2 = 220, .y2 = 120, .width = 4, 
      .color = rgb24_to_rgba32c(0xFFFF00) }, // Top Right
    { .x1 = 220, .y1 = 120, .x2 = 180, .y2 = 120, .width = 4, 
      .color = rgb24_to_rgba32c(0xFFFF00) }, // Middle
    { .x1 = 180, .y1 = 120, .x2 = 180, .y2 = 160, .width = 4, 
      .color = rgb24_to_rgba32c(0xFFFF00) }, // Bottom Left
    { .x1 = 180, .y1 = 160, .x2 = 220, .y2 = 160, .width = 4, 
      .color = rgb24_to_rgba32c(0xFFFF00) }  // Bottom
  };

  size_t line_count = sizeof(lines) / sizeof(lines[0]);

  aptMainLoop();
  start_frame();
  end_frame();
  
  LOGTRC("Drawing lines on software layer");
  layer_drawlines(&sw_layer, lines, line_count);

  // Also, see if the hardware layers work (they crash)
  aptMainLoop();
  start_frame();
  LOGTRC("Clearing hardware layer");
  layer_clear(&hw_layer, rgb24_to_rgba32c(0xFF0000));
  LOGTRC("Drawing lines on hardware layer");
  layer_drawlines(&hw_layer, lines, line_count);
  end_frame();

  // Copy/Convert Software layer -> Hardware layer (VRAM)
#ifndef LSH_NOCOPY
  if (layer_copy(&hw_layer, &sw_layer) != 0) {
    LOGERR("Failed to copy software layer to hardware layer");
    goto ERROR;
  }
#endif

  char outpath[80] = "/3dsjunkdrawtest1.png";
  LOGTRC("Writing png to %s", outpath);

  // Export the png
  layer_export_png(&sw_layer, outpath);

  LOGINF("Render ready! Press A on the 3DS to exit test...");

  // Render loop: Draw hardware layer image to bottom screen until 'A' is pressed
  while (aptMainLoop()) {
    hidScanInput();
    u32 kDown = hidKeysDown();

    if (kDown & KEY_A) {
      LOGDBG("A pressed, exiting display test.");
      break;
    }

    start_frame();
    C2D_TargetClear(bottom_target, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(bottom_target);
    // Draw the converted hardware layer texture onto the screen target
    S32Bounds bounds;
    layer_mapped_area(&hw_layer, &bounds);
    //LOGTRC("BX1: %d BY1: %d", bounds.x1, bounds.y1);
    C2D_DrawImageAt(hw_layer.texture.hw.image, -bounds.x1, -bounds.y1, 0.5f, NULL, 1.0f, 1.0f);
    end_frame();
  }

  // Make a really obvious clear color on sw
  layer_clear(&sw_layer, rgb24_to_rgba32c(0xFF0000));

  // Go back and forth a couple times, see if colors decay a bit
  for(int i = 2; i < 5; i+=1) {

    if (layer_copy(&sw_layer, &hw_layer) != 0) {
      LOGERR("Failed to copy hardware layer to software layer");
      goto ERROR;
    }

    sprintf(outpath, "/3dsjunkdrawtest%d.png", i);
    LOGTRC("Writing png to %s", outpath);

    // Export the png
    layer_export_png(&sw_layer, outpath);

    if (layer_copy(&hw_layer, &sw_layer) != 0) {
      LOGERR("Failed to copy software layer to hardware layer %d", i);
      goto ERROR;
    }
  }

  LOGDBG("Checking color query");
  if(sw_layer.type != JDL_TYPE_SOFTWARE) {
    LOGERR("Software layer no longer software???");
    goto ERROR;
  }
  if(hw_layer.type != JDL_TYPE_HARDWARE) {
    LOGERR("Hardware layer no longer hardware???");
    goto ERROR;
  }
  // At this point, both layers should have the same values, so let's see if that's the case.
  u32 swcolor = layer_querycolor(&sw_layer, 110, 80);
  u32 hwcolor = layer_querycolor(&hw_layer, 110, 80);
  u32 basecolor = rgb24_to_rgba32c(0x00FFFF);
  LOGTRC("SWCOL: %08X HWCOL: %08X BCOL: %08X", swcolor, hwcolor, basecolor);

  if(swcolor != hwcolor) {
    LOGERR("Cyan soft/hard mismatches!");
    goto ERROR;
  }
  // SPECIFICALLY use the old function!!!!
  if(swcolor != basecolor) {
    LOGERR("Cyan mismatches base color!");
    goto ERROR;
  }

  swcolor = layer_querycolor(&sw_layer, 180, 80);
  hwcolor = layer_querycolor(&hw_layer, 180, 80);
  basecolor = rgb24_to_rgba32c(0xFFFF00);
  LOGTRC("SWCOL: %08X HWCOL: %08X BCOL: %08X", swcolor, hwcolor, basecolor);

  if(swcolor != hwcolor) {
    LOGERR("Yellow soft/hard mismatches!");
    goto ERROR;
  }
  // SPECIFICALLY use the old function!!!!
  if(swcolor != basecolor) {
    LOGERR("CYellow mismatches base color!");
    goto ERROR;
  }


  // Cleanup resources
  layer_free(&hw_layer);
  layer_free(&sw_layer);

  LOGINF("PASS: Layer display test completed successfully");
  return ret;

ERROR:
  layer_free(&hw_layer);
  layer_free(&sw_layer);
  return 1;
}



int utils_test(void) {
  LOGINF("Starting Utils Test...");
  // Let's see if some crazy numbers are the same in both directions...
  for(u16 i = 0; i < 65535; i++) {
    u32 col = rgba5551_to_abgr8(i);
    u16 back = abgr8_to_rgba5551(col);
    if(back != i) {
      LOGERR("FAILED AT 0x%04X != 0x%04X", i, back);
      return 1;
    }
    u32 col2 = rgba16_to_abgr8(i);
    u32 back2 = abgr8_to_rgba16(col2);
    if(back2 != i) {
      LOGERR("FAILED(2) AT 0x%04X != 0x%04X", i, back2);
      return 1;
    }
  }

  LOGINF("PASS: Utils Test");
  return 0;
}




int run_lineconverter_test_suite(void) {
  LOGINF("Starting LineConverter Test Suite...");

  LineConverter lc;
  if (lineconverter_init(&lc) != 0) {
    LOGERR("Failed to initialize LineConverter");
    return 1;
  }

  // --------------------------------------------------------------------------
  // Basic conversion, color bit-packing, and batch conversion
  // --------------------------------------------------------------------------

  // Set pending stroke parameters: Layer 0, Color 0xF800 (Pure Red in RGBA5551: R=31, G=0, B=0, A=0)
  lc.pending.layer = 0;
  lc.pending.color = 0xF800; 
  lc.pending.width = 2;
  lc.pending.length = 5;

  for (u16 i = 0; i < lc.pending.length; i++) {
    lc.pending.lines[i] = (LineSegment){ .x1 = i, .y1 = i, .x2 = i + 1, .y2 = i + 1 };
  }

  // Metered conversion: Convert first 2 lines
  size_t converted = lineconverter_convert(&lc, 2);
  if (converted != 2 || lc.pending_next != 2) {
    LOGERR("Incremental convert (batch 1) failed: converted %zu, expected 2", converted);
    goto error;
  }
  if (lineconverter_done(&lc)) {
    LOGERR("lineconverter_done reported prematurely true");
    goto error;
  }

  // Convert remaining lines (request 10, should cap at remaining 3)
  converted = lineconverter_convert(&lc, 10);
  if (converted != 3 || lc.pending_next != 5) {
    LOGERR("Incremental convert (batch 2) failed: converted %zu, expected 3", converted);
    goto error;
  }
  if (!lineconverter_done(&lc)) {
    LOGERR("lineconverter_done reported false when pending lines exhausted");
    goto error;
  }

  // Validate converted layer structure and output array
  if (lc.lines.length <= 0 || lc.lines.array[0].length != 5) {
    LOGERR("Converted RenderLine vector length mismatch");
    goto error;
  }

  // Check color expansion: RGBA5551 -> ABGR8 
  u32 expected_color = rgba16_to_abgr8(0xF800);
  if (lc.lines.array[0].array[0].color != expected_color) {
    LOGERR("Color conversion mismatch: got 0x%08X, expected 0x%08X", 
           lc.lines.array[0].array[0].color, expected_color);
    goto error;
  }
  if (lc.lines.array[0].array[0].width != 2) {
    LOGERR("RenderLine width mismatch");
    goto error;
  }
  LOGDBG("Basic conversion and batching test passed");

  // --------------------------------------------------------------------------
  // State Reset functions (reset_pending vs reset_converted)
  // --------------------------------------------------------------------------
  lineconverter_reset_converted(&lc);
  for(int i = 0; i < lc.lines.length; i++) {
    if(lc.lines.array[i].length != 0) {
      LOGERR("Failed to reset converted lines");
      goto error;
    }
  }
  if(lc.pending_next == 0 || lc.pending.length == 0) {
    LOGERR("Pending got reset when only converted requested");
    goto error;
  }
  lineconverter_reset_pending(&lc);
  if(lc.pending_next != 0 || lc.pending.length != 0) {
    LOGERR("Failed to reset pending line");
    goto error;
  }

  // --------------------------------------------------------------------------
  // Tests out-of-bounds layer growth and vector initialization in nested vectors
  // --------------------------------------------------------------------------

  lc.pending.lines[0] = (LineSegment){ .x1 = 10, .y1 = 10, .x2 = 20, .y2 = 20 };
  lc.pending.length = 1;
  lc.pending.layer = 5; // Higher layer index forces intermediate vector allocations

  lineconverter_convert(&lc, 1);

  LOGTRC("VVL: %zu V5L: %zu", lc.lines.length, lc.lines.array[5].length);

  if (lc.lines.length < 6) {
    LOGERR("Outer vector failed to auto-expand to accommodate Layer 5 (was: %d)",
           lc.lines.length);
    goto error;
  }
  if (lc.lines.array[5].length != 1) {
    LOGERR("Layer 5 vector did not receive the converted RenderLine");
    goto error;
  }
  LOGDBG("Layer growth test passed");

  lineconverter_reset_pending(&lc);
  if (lc.pending_next != 0 || lc.pending.length != 0) {
    LOGERR("lineconverter_reset_pending failed to zero pending stroke indicators");
    goto error;
  }
  // Converted lines should remain intact across reset_pending
  if (lc.lines.array[5].length != 1) {
    LOGERR("lineconverter_reset_pending accidentally cleared converted lines");
    goto error;
  }

  lineconverter_reset_converted(&lc);
  // Layers vector structure remains, but inner lengths reset to 0
  if (lc.lines.array[0].length != 0 || lc.lines.array[5].length != 0) {
    LOGERR("lineconverter_reset_converted failed to clear inner RenderLine vectors");
    goto error;
  }
  LOGDBG("Reset functions test passed");

  // --------------------------------------------------------------------------
  // Edge Cases 
  // --------------------------------------------------------------------------
  // Edge Case A: Converting 0 lines
  converted = lineconverter_convert(&lc, 0);
  if (converted != 0) {
    LOGERR("Converting 0 lines returned non-zero count: %zu", converted);
    goto error;
  }

  // Edge Case B: Converting when pending stroke is empty
  lc.pending.length = 0;
  lc.pending_next = 0;
  converted = lineconverter_convert(&lc, 5);
  if (converted != 0 || !lineconverter_done(&lc)) {
    LOGERR("Converting empty pending stroke failed to return 0 / set done flag");
    goto error;
  }

  // Edge Case C: Full reset & re-conversion pass
  lineconverter_reset(&lc);
  if (lc.pending_next != 0 || lc.pending.length != 0) {
    LOGERR("lineconverter_reset failed full state purge");
    goto error;
  }

  lineconverter_free(&lc);
  LOGINF("PASS: LineConverter test suite completed successfully");
  return 0;

error:
  lineconverter_free(&lc);
  return 1;
}




#ifdef LWP_SOFT
#define LWP_TYPE JDL_TYPE_SOFTWARE
#else
#define LWP_TYPE JDL_TYPE_HARDWARE
#endif

int test_layerwindow_cache_and_pull(C3D_RenderTarget *bottom_target) {
  LOGINF("Starting LayerWindow Cache and Pull Test...");

  LOGTRC("Testing basic initialization");
  DataContainer dc;
  LayerWindow lw;
  int ret = 0;

  // Initialize Data Container with sufficient space
  if (datacontainer_init(&dc, 65536) != 0) {
    LOGERR("Failed to initialize DataContainer");
    return 1;
  }

  // Initialize LayerWindow using HARDWARE layers
  if (layerwindow_init(&lw, &dc, LWP_TYPE) != 0) {
    LOGERR("Failed to initialize LayerWindow");
    datacontainer_free(&dc);
    return 1;
  }

  LOGTRC("Testing reset (layer creation)");

  // Allocate 3 units, each having 2 layers 
  if (layerwindow_reset(&lw, LWP_WIDTH, LWP_HEIGHT, 2, 3) != 0) {
    LOGERR("Failed to reset LayerWindow with 3 units of 2 layers");
    ret = 1;
    goto CLEANUP;
  }

  if(lw.unit_count != 3) {
    LOGERR("LayerWindow not 3 units");
    ret = 1;
    goto CLEANUP;
  }
  for(int i = 0; i < 3; i++) {
    if(lw.units[i].layer_count != 2) {
      LOGERR("Unit[%d] not 2 layers", i);
      ret = 1;
      goto CLEANUP;
    }
    for(int j = 0; j < 2; j++) {
      if(lw.units[i].layers[j].type != LWP_TYPE) {
        LOGERR("Layer[%d][%d] not hardware", i, j);
        ret = 1;
        goto CLEANUP;
      }
      if(lw.units[i].layers[j].width != LWP_WIDTH|| 
         lw.units[i].layers[j].height != LWP_HEIGHT) {
        LOGERR("Layer[%d][%d] not %dx%d", i, j, LWP_WIDTH, LWP_HEIGHT);
        ret = 1;
        goto CLEANUP;
      }
    }
  }

  // My weird color format is specifically high bit A: ARRR RRGG GGGB BBBB
  u16 col_red    = 0xFC00;
  u16 col_green  = 0x83E0;
  u16 col_blue   = 0x801F;
  u16 col_yellow = 0xFFE0;
  u16 col_cyan   = 0x83FF;
  u16 col_magenta= 0xFC1F;

  // Set up lines across 3 pages (0, 1, 2) and 2 layers (0, 1) interleaved
  struct {
    page_t page;
    layer_t layer;
    LineSegment segment;
    u16 color;
  } test_strokes[] = {
    // Page 0, Layer 1: Horizontal Bar
    { .page = 0, .layer = 1, .segment = {10, 32, 54, 32}, .color = col_green },
    // Page 2, Layer 0: Main Diagonal
    { .page = 2, .layer = 0, .segment = {10, 10, 54, 54}, .color = col_cyan },
    // Page 1, Layer 0: Vertical Bar
    { .page = 1, .layer = 0, .segment = {32, 10, 32, 54}, .color = col_blue },
    // Page 0, Layer 0: Top-Left Cross
    { .page = 0, .layer = 0, .segment = {10, 10, 30, 30}, .color = col_red },
    // Page 2, Layer 1: Box Border Edge
    { .page = 2, .layer = 1, .segment = {10, 50, 54, 50}, .color = col_magenta },
    // Page 1, Layer 1: Anti-Diagonal
    { .page = 1, .layer = 1, .segment = {54, 10, 10, 54}, .color = col_yellow },
  };

  LOGTRC("Adding strokes to data container");
  size_t stroke_count = sizeof(test_strokes) / sizeof(test_strokes[0]);

  for (size_t i = 0; i < stroke_count; i++) {
    LineContainer lc;
    if (linecontainer_init_stroke(&lc) != 0) {
      LOGERR("Failed to init LineContainer stroke");
      ret = 1;
      goto CLEANUP;
    }

    lc.page = test_strokes[i].page;
    lc.layer = test_strokes[i].layer;
    lc.color = test_strokes[i].color;
    lc.width = 3;
    lc.style = JDDC_LINESTYLE_STROKE;
    lc.lines[0] = test_strokes[i].segment;
    lc.length = 1;

    if (datacontainer_addline(&dc, &lc) != 0) {
      LOGERR("Failed to add line to DataContainer");
      linecontainer_free(&lc);
      ret = 1;
      goto CLEANUP;
    }
    linecontainer_free(&lc);
  }

  LOGTRC("Pulling lines...");

  start_frame();
  PageRange prange = { .page = 0, .offset = 2, .loop_point = 0 };
  // Initial All-at-Once Pull: pull 3 pages starting at page 0
  if (layerwindow_pull(&lw, 100000, 1000, prange) != 0) {
    LOGERR("Failed initial full layerwindow_pull");
    ret = 1;
    goto CLEANUP;
  }
  end_frame();

#ifdef LWP_SOFT
  char outpath[80];
  for(int i = 0; i < lw.master_layers_length; i++) {
    sprintf(outpath, "/3dsjunkdrawtest_pull%d.png", i);
    LOGTRC("Writing png to %s", outpath);
    Layer * layer = lw.master_layers + i;
    layer_export_png(layer, outpath);
  }
#else
  LOGINF("Initial pull complete. Displaying 2x3 grid on bottom target.");
  LOGINF("Press 'A' to proceed to metered draw verification...");

  // Render Loop: Display 3 pages (rows) x 2 layers (cols) grid
  while (aptMainLoop()) {
    hidScanInput();
    u32 kDown = hidKeysDown();

    if (kDown & KEY_A) {
      LOGDBG("A pressed, continuing metered draw test.");
      break;
    }

    start_frame();
    C2D_TargetClear(bottom_target, C2D_Color32(20, 20, 20, 255));
    C2D_SceneBegin(bottom_target);

    // Grid config: sub-textures drawn across bottom screen (320x240)
    for (int p = 0; p < 3; p++) {
      size_t unit_idx = p % lw.unit_count;
      LayerWindowUnit *unit = &lw.units[unit_idx];

      for (int l = 0; l < 2; l++) {
        float posX = 40.0f + (l * 120.0f);
        float posY = 10.0f + (p * 75.0f);

        if (unit->layers[l].type == JDL_TYPE_HARDWARE) {
          C2D_DrawImageAt(unit->layers[l].texture.hw.image, posX, posY, 0.5f, NULL, 1.0f, 1.0f);
        }
      }
    }

    end_frame();
  }
#endif

  // --- Step 2: Metered Pull & Pending Unit Test ---
  LOGINF("Resetting layer cache to test metered pull/pending functionality...");

  // Reset layer window without freeing data container
  if (layerwindow_reset(&lw, 64, 64, 2, 3) != 0) {
    LOGERR("Failed to reset LayerWindow for metered draw");
    ret = 1;
    goto CLEANUP;
  }

  // Perform small metered pulls (limit max_draw to 1 line per call)
  // Call pull multiple times until all items are drawn into cache
  start_frame();
  int pull_attempts = 0;
  while (pull_attempts < 20) {
    layerwindow_pull(&lw, 5000, 1, prange);
    pull_attempts++;

    // Break early if pending unit cleared and scanners reached end of data
    int all_done = (lw.pending_unit == NULL);
    if (all_done) {
      for (size_t i = 0; i < lw.unit_count; i++) {
        if (!datascanner_at_end(&lw.units[i].scanner)) {
          all_done = 0;
          break;
        }
      }
    }
    if (all_done) break;
  }
  end_frame();

  LOGTRC("Metered pulls executed: %d attempts", pull_attempts);

  // Verification using layer_querycolor on expected drawn points
  LOGDBG("Verifying pixel colors on metered drawn layers...");

  // Test Page 0, Layer 0 (Top-left cross point at 20, 20)
  size_t p0_unit = 0 % lw.unit_count;
  u32 color_p0_l0 = layer_querycolor(&lw.units[p0_unit].layers[0], 20, 20);

  // Test Page 1, Layer 0 (Vertical bar point at 32, 32)
  size_t p1_unit = 1 % lw.unit_count;
  u32 color_p1_l0 = layer_querycolor(&lw.units[p1_unit].layers[0], 32, 32);

  // Test Page 2, Layer 1 (Box border edge point at 32, 50)
  size_t p2_unit = 2 % lw.unit_count;
  u32 color_p2_l1 = layer_querycolor(&lw.units[p2_unit].layers[1], 32, 50);

  LOGTRC("P0L0 Col: 0x%08X, P1L0 Col: 0x%08X, P2L1 Col: 0x%08X", 
         color_p0_l0, color_p1_l0, color_p2_l1);

  // Verify non-zero/non-clear color exists at target drawn pixels
  if (color_p0_l0 == JDLC_CLEAR) {
    LOGERR("Color Query Failed: Page 0 Layer 0 line missing!");
    ret = 1;
    goto CLEANUP;
  }
  if (color_p1_l0 == JDLC_CLEAR) {
    LOGERR("Color Query Failed: Page 1 Layer 0 line missing!");
    ret = 1;
    goto CLEANUP;
  }
  if (color_p2_l1 == JDLC_CLEAR) {
    LOGERR("Color Query Failed: Page 2 Layer 1 line missing!");
    ret = 1;
    goto CLEANUP;
  }

  LOGINF("PASS: LayerWindow cache, metered pull, and pending unit tests passed!");

CLEANUP:
  layerwindow_free(&lw);
  datacontainer_free(&dc);
  return ret;
}



// ==========================
//      MAIN (OBVIOUSLY)
// ==========================

int main(int argc, char **argv) {
  gfxInitDefault();
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

  logbuffer_init(&logbuf, 100, 128);

  // Enable the higher clock speed on New 3DS
  osSetSpeedupEnable(true);

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  C3D_DepthTest(false, GPU_ALWAYS, GPU_WRITE_COLOR);

  //PrintConsole * console_ptr = 
  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  LOGTRC("INITIALIZED");

  // Run tests... here?? Or run some amount of tests per frame maybe...
#ifndef DC_SKIP
  if(run_datacontainer_test_suite()) {
    LOGERR("DATA CONTAINER FAILED");
    goto SKIPTESTS;
  }
#endif
#ifndef EDIT_SKIP
  if(run_edit_test_suite()) {
    LOGERR("EDIT FAILED");
    goto SKIPTESTS;
  }
#endif
#ifndef UTILS_SKIP
  if(utils_test()) {
    LOGERR("UTILS FAILED");
    goto SKIPTESTS;
  }
#endif
#ifndef LC_SKIP
  if(run_lineconverter_test_suite()) {
    LOGERR("LINECONVERTER FAILED");
    goto SKIPTESTS;
  }
#endif
#ifndef LWP_SKIP
  if(test_layerwindow_cache_and_pull(screen)) {
    LOGERR("LAYERWINDOW FAILED");
    goto SKIPTESTS;
  }
#endif
  if(test_layer_sw_to_hw_display(screen)) {
    LOGERR("LAYER FAILED");
    goto SKIPTESTS;
  }

  LOGINF("-- DONE --");

SKIPTESTS:;

  while (aptMainLoop()) {
    hidScanInput();

    u32 kUp = hidKeysUp();
    //u32 kDown = hidKeysDown();
    //u32 kRepeat = hidKeysDownRepeat();
    //u32 kHeld = hidKeysHeld();
    circlePosition pos;
    touchPosition current_touch;
    hidTouchRead(&current_touch);
    hidCircleRead(&pos);

    if(kUp & KEY_START) {
      break;
    }

    // =======================================
    // Render the scene
    // =======================================
    start_frame();


    // -- OTHER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA);

    C2D_TargetClear(screen, 0xFFFFFFFF);
    C2D_SceneBegin(screen);

    // draw_layers(&layer_window, &sys);
    // draw_scrollbars(&sys.screen_state);
    // draw_colorpicker(&sys.colors, !sstate.palette_active);

    end_frame();
  }

  C3D_RenderTargetDelete(screen);
  logbuffer_free(&logbuf);

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}
