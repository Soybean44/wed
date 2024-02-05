#include <stdio.h>
#include <ncurses.h>

int main (int argc, char** argv) {
  (void)argc;
  (void)argv;
  // init screen and sets up screen
  initscr();

  // print to screen
  printw("Hello World");

  // refreshes the screen
  refresh();

  // pause the screen output
  getch();

  // deallocates memory and ends ncurses
  endwin();
  return 0;
}
