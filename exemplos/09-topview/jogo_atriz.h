#ifndef JOGO_ATRIZ_H
#define JOGO_ATRIZ_H

#include <c2d2/ator.h>

#define VATRIZ	2


// Enumera��o dos estados da atriz do jogo
enum {ATRIZ_INVULNERAVEL=ATOR_PROG_INI, ATRIZ_PARADA, ATRIZ_ANDANDO, 
		ATRIZ_ATIRANDO, ATRIZ_VENCEU, ATRIZ_MORRENDO};

// A fun��o que carrega a atriz (pra facilitar). DEscarregar n�o precisa assim por usar s�
// o identificador, que o jogo tem que conhecer de qualquer maneira
//
// 0,5 ponto na nota pra quem explicar em detalhes a vantagem de ter essa fun��o AQUI
//
bool JOGO_CarregaAtriz();
// A fun��o para fazer a l�gica da atriz
bool AtualizaAtriz(Ator *a, unsigned int mapa);


#endif
