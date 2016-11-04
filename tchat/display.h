#ifndef DISPLAY_

#define DISPLAY_

void display_clear_screen(void);
void display_clear_line(void);
void display_move_cursor_xy(int row, int col);
void display_print_flush(const char * msg);

#endif
