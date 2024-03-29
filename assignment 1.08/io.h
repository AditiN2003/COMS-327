#ifndef IO_H
# define IO_H
#include <vector>
class character_t;
typedef int16_t pair_t[2];

void io_init_terminal(void);
void io_reset_terminal(void);
void io_starter();
void io_display(void);
void io_handle_input(pair_t dest);
void io_queue_message(const char *format, ...);
void io_battle(character_t *aggressor, character_t *defender);

#endif