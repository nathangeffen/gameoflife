CC              = gcc
EXE		= gameoflife
GUI		?= main_gui.c
SOURCES		= gameoflife.c
SOURCES 	+= $(GUI)
OBJECTS		= $(SOURCES:.c=.o)
CFLAGS          = -g -c -Wall -pedantic
GUILIB 		= `pkg-config --cflags --libs gtk+-3.0`
LDFLAGS         = -g `pkg-config --cflags --libs gtk+-3.0`

all: $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

gameoflife.o: gameoflife.c gameoflife.h
	$(CC) $(CFLAGS) $< -o $@

main_gui.o: main_gui.c gameoflife.h
	$(CC) $(CFLAGS) $< -o $@ $(GUILIB)

main.o: main.c gameoflife.h
	$(CC) $(CFLAGS) $< -o $@

release:
	$(CC) -O3 $(SOURCES) -o $(EXE) $(LDFLAGS)

release_no_gui:
	$(CC) -O3 main.c gameoflife.c -o gameoflife -lpthread

clean:
	rm -rf $(OBJECTS) $(EXE)

check-syntax:
	gcc -o /dev/null -S ${CHK_SOURCES}
