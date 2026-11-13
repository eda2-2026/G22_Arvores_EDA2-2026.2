# Sistema de Reservas com Deteccao de Conflito — EDA2 2026.2
# Funciona no Linux/macOS (make) e no Windows com MinGW (mingw32-make).
#
#   make              compila reservas, gerador e benchmark
#   make test         compila e roda todos os testes
#   make run          abre a CLI com data/exemplo.csv
#   make demo         roda o roteiro data/demo.txt na CLI (AVL e Rubro-Negra)
#   make bench        roda o benchmark (resultados.csv) e gera os graficos em resultados/
#   make clean        apaga binarios e objetos

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -std=c11 -O2
CFLAGS  += -Isrc
LDLIBS  += -lm

ifeq ($(OS),Windows_NT)
    EXE    := .exe
    RM     := del /Q /F
    FIXP    = $(subst /,\,$1)
    PYTHON ?= python
    RUN    := $(subst /,\,./)
else
    EXE    :=
    RM     := rm -f
    FIXP    = $1
    PYTHON ?= python3
    RUN    := ./
endif

ifeq ($(CC),cc)
    CC := gcc
endif

AGENDA  := src/agenda.c src/agenda_avl.c src/agenda_rb.c
APP     := src/reserva.c src/sistema.c src/csv.c src/gerador.c
OBJ_AG  := $(AGENDA:.c=.o)
OBJ_APP := $(APP:.c=.o)

BINS  := reservas$(EXE) gerador$(EXE) benchmark$(EXE)
TESTS := test_avl$(EXE) test_rb$(EXE) test_agenda$(EXE) test_reserva$(EXE) test_csv$(EXE)

.PHONY: all test run demo bench graficos clean

all: $(BINS)

reservas$(EXE): src/main.o $(OBJ_APP) $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

gerador$(EXE): src/gerador_main.o $(OBJ_APP) $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

benchmark$(EXE): tools/benchmark.c $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_avl$(EXE): tests/test_avl.c $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_rb$(EXE): tests/test_rb.c $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_agenda$(EXE): tests/test_agenda.c $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_reserva$(EXE): tests/test_reserva.c src/reserva.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

test_csv$(EXE): tests/test_csv.c $(OBJ_APP) $(OBJ_AG)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# dependencias de cabecalhos (simples e explicitas)
$(OBJ_AG): src/agenda.h src/agenda_impl.h src/reserva.h
src/reserva.o: src/reserva.h
src/sistema.o: src/sistema.h src/agenda.h src/reserva.h
src/csv.o: src/csv.h src/sistema.h src/agenda.h src/reserva.h
src/gerador.o: src/gerador.h src/reserva.h
src/main.o: src/csv.h src/sistema.h src/agenda.h src/reserva.h
src/gerador_main.o: src/csv.h src/gerador.h src/reserva.h

test: $(TESTS)
	$(RUN)test_reserva$(EXE)
	$(RUN)test_avl$(EXE)
	$(RUN)test_rb$(EXE)
	$(RUN)test_agenda$(EXE)
	$(RUN)test_csv$(EXE)

run: reservas$(EXE)
	$(RUN)reservas$(EXE) data/exemplo.csv

demo: reservas$(EXE)
	$(RUN)reservas$(EXE) -e -a avl < data/demo.txt
	$(RUN)reservas$(EXE) -e -a rb  < data/demo.txt

bench: benchmark$(EXE)
	$(RUN)benchmark$(EXE)
	$(PYTHON) scripts/graficos.py

graficos:
	$(PYTHON) scripts/graficos.py

clean:
	-$(RM) $(call FIXP,src/*.o) $(BINS) $(TESTS)