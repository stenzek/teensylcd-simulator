SELF_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SRCFILES = \
		   pcd8544.c \
		   teensylcd.c \
		   timer.c

LIBNAME := libteensylcd
INCLUDE := -I$(SELF_DIR)../simavr/simavr/sim

include ../Makefile.library

