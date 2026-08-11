#include "3ds/os.h"
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
#include "filesys.h"
#include "lineconversion.h"

#define BREPEAT_DELAY 20
#define BREPEAT_INTERVAL 7

#define STATUS_TRACE 35     // magenta?   //34     // blue?
#define STATUS_DEBUG 36     // teal?
#define STATUS_INFO 37      // white?
#define STATUS_WARNING 33   // yellow?
#define STATUS_ERROR 31     // red?

// So apparently printf doesn't work unless you do the standard Citro3D frame
// stuff. It only SOMETIMES works, IDK. So, this will wait a full frame to show
// the given message, basically forcing it to be shown even if you're about to
// do a long running task.
void printf_flush(const char *format, ...) {
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  C3D_FrameEnd(0);
}

static inline void logbase(u8 color, const char * fmt, va_list args) {
  static u8 _db_prnt_num = 0;
  //printf("\x1b[%d;1H%50s", _db_prnt_row + DEBUG_PRINT_MINROW, "");
  printf("\x1b[%dm", color);
  _db_prnt_num = (_db_prnt_num + 1) % 100;
  time_t rawtime = time(NULL);
  struct tm *timeinfo = localtime(&rawtime);
  printf("[%02d|%02d:%02d] ", _db_prnt_num, timeinfo->tm_hour, timeinfo->tm_min);
  vprintf(fmt, args);
  printf_flush("\n");
}

void LOGERR(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_ERROR, fmt, args);
  va_end(args);
}

void LOGWRN(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_WARNING, fmt, args);
  va_end(args);
}

void LOGINF(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_INFO, fmt, args);
  va_end(args);
}

void LOGDBG(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_DEBUG, fmt, args);
  va_end(args);
}

void LOGTRC(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logbase(STATUS_TRACE, fmt, args);
  va_end(args);
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
  layer_clear(&sw_layer, rgb24_to_rgba32c(0x444444));

  // Define lines to draw numbers "1" and "2"
  RenderLine lines[] = {
    // Digit "1" (Cyan) - X offset ~ 100
    { .x1 = 110, .y1 = 80,  .x2 = 110, .y2 = 160, .width = 4, .color = rgb24_to_rgba32c(0x00FFFF) },

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

  //TickCounter timer;
  //osTickCounterStart(&timer);
  // Draw lines onto the SOFTWARE layer
  layer_drawlines(&sw_layer, lines, line_count);
  // osTickCounterUpdate(&timer);
  // double ms = osTickCounterRead(&timer);
  // printf("TIMEDRAW: %0.3fms\n", ms);

  // Copy/Convert Software layer -> Hardware layer (VRAM)
  if (layer_copy(&hw_layer, &sw_layer) != 0) {
    LOGERR("Failed to copy software layer to hardware layer");
    goto ERROR;
  }

  char outpath[80] = "/3dsjunkdrawtest1.png";
  LOGTRC("Writing png to %s", outpath);

  // Export the png
  write_citropng(sw_layer.texture.sf.buf, sw_layer.texture.sf.width, 
                 sw_layer.texture.sf.height, outpath, 1);

  LOGINF("Render ready! Press A on the 3DS to exit test...");

  // Render loop: Draw hardware layer image to bottom screen until 'A' is pressed
  while (aptMainLoop()) {
    hidScanInput();
    u32 kDown = hidKeysDown();

    if (kDown & KEY_A) {
      LOGDBG("A pressed, exiting display test.");
      break;
    }

    C2D_TargetClear(bottom_target, C2D_Color32(0, 0, 0, 255));
    C2D_SceneBegin(bottom_target);

    // Draw the converted hardware layer texture onto the screen target
    C2D_DrawImageAt(hw_layer.texture.hw.image, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);

    C3D_FrameEnd(0);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
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
    write_citropng(sw_layer.texture.sf.buf, sw_layer.texture.sf.width, 
                   sw_layer.texture.sf.height, outpath, 1);

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
  u32 expected_color = rgba5551_to_abgr8(0xF800);
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



// ==========================
//      MAIN (OBVIOUSLY)
// ==========================

int main(int argc, char **argv) {
  gfxInitDefault();
  hidSetRepeatParameters(BREPEAT_DELAY, BREPEAT_INTERVAL);

  // Enable the higher clock speed on New 3DS
  osSetSpeedupEnable(true);

  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();

  //PrintConsole * console_ptr = 
  consoleInit(GFX_TOP, NULL);
  C3D_RenderTarget *screen = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  LOGTRC("INITIALIZED");

  // Run tests... here?? Or run some amount of tests per frame maybe...
  if(run_datacontainer_test_suite()) {
    LOGERR("DATA CONTAINER FAILED");
    goto SKIPTESTS;
  }
  if(run_edit_test_suite()) {
    LOGERR("EDIT FAILED");
    goto SKIPTESTS;
  }
  if(run_lineconverter_test_suite()) {
    LOGERR("LINECONVERTER FAILED");
    goto SKIPTESTS;
  }
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
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // -- LAYER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_ONE, GPU_ZERO, GPU_ONE,
                   GPU_ZERO);

    C2D_Flush();

    // -- OTHER DRAW SECTION --
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA,
                   GPU_ONE_MINUS_SRC_ALPHA);

    C2D_TargetClear(screen, 0xFFFFFFFF);
    C2D_SceneBegin(screen);

    // draw_layers(&layer_window, &sys);
    // draw_scrollbars(&sys.screen_state);
    // draw_colorpicker(&sys.colors, !sstate.palette_active);

    C3D_FrameEnd(0);
  }
// ENDMAINLOOP:;

  C3D_RenderTargetDelete(screen);

  C2D_Fini();
  C3D_Fini();
  gfxExit();
  return 0;
}
