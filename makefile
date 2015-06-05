SHELL=/bin/bash

TARGET = TestEGLImage.ex

#lista directory in cui cercare sorgenti 
VPATH =	. \
	../common

#lista directory in cui cercare .h
INCLUDE =  -I../common -I../boost

#elenco dei sorgenti
SOURCES =  TestEGLImage.cpp


#elenco lib
LIBS = \
       -lc \
       -lpthread \
       -lstdc++ \
       -ldl \
       -L /usr/lib/arm-linux-gnueabihf/mali-egl/ -lmali \
       -lX11

#dir base di output
BASEOUTDIR = linuxout/

#dipendenza da altre librerie: per ognuna delle librerie aggiungere un comando al target lib
LIBRARYDEPENDENCY = 

include ../target-linux.mk

lib:
	@echo
