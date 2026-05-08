# Makefile for IFJ25 Project
# Makefile for compiling the IFJ25 source files into an executable.
# Authors: 
# Vojtěch Kadlev xkadlev00 FIT VUT v Brně
# Aleš Obr xobral00 FIT VUT v Brně
# Date: 2025
CC=gcc
CFLAGS= -g -std=c11 -pedantic -Wall -Wextra
TARGET=translator
HEADERS := lexikal_analyzator.h syntaktic_analyzator.h semantic_functions.h machine_code_generator.h symtable.h token.h prec_analytics.h syntaktik_tree.h error.h v_stack.h
OBJECTS := main.o lexikal_analyzator.o syntaktic_analyzator.o semantic_functions.o machine_code_generator.o symtable.o token.o prec_analytics.o syntaktik_tree.o error.o v_stack.o

default:$(TARGET)

$(TARGET):$(OBJECTS)
	$(CC) $(OBJECTS) -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

run: default

ZIPNAME=xobrale00.zip

.PHONY: zip run

zip:
	zip -j $(ZIPNAME) *.c *.h Makefile ../docs/dokumentace.pdf rozdeleni 2>/dev/null || zip -j $(ZIPNAME) *.c *.h Makefile ../docs/*.pdf

clean:
	rm -f $(OBJECTS)
	rm -f $(TARGET)