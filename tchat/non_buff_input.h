/* makes console input non-buffered
 * this is needed so the input buffer is updated on every keystroke */
#ifndef NON_BUFF_INPUT_

#define NON_BUFF_INPUT_

#ifdef LINUX
#include <stdio.h>
#include <termios.h>

void non_buff_in(void)
{
	/* Linux specific */
	static struct termios oldt, newt;

    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);          
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    return;
}

#define getch_() getchar()

#else
#include <conio.h>
#define getch_() getche()
#endif

#endif
