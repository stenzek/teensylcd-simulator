SDL_INCLUDE_DIR := /usr/include/SDL2

SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SRCFILES = \
		   teensylcd-run.c

PROGNAME := teensylcd-run
INCLUDE := -I$(SELF_DIR)../simavr/simavr/sim -I$(SELF_DIR)../libteensylcd -I$(SDL_INCLUDE_DIR)
LDPATH := -L$(SELF_DIR)../simavr -L$(SELF_DIR)../libteensylcd
LIBS := -lteensylcd -lsimavr -lSDL2

include ../Makefile.program

