#include <assert.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} Data;

typedef struct {
	size_t begin;
	size_t end;
} Line;

typedef struct {
	Line *items;
	size_t count;
	size_t capacity;
} Lines;

#define ITEMS_INIT_CAPACITY (10 * 1024)

#define da_append(da, item)                                               \
  do {                                                                    \
    if ((da)->count >= (da)->capacity) {                                  \
      (da)->capacity =                                                    \
          (da)->capacity == 0 ? ITEMS_INIT_CAPACITY : (da)->capacity * 2; \
      (da)->items =                                                       \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items));    \
      assert((da)->items != NULL && "Buy more RAM lol");                    \
    }                                                                     \
    (da)->items[(da)->count++] = (item);                                  \
  } while (0)

#define da_reserve(da, desired_capacity)                               \
  do {                                                                 \
    if ((da)->capacity < desired_capacity) {                           \
      (da)->capacity = desired_capacity;                               \
      (da)->items =                                                    \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
      assert((da)->items != NULL && "Buy more RAM lol");                 \
    }                                                                  \
  } while (0)

#define refresh_screen(mode)\
  clear(); \
  printw("%s", editor.data.items);\
  move(editor.view_y-1, 0);\
  addstr(mode);\
  move(editor.cursor_y, editor.cursor_x);

typedef struct {
	Data data;
	Lines lines;
	size_t cursor;
	size_t cursor_x;
	size_t cursor_y;
	size_t view_x;
	size_t view_y;
} wed_Editor;

void wed_free_buffers(wed_Editor *e) {
	free(e->data.items);
	free(e->lines.items);
	e->data.items = NULL;
	e->lines.items = NULL;
}

void wed_recalculate_lines(wed_Editor* e) {
	memset(e->lines.items, 0, e->lines.capacity);
	e->lines.count = 0;
	Line curr_line = {0};
	for(size_t i=1; i<e->data.count; i++) {
		if (e->data.items[i] == '\n') {
			curr_line.end = i;
			da_append(&e->lines, curr_line);
			curr_line.begin = i+1;
		}
	}
}

void wed_load_file(wed_Editor* e, char* file_path) {
	int fd = -1;
	fd = open(file_path, O_CREAT, O_RDONLY);
	if (fd < 0) {
		perror("Error: Couldn't Open File");
		exit(1);
	}
	struct stat statbuf;
	if (stat(file_path, &statbuf) < 0) {
		perror("Error: Couldn't Get File Stats");
		exit(1);
	}
	size_t file_size = statbuf.st_size;
	da_reserve(&e->data, file_size);
	e->data.count = file_size;
	if(read(fd, e->data.items, e->data.count) < 0) {
		perror("Error: Couldn't Read from File");
		close(fd);
		exit(1);
	}
	close(fd);
}

void wed_write_to_file(wed_Editor* e, char* file_path) {
	FILE* f = fopen(file_path, "w+");
	if (f == NULL) {
		perror("Error: Couldn't Open File");
		exit(1);
	}
	fwrite(e->data.items, e->data.count, 1, f);
}

void wed_move_left(wed_Editor* e) {
	if (e->cursor_x != 0) {
		e->cursor_x--;
		e->cursor--;
	}
}

void wed_move_right(wed_Editor* e) {
	size_t eol = (e->lines.items[e->cursor_y].end-e->lines.items[e->cursor_y].begin);
	if (e->cursor_x < eol) {
		e->cursor_x++;
		e->cursor++;
	}
}

void wed_move_up(wed_Editor* e) {
	if (e->cursor_y != 0) {
		e->cursor_y--;
		size_t eol = (e->lines.items[e->cursor_y].end-e->lines.items[e->cursor_y].begin);
		if (e->cursor_x > eol) {
			e->cursor_x = eol;
		}
		e->cursor = e->lines.items[e->cursor_y].begin + e->cursor_x;
	}
}

void wed_move_down(wed_Editor* e) {
	if (e->cursor_y < e->lines.count) {
		e->cursor_y++;
		size_t eol = (e->lines.items[e->cursor_y].end-e->lines.items[e->cursor_y].begin);
		if (e->cursor_x > eol) {
			e->cursor_x = eol;
		}
		e->cursor = e->lines.items[e->cursor_y].begin + e->cursor_x;
	}
}

