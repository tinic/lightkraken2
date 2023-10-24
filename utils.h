#ifndef _UTILS_H_
#define _UTILS_H_

#define ESCAPE_RESET "\u001b[0m"

#define ESCAPE_FG_RED "\u001b[31m"
#define ESCAPE_FG_GREEN "\u001b[32m"
#define ESCAPE_FG_YELLOW "\u001b[33m"
#define ESCAPE_FG_BLUE "\u001b[34m"
#define ESCAPE_FG_MAGENTA "\u001b[35m"
#define ESCAPE_FG_CYAN "\u001b[36m"
#define ESCAPE_FG_WHITE "\u001b[37m"

#define ESCAPE_FG_BRIGHT_RED "\u001b[31;1m"
#define ESCAPE_FG_BRIGHT_GREEN "\u001b[32;1m"
#define ESCAPE_FG_BRIGHT_YELLOW "\u001b[33;1m"
#define ESCAPE_FG_BRIGHT_BLUE "\u001b[34;1m"
#define ESCAPE_FG_BRIGHT_MAGENTA "\u001b[35;1m"
#define ESCAPE_FG_BRIGHT_CYAN "\u001b[36;1m"
#define ESCAPE_FG_BRIGHT_WHITE "\u001b[37;1m"

#define ESCAPE_BG_RED "\u001b[41m"
#define ESCAPE_BG_GREEN "\u001b[42m"
#define ESCAPE_BG_YELLOW "\u001b[43m"
#define ESCAPE_BG_BLUE "\u001b[44m"
#define ESCAPE_BG_MAGENTA "\u001b[45m"
#define ESCAPE_BG_CYAN "\u001b[46m"
#define ESCAPE_BG_WHITE "\u001b[47m"

#define ESCAPE_BG_BRIGHT_RED "\u001b[41;1m"
#define ESCAPE_BG_BRIGHT_GREEN "\u001b[42;1m"
#define ESCAPE_BG_BRIGHT_YELLOW "\u001b[43;1m"
#define ESCAPE_BG_BRIGHT_BLUE "\u001b[44;1m"
#define ESCAPE_BG_BRIGHT_MAGENTA "\u001b[45;1m"
#define ESCAPE_BG_BRIGHT_CYAN "\u001b[46;1m"
#define ESCAPE_BG_BRIGHT_WHITE "\u001b[47;1m"

#define ESCAPE_BOLD "\u001b[1m"
#define ESCAPE_UNDERLINE "\u001b[4m"
#define ESCAPE_REVERSE "\u001b[7m"

#define ESCAPE_UP(N) "\u001b[N##A"
#define ESCAPE_DOWN(N) "\u001b[N##B"
#define ESCAPE_RIGHT(N) "\u001b[N##C"
#define ESCAPE_LEFT(N) "\u001b[N##D"

#endif  // #ifndef _UTILS_H_
