#ifndef __ANSI_H__
#define __ANSI_H__

// Formatting
#define ANSI_RESET          "\x1b[0m"
#define ANSI_BOLD_ON        "\x1b[1m"
#define ANSI_DIM_ON         "\x1b[2m"
#define ANSI_BOLD_DIM_OFF   "\x1b[22m"
#define ANSI_ITALIC_ON      "\x1b[3m"
#define ANSI_ITALIC_OFF     "\x1b[23m"
#define ANSI_UNDERLINE_ON   "\x1b[4m"
#define ANSI_UNDERLINE_OFF  "\x1b[24m"
#define ANSI_BLINK          "\x1b[5m"
#define ANSI_INVERT_ON      "\x1b[7m"
#define ANSI_INVERT_OFF     "\x1b[27m"
#define ANSI_HIDDEN         "\x1b[8m"
#define ANSI_STRIKETHROUGH  "\x1b[9m"

// Foreground
#define ANSI_FG_BLACK       "\x1b[30m"
#define ANSI_FG_RED         "\x1b[31m"
#define ANSI_FG_GREEN       "\x1b[32m"
#define ANSI_FG_YELLOW      "\x1b[33m"
#define ANSI_FG_BLUE        "\x1b[34m"
#define ANSI_FG_MAGENTA     "\x1b[35m"
#define ANSI_FG_CYAN        "\x1b[36m"
#define ANSI_FG_WHITE       "\x1b[37m"
#define ANSI_FG_DEFAULT     "\x1b[39m"

#define ANSI_FG_BRIGHT_BLACK   "\x1b[90m"
#define ANSI_FG_BRIGHT_RED     "\x1b[91m"
#define ANSI_FG_BRIGHT_GREEN   "\x1b[92m"
#define ANSI_FG_BRIGHT_YELLOW  "\x1b[93m"
#define ANSI_FG_BRIGHT_BLUE    "\x1b[94m"
#define ANSI_FG_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_FG_BRIGHT_CYAN    "\x1b[96m"
#define ANSI_FG_BRIGHT_WHITE   "\x1b[97m"

// Background
#define ANSI_BG_BLACK       "\x1b[40m"
#define ANSI_BG_RED         "\x1b[41m"
#define ANSI_BG_GREEN       "\x1b[42m"
#define ANSI_BG_YELLOW      "\x1b[43m"
#define ANSI_BG_BLUE        "\x1b[44m"
#define ANSI_BG_MAGENTA     "\x1b[45m"
#define ANSI_BG_CYAN        "\x1b[46m"
#define ANSI_BG_WHITE       "\x1b[47m"
#define ANSI_BG_DEFAULT     "\x1b[49m"

#define ANSI_BG_BRIGHT_BLACK   "\x1b[100m"
#define ANSI_BG_BRIGHT_RED     "\x1b[101m"
#define ANSI_BG_BRIGHT_GREEN   "\x1b[102m"
#define ANSI_BG_BRIGHT_YELLOW  "\x1b[103m"
#define ANSI_BG_BRIGHT_BLUE    "\x1b[104m"
#define ANSI_BG_BRIGHT_MAGENTA "\x1b[105m"
#define ANSI_BG_BRIGHT_CYAN    "\x1b[106m"
#define ANSI_BG_BRIGHT_WHITE   "\x1b[107m"

// Screen / Cursor
#define ANSI_CLEAR_SCREEN   "\x1b[2J"
#define ANSI_CLEAR_LINE     "\x1b[2K"
#define ANSI_HOME           "\x1b[H"
#define ANSI_CURSOR_HIDE    "\x1b[?25l"
#define ANSI_CURSOR_SHOW    "\x1b[?25h"

// Cursor Movement (1-based index)
#define ANSI_GOTO(row, col)         printf("\x1b[%d;%dH", (row), (col))
#define ANSI_CURSOR_UP(n)           printf("\x1b[%dA", (n))
#define ANSI_CURSOR_DOWN(n)         printf("\x1b[%dB", (n))
#define ANSI_CURSOR_RIGHT(n)        printf("\x1b[%dC", (n))
#define ANSI_CURSOR_LEFT(n)         printf("\x1b[%dD", (n))

// 256-Color extended (0-255)
#define ANSI_FG_256(color_code)     printf("\x1b[38;5;%dm", (color_code))
#define ANSI_BG_256(color_code)     printf("\x1b[48;5;%dm", (color_code))

// 24-bit truecolor / RGB Mode (0-255 per channel)
#define ANSI_FG_RGB(r, g, b)        printf("\x1b[38;2;%d;%d;%dm", (r), (g), (b))
#define ANSI_BG_RGB(r, g, b)        printf("\x1b[48;2;%d;%d;%dm", (r), (g), (b))

#endif