void wed_insert_char(wed_Editor* e, char letter) {
	da_append(&e->data,' ');
	memmove(&e->data.items[e->cursor+1],&e->data.items[e->cursor], e->data.count-e->cursor);
	e->data.items[e->cursor] = letter;
	e->lines.items[e->cursor_y].end += 1;
	for(size_t i = e->cursor_y+1; i<e->lines.count; i++) {
		e->lines.items[i].begin += 1;
		e->lines.items[i].end += 1;
	}
	wed_move_right(e);
}

void wed_backspace(wed_Editor* e) {
	if (e->cursor > 0) {
		memmove(&e->data.items[e->cursor-1],&e->data.items[e->cursor], e->data.count-e->cursor);
		e->data.count--;
		if (e->cursor_x > 0) {
			e->lines.items[e->cursor_y].end -= 1;
			for(size_t i = e->cursor_y+1; i<e->lines.count; i++) {
				e->lines.items[i].begin -= 1;
				e->lines.items[i].end -= 1;
			}
			wed_move_left(e);
		}
		if (e->cursor_x <= 0) {
			e->cursor_y--;
			e->cursor_x = (e->lines.items[e->cursor_y].end-e->lines.items[e->cursor_y].begin);
			e->lines.items[e->cursor_y].end = e->lines.items[e->cursor_y+1].end-1;
			memmove(&e->lines.items[e->cursor_y+1], &e->lines.items[e->cursor_y+2], e->lines.count-e->cursor_y-2);
			e->lines.count--;
		}
	}
}

void wed_new_line(wed_Editor* e) {
	da_append(&e->data,' ');
	memmove(&e->data.items[e->cursor+1],&e->data.items[e->cursor], e->data.count-e->cursor);
	e->data.items[e->cursor] = '\n';
	e->cursor_y++;
	e->cursor_x = 0;
	e->cursor++;
	wed_recalculate_lines(e);
}

enum Mode { NORMAL, INSERT };
int main(int argc, char **argv) {
	if (argc <= 1) {
		perror("Error: Not enough arguments");
		exit(1);
	}

	char *file_path = argv[1];
	wed_Editor editor = {0};
	wed_load_file(&editor, file_path);
	wed_recalculate_lines(&editor);
	// init screen and sets up screen
	initscr();
	noecho();
	keypad(stdscr, TRUE);
	getmaxyx(stdscr, editor.view_y, editor.view_x);

	// print to screen
	printw("%s", editor.data.items);

	// refreshes the screen
	refresh();

	enum Mode mode = NORMAL;
	int isRunning = 1;
	while (isRunning) {
		// move cursor
		move(editor.cursor_y,editor.cursor_x);

		// pause the screen output
		char input = getch();

		if (mode == NORMAL) {
			if (input == 'q') {
				isRunning = 0;
			} else if (input == 'w') {
				wed_write_to_file(&editor, file_path);
			} else if (input == 'i') {
				mode = INSERT;
				move(editor.view_y-1, 0);
				addstr("--INSERT--");
				move(editor.cursor_y, editor.cursor_x);
			} else if (input == 'a') {
				wed_move_right(&editor);
				mode = INSERT;
				move(editor.view_y-1, 0);
				addstr("--INSERT--");
				move(editor.cursor_y, editor.cursor_x);
			} else if (input == 'h') {
				wed_move_left(&editor);
				move(editor.cursor_y, editor.cursor_x);
			} else if (input == 'j') {
				wed_move_down(&editor);
				move(editor.cursor_y, editor.cursor_x);
			} else if (input == 'k') {
				wed_move_up(&editor);
				move(editor.cursor_y, editor.cursor_x);
			} else if (input == 'l') {
				wed_move_right(&editor);
				move(editor.cursor_y, editor.cursor_x);
			} else if (input == 'r') {
				wed_recalculate_lines(&editor);
			}
		} else if (mode == INSERT) {

			if (input == (char)27) { // Escape key
				mode = NORMAL;
				refresh_screen("");
				continue;
			} else if (input == 10) { // Enter Key
				wed_new_line(&editor);
				refresh_screen("--poo--");
			} else if (input == (char)KEY_BACKSPACE) {
				wed_backspace(&editor);
				refresh_screen("--INSERT--");
			} else {
				wed_insert_char(&editor, input);
				refresh_screen("--INSERT--");
			}
		}
	}

	// deallocates memory and ends ncurses
	endwin();
	return 0;
}
