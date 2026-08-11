#ifndef __HEADER_JUNKDRAW_LINECONVERSION__
#define __HEADER_JUNKDRAW_LINECONVERSION__

#include "vector.h"
#include "layer.h"
#include "datacontainer.h"

VECTOR_DECLARE(RenderLine);
VECTOR_DECLARE(vector_RenderLine);

// Set a pending line, reset, then you can convert as much as you want into 
// the vector, which will always be able to hold the lines from LineContainer.
typedef struct {
  vector_vector_RenderLine lines; // One vector per layer
  LineContainer pending;
  size_t pending_next;
} LineConverter;

int lineconverter_init(LineConverter *);
void lineconverter_free(LineConverter * lc);
// Prep lineconverter to be ready for the next conversion. Clears out the 
// pending line container and all converted lines.
void lineconverter_reset(LineConverter * lc);
// Reset ONLY the converted lines, leaving the state of the pending line container
void lineconverter_reset_converted(LineConverter * lc);
void lineconverter_reset_pending(LineConverter * lc);
// Whether the converter is at the end of the pending line
int lineconverter_done(LineConverter * lc);
// Convert up to the given amount of individual lines, returning the amount
// actually converted (may convert less if there aren't enough)
size_t lineconverter_convert(LineConverter * lc, size_t count);

// Delete all inner lines, resetting the line vector completely.
// void lineconverter_delete_lines(LineConverter * lc);

#endif
