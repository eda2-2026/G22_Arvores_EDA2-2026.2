#ifndef CSV_H
#define CSV_H

#include <stdio.h>
#include "sistema.h"

/* Formato do CSV de reservas (uma reserva por linha, cabecalho obrigatorio):

     id,recurso,inicio,fim,responsavel
     1,Sala1,2026-11-16 08:00,2026-11-16 09:30,Anna Clara

   - Separador detectado pelo cabecalho: virgula, ponto e virgula ou TAB.
   - Colunas localizadas pelo NOME no cabecalho (qualquer ordem, sem
     diferenciar maiusculas). Obrigatorias: recurso, inicio, fim.
     Opcionais: id (vazio/ausente -> id automatico) e responsavel.
   - Datas no formato "AAAA-MM-DD HH:MM" (tambem aceita "AAAA-MM-DDTHH:MM").
   - Campos podem vir entre aspas ("Silva, Ana"); aspas internas como "".
   - Linhas em branco e linhas iniciadas por # sao ignoradas. */

typedef struct {
    int lidas;       /* linhas de dados (fora cabecalho, vazias e comentarios) */
    int aceitas;
    int rejeitadas;
    int conflitos;   /* parte das rejeitadas: conflito de horario */
} ResumoCsv;

/* Carrega as reservas do arquivo para o sistema. Cada linha rejeitada e
   relatada em 'relatorio' (se != NULL) com o numero da linha e o motivo; a
   carga continua nas linhas seguintes.
   Retorna 1 se o arquivo foi lido (mesmo com linhas rejeitadas) e 0 se nao
   abriu ou se o cabecalho e invalido. */
int csv_carregar(Sistema *s, const char *caminho, FILE *relatorio, ResumoCsv *resumo);

/* Grava todas as reservas do sistema (recurso por recurso, em ordem de
   inicio). Retorna o numero de reservas gravadas ou -1 em erro. */
int csv_salvar(const Sistema *s, const char *caminho);

/* Escreve o cabecalho / uma reserva no formato acima (usado tambem pelo
   gerador de reservas). */
void csv_escrever_cabecalho(FILE *f);
void csv_escrever_reserva(FILE *f, const Reserva *r);

/* Divide 'linha' (modificada no lugar) em campos pelo separador, tratando
   aspas. Retorna o numero de campos, ou -1 se ha aspas sem fechar. */
int csv_dividir(char *linha, char sep, char **campos, int max_campos);

#endif /* CSV_H */