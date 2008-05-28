#include "vt.h"

//#define DEBUG
//
void vtPrintCharacter(void *pointer, char c, int x, int y) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	SLsmg_gotorc(vt->y + y, vt->x + x);
#ifdef DEBUG
	fprintf(stderr, "Printing: %c\n", c);
#endif
	SLsmg_write_char(c);
}

void vtPrintString(void *pointer, char *c, int length, int x, int y) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	SLsmg_gotorc(vt->y + y, vt->x + x);
#ifdef DEBUG
	fprintf(stderr, "Printing: %c\n", c);
#endif
	SLsmg_write_nchars(c, length);
}

void vtInsertLine(void *pointer, int y) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	SLsmg_gotorc(vt->y + y, vt->x);
#ifdef DEBUG
	fprintf(stderr, "cbInsertLine\n");
#endif
}

void vtEraseLine(void *pointer, int y) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	int i;
	for(i = 0; i < *(vt->width); i++) {
		SLsmg_gotorc(vt->y + y, vt->x + i);
		SLsmg_write_char(' ');
	}
}

void vtScrollRegion(void *pointer, int start, int end) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "cbScrollRegion\n");
#endif
}

void vtScrollUp(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
#ifdef DEBUG
	fprintf(stderr, "cbScrollUp\n");
#endif
}

void vtAttributes(void *pointer, short bold, short blink, short inverse, short underline, short foreground, short background, short charset) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	int on, off;
	on = off = 0;
	if(bold)
		on |= A_BOLD;
	else
		off |= A_BOLD;
	if(blink)
		on |= A_BLINK;
	else
		off |= A_BLINK;
	if(inverse)
		on |= A_REVERSE;
	else
		off |= A_REVERSE;
	if(underline)
		on |= A_UNDERLINE;
	else
		off |= A_UNDERLINE;
	/*
	if(foreground)
		on |= foreground;
	else
		off |= foreground;
	if(background)
		on |= background;
	else
		off |= background;
	*/
}

void vtExit(void *pointer) {
	struct virtualTerminal *vt = (struct virtualTerminal*) pointer;
	// set status of dead, update border
	vt->fd = -1; // this will no longer be added to the select list
	//fprintf(stderr,"VT Exit\n");
}

void vtCreate(int width, int height, int x, int y, int number) {
	struct tesiObject *to;
	struct virtualTerminal *vt;

	to = newTesiObject("/bin/bash", width, height);
	to->callback_printCharacter = &vtPrintCharacter;
	//to->callback_printString = &vtPrintString;
	//to->callback_moveCursor = &vtMoveCursor;
	to->callback_insertLine = &vtInsertLine;
	to->callback_eraseLine = &vtEraseLine;
	to->callback_scrollRegion = &vtScrollRegion;
	to->callback_scrollUp = &vtScrollUp;
	to->callback_attributes = &vtAttributes;

	vt = malloc(sizeof(struct virtualTerminal));
	if(vt == NULL) {
		fprintf(stderr, "Error allocating memory for virtual terminal\n");
		return;
	}
	vt->id = number;
	vt->state = VT_RUNNING;
	vt->border = 1;
	vt->padding = 0;

	vt->window = NULL;
	vt->wBorder = NULL;

	if(vt->border) {
#ifdef DEBUG
		fprintf(stderr, "Creating nCurses border window (rows,cols,y,x) %d %d %d %d\n", height, width, y, x);
#endif
		vt->wBorder = newwin(height, width, y, x);
		if(vt->wBorder == NULL) {
			fprintf(stderr, "Error creating nCurses border window\n");

		} else {
			box(vt->wBorder, ACS_VLINE, ACS_HLINE);
			mvwprintw(vt->wBorder, 0, 0, "%i ", vt->id);
			width -= 2;
			height -= 2;
			/*
			y += 1;
			x += 1;
			*/
			wnoutrefresh(vt->wBorder);
		}
	}

#ifdef DEBUG
	fprintf(stderr, "Creating nCurses window (rows,cols,y,x) %d %d %d %d\n", height, width, y, x);
#endif

	if(vt->border)
		vt->window = derwin(vt->wBorder, height, width, 1, 1);
	else
		vt->window = newwin(height, width, y, x);

	if(vt->window == NULL) {
		fprintf(stderr, "Error creating nCurses window\n");
		free(vt);
		return;
	}

	to->pointer = vt;
	tesiObjects[number] = to;
}

struct tesiObject *vtGet(int index) {
	return tesiObjects[index];
}

void vtMove(int index, int x, int y, int width, int height) {
	struct tesiObject *to = tesiObjects[index];
	struct virtualTerminal *vt;

	if(to) {
		vt = (struct virtualTerminal*) to->pointer;
		vt->x = x;
		vt->y = y;
		to->width = width;
		to->height = height;
		// send resize signal
	}
}
void sendToVT(int index, char *input) {
	struct tesiObject *to = tesiObjects[index];
	// set input to vt
	write(to->fd_input, input, strlen(input));
}


/*
 * Takes a pointer to a virtualTerminal structure
 * Frees all elements, and then the structure itself
 * */
void vtDestroy(int index) {
	struct tesiObject *to;
	struct virtualTerminal *vt;

	to = tesiObjects[index];
	// send kill signals to child processes
	vt = to->pointer;
	// not sure if the library code makes sure not to double-free
	if(vt)
		free(vt); // free our encapsulation structure
	deleteTesiObject(to);
	tesiObjects[index] = NULL;
}
