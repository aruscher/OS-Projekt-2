# ######################
# Makefile for KAC-Shell
# ######################

SOURCES=shell.c parser.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=kac_shell

LDFLAGS=-Wall -lreadline -O2
# all targets
all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@echo "building project"
	gcc $(LDFLAGS) $(OBJECTS) -o $@

clean:
	@echo "cleaning project"
	@rm -rf $(OBJECTS) $(EXECUTABLE)