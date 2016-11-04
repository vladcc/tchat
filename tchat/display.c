/* functions related to the screen and the text cursor */
#include "os_def.h"
#include "display.h"
#ifdef WINDOWS
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

// assumes a line of 80 characters (79 avoids word wrap)
#define LINE_LEN 79 

void display_clear_screen(void)
{
	/* clears the screen */
#ifdef WINDOWS
	system("cls");
#else
	system("clear");
#endif
	return;
}

void display_clear_line(void)
{
	/* print LINE_LEN number of spaces
	 * effectively clearing the current line */
	int line = LINE_LEN;

	putchar('\r');
	while (line-- > 0)
		putchar(' ');
		
	putchar('\r');

	return;
}

void display_move_cursor_xy(int row, int col)
{
	/* moves the cursor at the specified row and column */
#ifdef WINDOWS
	HANDLE h;
	COORD c;
	
	h = GetStdHandle(STD_OUTPUT_HANDLE);
	c.X = col;
	c.Y = row;
	SetConsoleCursorPosition(h, c);
#else
	printf("\e[%d;%dH", row, col);
	fflush(stdout);
#endif

	return;
}

void display_print_flush(const char * msg)
{
	/* print a string without a new line at the end */
	printf("%s", msg);
	/* flushing guarantees the message is printed */
	fflush(stdout);
	return;
}
