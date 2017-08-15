/*
Copyright 2008-2010, Paulo Vinicius Wolski Radtke (pvwradtke@gmail.com)

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/******************************

ModificaÁıes:

	12/08/2010 - Inclus„o da funÁ„o para definir o callback de sincronizaÁ„o do usu·rio.
		Inclui a definiÁ„o do ponteiro de funÁ„o e da funÁ„o que o preenche. O callback
		deve ser invocado na funÁ„o especÌfica de sincronizaÁ„o do renderer especÌfico.
		Respons·vel: Paulo V. W. Radtke

*******************************/

#include <c2d2/chien2d2.h>
#include <c2d2/chien2d2_sdl.h>
#include <c2d2/chien2d2_gl.h>
#include <c2d2/chien2d2_interno.h>

#include <assert.h>

// Ponteiros de funÁıes
// FunÁ„o que encerra a Chien2D 2
void (*C2D2_Encerra)()=0;
// FunÁ„o para sincronizar o vÌdeo a 60ppfps
void (*C2D2_Sincroniza)(Uint8 fps)=0;
// FunÁ„o para limpar a tela
void (*C2D2_LimpaTela)()=0;
// FunÁ„o para escolher a cor de limpeza da tela
void (*C2D2_TrocaCorLimpezaTela)(Uint8 r, Uint8 g, Uint8 b)=0;
// FunÁ„o para limitar a ·rea de desenho da tela
void (*C2D2_LimitaAreaDesenho)(int x, int y, int largura, int altura)=0;
// FunÁ„o para eliminar o limite da ·rea de desenho
void (*C2D2_LiberaAreaDesenho)()=0;

// FunÁ„o para recuperar o tempo do sistema
//
// Data: 30/07/2008
//
unsigned long C2D2_TempoSistema()
{
	return SDL_GetTicks();
}

// FunÁıes para manipular sprites

// funÁ„o para carregar um sprite set na memÛria
unsigned int (*C2D2_CarregaSpriteSet)(const char *arquivo, int largura, int altura)=0;
// Remove um sprite set da memÛria
void (*C2D2_RemoveSpriteSet)(unsigned int id)=0;
// FunÁ„o para desenhar um sprite
bool (*C2D2_DesenhaSprite)(unsigned int identificador, unsigned int indice, int x, int y)=0;
// FunÁ„o para desenhar um sprite distorcido e com efeitos de cor/alpha
bool (*C2D2_DesenhaSpriteEfeito)(unsigned int identificador, unsigned int indice, int x[4], int y[4], Uint8 r, Uint8 g, Uint8 b, Uint8 a);
//Desenha um sprite centralizado nas coordenadas xcentro e ycentro, com dimensoes largura e altura
bool (*C2D2_DesenhaSpriteCentro)(unsigned int id, unsigned int indice, double xcentro, double ycentro, int largura, int altura);
bool (*C2D2_DesenhaSpriteCentroAlfa)(unsigned int id, unsigned int indice, double xcentro, double ycentro, int largura, int altura, Uint8 r, Uint8 g, Uint8 b, Uint8 a);


// ¡rea de vari·veis globais da Chien2D v2.0

// indica se a Chien2D 2 j· foi inicializada. Por default, n„o foi ainda.
bool inicializado;
// A tela principal
SDL_Window *screen;
// Contexto OpenGL
SDL_GLContext mainContext;
// Renderer
SDL_Renderer *renderer;
// O vetor com os sprite sets
C2D2_SpriteSet sprites[C2D2_MAX_SPRITESET];
// Finalmente, o vetor com as fontes
C2D2_Fonte fontes[C2D2_MAX_FONTES];
// Indica se o sistema est· em shutdown ou n„o
bool c_shutdown = false;
// O vetor de teclas utilizadas
C2D2_Botao teclas[C2D2_MAXTECLAS];
// O mouse do sistema
C2D2_Mouse mouse;

static C2D2_Joystick joysticks[C2D2_MAX_JOYSTICKS] = {0};

// Indica o tipo de render utilizado pela lib
int render;
// FunÁ„o de sincronizaÁ„o do jogo do usu·rio
void (*C2D2_SincronizaUsuario)()=0;

// FunÁ„o que inicia a Chien2D 2 de acordo com o tipo do render.
//
// Data: 10/06/2008
//
bool C2D2_Inicia(unsigned int largura, unsigned int altura, int modoVideo, int tipoRender, char *tituloJanela)
{
	switch(tipoRender)
	{
	case C2D2_DESENHO_OPENGL:
		if(C2D2GL_Inicia(largura, altura, modoVideo, tituloJanela))
		{
			render=C2D2_DESENHO_OPENGL;
			return true;
		}
		break;
	case C2D2_DESENHO_PADRAO:
	default:
		if(C2D2SDL_Inicia(largura, altura, modoVideo, tituloJanela))
		{
			render = C2D2_DESENHO_PADRAO;
			return true;
		}
		break;
	}
	return false;
}

// funÁ„o que indica as dimensıes da tela
//
// Data: 26/01/2011
//
bool C2D2_DimensoesTela(int *largura, int *altura)
{
	if(inicializado)
	{
		SDL_GetWindowSize(screen, largura, altura);
		//REMOVER *largura = screen->w;
		//REMOVER *altura = screen->h;
		return true;
	}
	else
		return false;
}


// Procura um spriteset pelo nome do arquivo original
//
// Data: 13/04/2007
//
unsigned int C2D2_ProcuraSpriteSet(const char *apelido)
{
    // Õndice de spriteset inv·lido, caso n„o encontre
    unsigned int idx=0;
    for(int i=0;i<C2D2_MAX_SPRITESET;i++)
        if(strcmp(sprites[i].apelido, apelido)==0)
        {
            // O Ìndice È a posiÁ„o atual + 1
            idx=i+1;
            // Encerra a busca
            break;
        }
    return idx;
}

// FunÁ„o para verificar a colis„o entre um sprite e um quadrado de referÍncia
//
// Data: 22/06/2010
//
bool C2D2_ColidiuQuadrados(int x1b, int y1b, int l1b, int a1b,
					       int x2b, int y2b, int l2b, int a2b)
{
	// Calcula o offset x e y do segundo sprite em relaÁ„o ao primeiro
	int offx, offy;
	offx=x2b-x1b;
	offy=y2b-y1b;
	// Testa se o bounding box dos sprites se sobrepıe
	// verifica quem est· ‡ esquerda
	if(offx>=0)
	{
		// Quando o offset È positivo, o sprite 2 est· mais ‡ direita
		// O offset n„o pode ser maior que a largura do quadrado do sprite 1
		if(offx > l1b)
			return 0;
	}
	else
	{
		// Sen„o È o contr·rio
		if(-offx > l2b)
			return 0;
	}
	// Verifica quem est· acima
	if(offy>=0)
	{
		// Quando o offset È positivo, o sprite 1 est· acima
		// O offset n„o pode ser maior que a altura do quadrado do sprite 1
		if(offy>a1b)
			return 0;
	}
	else
	{
		// Sen„o È o contr·rio
		if(-offy > a2b)
			return 0;
	}

	return 1;
}

// FunÁ„o para verificar a colis„o entre sprites usando bounding box
//
// Data: 22/06/2010
//
bool C2D2_ColidiuSprites(unsigned int id1, unsigned int indice1, int x1, int y1, int x1b, int y1b, int l1b, int a1b,
					     unsigned int id2, unsigned int indice2, int x2, int y2, int x2b, int y2b, int l2b, int a2b)
{
	x1 += x1b;
	y1 += y1b;

	x2 += x2b;
	y2 += y2b;

	// Calcula o offset x e y do segundo sprite em relaÁ„o ao primeiro
	int offx, offy;
	offx=x2-x1;
	offy=y2-y1;
	// verifica se os Ìndices dos sprites s„o v·lidos]
	if(indice1 >= (unsigned int)(sprites[id1-1].matrizX*sprites[id1-1].matrizY) || indice2 >= (unsigned int)(sprites[id2-1].matrizX*sprites[id2-1].matrizY))
		return 0;
	// Testa se o bounding box dos sprites se sobrepıe
	// verifica quem est· ‡ esquerda
	if(offx>=0)
	{
		// Quando o offset È positivo, o sprite 2 est· mais ‡ direita
		// O offset n„o pode ser maior que a largura do quadrado do sprite 1
		if(offx > l1b)
			return 0;
	}
	else
	{
		// Sen„o È o contr·rio
		if(-offx > l2b)
			return 0;
	}
	// Verifica quem est· acima
	if(offy>=0)
	{
		// Quando o offset È positivo, o sprite 1 est· acima
		// O offset n„o pode ser maior que a altura do quadrado do sprite 1
		if(offy>a1b)
			return 0;
	}
	else
	{
		// Sen„o È o contr·rio
		if(-offy > a2b)
			return 0;
	}

	return C2D2_ColidiuSprites(id1, indice1, x1, y1, id2, indice2, x2, y2);
}

// FunÁ„o para verificar a colis„o entre sprites
//
// Data: 03/05/2007
//
bool C2D2_ColidiuSprites(unsigned int id1, unsigned int indice1, int x1, int y1, 
					unsigned int id2, unsigned int indice2, int x2, int y2)
{
	// Verifica se os sprites s„o v·lidos
	if(id1==0 || id2==0)
	{
		printf("Tentou colidir com um sprite invalido (0)\n");
		return false;
	}
	// Calcula o offset x e y do segundo sprite em relaÁ„o ao primeiro
	int offx, offy;
	offx=x2-x1;
	offy=y2-y1;
	// verifica se os Ìndices dos sprites s„o v·lidos]
	if(indice1 >= (unsigned int)(sprites[id1-1].matrizX*sprites[id1-1].matrizY) || indice2 >= (unsigned int)(sprites[id2-1].matrizX*sprites[id2-1].matrizY))
		return 0;
	// Testa se o bounding box dos sprites se sobrepıe
	// verifica quem est· ‡ esquerda
	if(offx>=0)
	{
		// Quando o offset È positivo, o sprite 2 est· mais ‡ direita
		// O offset n„o pode ser maior que a largura do sprite 1
		if(offx > sprites[id1-1].spLargura)
			return 0;
	}
	else
	{
		// Sen„o È o contr·rio
		if(-offx > sprites[id2-1].spLargura)

			return 0;
	}
	// Verifica quem est· acima
	if(offy>=0)
	{
		// Quando o offset È positivo, o sprite 1 est· acima
		// O offset n„o pode ser maior que a altura do sprite 1
		if(offy>sprites[id1-1].spAltura)
			return 0;
	}
	else
	{
		// Sen„o È o contr·rio
		if(-offy > sprites[id2-1].spAltura)
			return 0;
	}

	// Se chegou aqui, retorna o resultado da colis„o ponto-a-ponto
	return C2D2_SobrepoeBitMasks(sprites[id1-1].bmask[indice1], 
		sprites[id2-1].bmask[indice2], offx, offy) ? true : false;

}


// FunÁ„o para retornar as dimensıes de um sprites
//
//   Data: 22/01/2011
//
bool C2D2_DimensoesSprite(unsigned int idx, int *largura, int *altura)
{
	// Verifica se os sprites s„o v·lidos
	if(idx == 0)
	{
		printf("Sprite invalido - 0.\n");
		return false;
	}
	// Ajusta o indice
	idx--;
	// Verifica se o sprite existe mesmo
	switch(render)
	{
	    case C2D2_DESENHO_PADRAO:
	        if(sprites[idx].imagem == 0)
	            return false;
	        break;
	    case C2D2_DESENHO_OPENGL:
        	if( sprites[idx].textura==0)
        		return false;
	        break;
	    default:
	        return false;
	}
	*largura = sprites[idx].spLargura;
	*altura = sprites[idx].spAltura;
	return true;
}


////////////////////////////////
// FunÁıes para manipular Fontes
////////////////////////////////

// funÁ„o para carregar uma fonte na memÛria
//
// Data: 24/04/2007
//
unsigned int C2D2_CarregaFonte(const char *arquivo, int dimensao)
{
    // Verifica se a fonte existe
    unsigned int idx = C2D2_ProcuraFonte(arquivo);
    // Se j· existe uma fonte com o apelido, retorna o Ìndice associado
    if(idx != 0)
    {
        fontes[idx-1].usuarios = fontes[idx-1].usuarios+1;
        return idx;
    }
        
    // Se n„o existe, procura o primeiro Ìndice vago (idx ainda È igual a 0)
    for(int i=0;i<C2D2_MAX_FONTES;i++)
        // A fonte È vaga (n„o tem imagem associada?
		if(fontes[i].idSpriteSet == 0)
        {
            idx=i+1;
            break;
        }
    
    // ATEN«√O: n„o ter espaÁo na memÛria n„o È relacionado a este teste. Apenas
    // testa-se aqui se existe um Ìndice vago na lista. Se n„o existe, idx==0
    if(idx==0)

        // Retorna um Ìndice inv·lido para indicar que o mÈtodo falhou
        return 0;
    
    // Se chegou atÈ aqui, idx contÈm o identificador correto 

    // posiÁ„o no array (sem incrementar 1)
    idx=idx-1;    

    // Indica o apelido
	strcpy(fontes[idx].apelido, arquivo);

	// Guarda o tamanho, em pontos, da fonte
	fontes[idx].tamanhoFonte = dimensao;
	// Carrega o spriteset
	fontes[idx].idSpriteSet = C2D2_CarregaSpriteSet(arquivo, dimensao, dimensao);
    // Conseguiu carregar o spriteset? Se for inv·lido, retorna 0
    if(fontes[idx].idSpriteSet == 0)
    {
		printf("Nao conseguir abrir a fonte do arquivo %s.\n", arquivo);
        fontes[idx].apelido[0]='\0';
        return 0;
    }
	 // Testa se o tamanho da imagem est· certo para a fonte
	 // Se inv·lido, retorna 0
	 if(sprites[fontes[idx].idSpriteSet-1].imagem->w != dimensao*16 ||
	  sprites[fontes[idx].idSpriteSet-1].imagem->h != dimensao*16)
	 {
		 printf("As dimensoes da fonte sao invalidas. utilize um arquivo de %ix%i pixels, com as fontes numa matriz 16x16.\n", dimensao*16, dimensao*16);
		 fontes[idx].apelido[0]='\0';
		 C2D2_RemoveSpriteSet(fontes[idx].idSpriteSet);
		 return 0;
	 }    

    // Preenche os deslocamentos
	SDL_LockSurface(sprites[fontes[idx].idSpriteSet-1].imagem);
    for(int linha=0;linha<16;linha++)
        for(int coluna=0;coluna<16;coluna++)
        {           
            // Procura a primeira coluna com pixels n„o transparentes
            
            // A primeira coluna com pixel n„o transparente
            int primeira=-1;
            // Para todas as colunas e linhas da fonte...
            for(int x=0;x<fontes[idx].tamanhoFonte;x++)
            {
                for(int y=0;y<fontes[idx].tamanhoFonte;y++) {

                    if(!PontoTransparente(sprites[fontes[idx].idSpriteSet-1].imagem, x+coluna*fontes[idx].tamanhoFonte, y+linha*fontes[idx].tamanhoFonte))
                    {
                        primeira = x;
                        break;
                    }
                    // Deve parar o laÁo do x?
                    if(primeira != -1)
                        break;
                }
            }
			SDL_UnlockSurface(sprites[fontes[idx].idSpriteSet-1].imagem);
            // Achou uma coluna n„o transparente?
            if(primeira >= 0)
				    
                    fontes[idx].deslocamentos[linha*16+coluna] = primeira;
            else
            {
                // A letra È toda transparente, pega o m·ximo de largura e deslocamento 0
                fontes[idx].larguras[linha*16+coluna] = fontes[idx].tamanhoFonte;
                fontes[idx].deslocamentos[linha*16+coluna] = 0;
                // Pula para a prÛxima letra
                continue;
            }
            
            // Agora procura a primeira coluna com todos os pontos transparntes
            // (executa sÛ se h· uma coluna transparente, vide else anterior)

            // Para todas as colunas e linhas da fonte...
            int ultima=-1;
            // Indica que todos os pontos na coluna s„o transparentes
            bool todosBrancos;
			SDL_LockSurface(sprites[fontes[idx].idSpriteSet-1].imagem);
            for(int x=primeira+1;x<fontes[idx].tamanhoFonte;x++)
            {
                // Todos os pontos s„o transparentes atÈ prova contr·ria
                todosBrancos = true;
                for(int y=0;y<fontes[idx].tamanhoFonte;y++) {

					if(!PontoTransparente(sprites[fontes[idx].idSpriteSet-1].imagem,
						x+coluna*fontes[idx].tamanhoFonte, 
						y+linha*fontes[idx].tamanhoFonte))
					{
						todosBrancos = false;
						break;
                    }
                }
                
                // Deve parar o laÁo do x?
                if(todosBrancos)
                {
                    ultima=x;
                    break;
                }
            }
			SDL_UnlockSurface(sprites[fontes[idx].idSpriteSet-1].imagem);
            // Achou uma coluna transparente?
            if(ultima >= 0)
            {
                // Adiciona um pouco ‡ largura, de acordo com o tamanho da fonte
                if(fontes[idx].tamanhoFonte<16)
					ultima+=1;
				else if(fontes[idx].tamanhoFonte<32)
                    ultima+=1;
				else if(fontes[idx].tamanhoFonte<64)
					ultima+=2;
				else
					ultima+=3;
                // Testa se n„o saiu fora da fonte
                if(ultima <= fontes[idx].tamanhoFonte)
                    fontes[idx].larguras[linha*16+coluna] = ultima - primeira;
                else
                    // Se saiu fora da fonte, volta pra dentro. Pode ser que n„o sobre espaÁo
                    fontes[idx].larguras[linha*16+coluna] = fontes[idx].tamanhoFonte - primeira;
            }
            else
                fontes[idx].larguras[linha*16+coluna] = fontes[idx].tamanhoFonte - primeira;          
        }
    // Deixa o espaÁo como metade do tamanho da fonte
    fontes[idx].larguras[' '] = fontes[idx].tamanhoFonte / 2;
    fontes[idx].deslocamentos[' '] = 0;    
    // Indica que tem um usu·rio
    fontes[idx].usuarios = 1;
    return idx+1;
    
}


// Remove uma fonte da memÛria
//
// Data: 24/04/2007
//
void C2D2_RemoveFonte(unsigned int id)
{
    // Est· na faixa esperada para remover?
    if(id > C2D2_MAX_FONTES || id==0)
        return;
    // SÛ remove se existe apenas um usu·rio da fonte ou se estiver em shutdown
    if(fontes[id-1].usuarios > 1 && !c_shutdown)
    {
        fontes[id-1].usuarios = fontes[id-1].usuarios-1;
        return;
    }
        
    if(fontes[id-1].idSpriteSet != 0)
    {
        C2D2_RemoveSpriteSet(fontes[id-1].idSpriteSet);
        fontes[id-1].idSpriteSet = 0;
        strcpy(fontes[id-1].apelido, "");
    }
    fontes[id-1].usuarios = 0;
}


// Procura uma fonte pelo nome do arquivo original
//
// Data: 24/04/2007
//
unsigned int C2D2_ProcuraFonte(const char *apelido)
{
    for(int i=0;i<C2D2_MAX_FONTES;i++)
		if(strcmp(fontes[i].apelido, apelido) == 0)
            // O Ìndice È a posiÁ„o atual + 1
            return i+1;
    return 0;
}

// FunÁ„o para desenhar um texto
//
// Data: 24/04/2007
//
bool C2D2_DesenhaTexto(unsigned int identificador, int x, int y, char const *texto, unsigned int alinhamento)
{
    // A fonte existe?

    if(identificador > C2D2_MAX_FONTES || identificador ==0)
        return false;
    // A largura real do texto
    int larguraReal, alturaReal;

    if(!C2D2_DimensoesTexto(identificador, texto, &larguraReal, &alturaReal))
    {
        printf("Falhou ao procurar as dimensoes do texto %s com a fonte %s.\n", 
			texto, fontes[identificador-1].apelido);
        return false;
    }

    // Pega a fonte para acesso local
    C2D2_Fonte *fonte = &(fontes[identificador-1]);
    
    // Desenha de acordo com o alinhamento
    switch(alinhamento)
    {
        case C2D2_TEXTO_CENTRALIZADO:
        {
            // ComeÁa a desenhar a partir da coordenada x-deslocamento da primeira imagem
            int posicao = x - larguraReal/2;
			int letras = (int)strlen(texto);
            for(int i=0;i<letras;i++)
            {   
                C2D2_DesenhaSprite(fonte->idSpriteSet, (unsigned char)texto[i], posicao-fonte->deslocamentos[(unsigned char)(texto[i])], y);
                // Atualiza para a prÛxima letra
                posicao+=fonte->larguras[(unsigned char)(texto[i])];
            }
            break;
        }

        case C2D2_TEXTO_DIREITA:
        {
            // ComeÁa a desenhar a partir da coordenada x-deslocamento da primeira imagem
            int posicao = x - larguraReal;
			int letras = (int)strlen(texto);
            for(int i=0;i<letras;i++)
            {   
				C2D2_DesenhaSprite(fonte->idSpriteSet, (unsigned char)texto[i], posicao-fonte->deslocamentos[(unsigned char)(texto[i])], y);
                // Atualiza para a prÛxima letra
                posicao+=fonte->larguras[(unsigned char)(texto[i])];
            }
            break;
        }

        case C2D2_TEXTO_ESQUERDA:
        default:
        {
            // ComeÁa a desenhar a partir da coordenada x-deslocamento da primeira imagem
            int posicao = x;
			int letras = (int)strlen(texto);
            for(int i=0;i<letras;i++)
            {   
                C2D2_DesenhaSprite(fonte->idSpriteSet, (unsigned char)texto[i], posicao-fonte->deslocamentos[(unsigned char)(texto[i])], y);
                // Atualiza para a prÛxima letra
                posicao+=fonte->larguras[(unsigned char)(texto[i])];
            }
            break;
        }
    }    
    return true;
}

// FunÁ„o para calcular as dimensıes de um texto        
//
// Data: 25/04/2007
//
bool C2D2_DimensoesTexto(unsigned int idFonte, const char *texto, int *largura, int *altura)
{
    // A fonte existe?
    if(idFonte > C2D2_MAX_FONTES || idFonte == 0)
        return false;
    C2D2_Fonte *fonte = &(fontes[idFonte-1]);
	if(fonte->idSpriteSet != 0)
    {
        //  Pega a altura
		if(altura != 0)
			*altura=fonte->tamanhoFonte;
        // Largura local
        int larg=0;
        // Calcula o tamanho
		int letras = (int)strlen(texto);
        for(int i =0;i<letras;i++)
            larg+=fonte->larguras[(unsigned char)(texto[i])];
        // Passa o valor
		if(largura != 0)
			*largura=larg;
        return true;
    }
    return false;
}

// funÁ„o para retornar a dimens„o de uma fonte
//
// Data: 26/01/2011
//
bool C2D2_DimensaoFonte(unsigned int idx, int *altura)
{
	if(idx > C2D2_MAX_FONTES || idx == 0)
		return false;
	C2D2_Fonte *fonte = &(fontes[idx-1]);
	if(fonte->idSpriteSet != 0)
	{
		//  Pega a altura
		if(altura != 0)
		*altura=fonte->tamanhoFonte;
		return true;
	}
	return false;
}

// FunÁıes inline auxiliares para pressionar um bot„o
//
// Data: 04/05/2007
//
inline void C2D2_PressionaBotao(C2D2_Botao *botao)
{
	botao->pressionado=true;
	botao->liberado=false;
	botao->ativo=true;
	
}

inline void C2D2_LiberaBotao(C2D2_Botao *botao)
{
	botao->ativo=false;
	botao->liberado=true;
	botao->pressionado=false;
}

inline void C2D2_AtualizaDirecional(C2D2_Botao *botao, int sdlHatValue, int sdlHatFlag)
{
	if(sdlHatValue & sdlHatFlag)
	{
		if(!botao->pressionado)
		{
			C2D2_PressionaBotao(botao);
		}
	}
	else if(botao->pressionado)
	{						
		C2D2_LiberaBotao(botao);
	}
}


// FunÁ„o auxiliar para processar uma tecla
//
// Data: 09/02/2007
//

inline void C2D2_ProcessaTecla(C2D2_Botao *tecla, Uint32 evento)
{
    switch(evento)
    {
        case SDL_KEYDOWN:
            tecla->pressionado=true;
            tecla->liberado=false;
            tecla->ativo=true;
            break;
        case SDL_KEYUP:
            tecla->ativo=false;
            tecla->liberado=true;
            tecla->pressionado=false;
            break;
    }
}

// Poling das entradas. TRata todos os eventos relacionados a dispositivos de
// entrada do sistema
//
// Data: 04/05/2007
//
void C2D2_Entrada()
{
	// Remove o pressionamento das teclas do passo anterior
	for(int i=0;i<C2D2_MAXTECLAS;i++)
		teclas[i].pressionado = false;
	// Remove o liberamento das teclas do passo anterior
	for(int i=0;i<C2D2_MAXTECLAS;i++)
		teclas[i].liberado=false;
	// Remove o pressionamento do mouse do passo anterior
	for(int i=0;i<C2D2_MMAX;i++)
		mouse.botoes[i].pressionado = false;
	// Remove o liberamento do mpouse do passo anterior
	for(int i=0;i<C2D2_MMAX;i++)
		mouse.botoes[i].liberado=false;

	for(int i = 0;i < C2D2_MAX_JOYSTICKS; ++i)
	{
		for(int j = 0;j < C2D2_MAX_JBOTOES; ++j)
		{
			joysticks[i].botoes[j].liberado = false;
			joysticks[i].botoes[j].pressionado = false;
		}
	}


    // A estrutura que recebe os eventos
     SDL_Event event;
     // Enquanto houverem eventos a processar ...
     while(SDL_PollEvent( &event ))
     {
         // TRata de acordo com o tipo de evento
         switch( event.type )
         {
                case SDL_QUIT:
				C2D2_PressionaBotao(&teclas[C2D2_ENCERRA]);
				break;
                  
              case SDL_KEYDOWN:
              case SDL_KEYUP:
                        switch(event.key.keysym.sym)
                        {
                                case SDLK_UP:
                                    C2D2_ProcessaTecla(&teclas[C2D2_CIMA], event.type);
									break;
								case SDLK_DOWN:
									C2D2_ProcessaTecla(&teclas[C2D2_BAIXO], event.type);
									break;
								case SDLK_LEFT:
									C2D2_ProcessaTecla(&teclas[C2D2_ESQUERDA], event.type);
									break;									
								case SDLK_RIGHT:
									C2D2_ProcessaTecla(&teclas[C2D2_DIREITA], event.type);
									break;
                                case SDLK_ESCAPE:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_ESC], event.type);
                                  	break;
                                case SDLK_F1:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_F1], event.type);
                                  	break;
                                case SDLK_F2:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_F2], event.type);
                                  	break;
                                case SDLK_F3:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_F3], event.type);
                                  	break;
                                case SDLK_F4:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_F4], event.type);
                                  	break;
/*                                case SDLK_p:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_P], event.type);
                                  	break;*/
                                case SDLK_RETURN:
                                  	C2D2_ProcessaTecla(&teclas[C2D2_ENTER], event.type);
                                  	break;
								case SDLK_SPACE:
									C2D2_ProcessaTecla(&teclas[C2D2_ESPACO], event.type);
									break;
								case SDLK_LALT:
									C2D2_ProcessaTecla(&teclas[C2D2_LALT], event.type);
									break;
								case SDLK_RALT:
									C2D2_ProcessaTecla(&teclas[C2D2_RALT], event.type);
									break;
								case SDLK_LCTRL:
									C2D2_ProcessaTecla(&teclas[C2D2_LCTRL], event.type);
									break;
								case SDLK_RCTRL:
									C2D2_ProcessaTecla(&teclas[C2D2_RCTRL], event.type);
									break;
								case SDLK_LSHIFT:
									C2D2_ProcessaTecla(&teclas[C2D2_LSHIFT], event.type);
									break;
								case SDLK_RSHIFT:
									C2D2_ProcessaTecla(&teclas[C2D2_RSHIFT], event.type);
									break;
/*								case SDLK_z:
									C2D2_ProcessaTecla(&teclas[C2D2_Z], event.type);
									break;
								case SDLK_x:
									C2D2_ProcessaTecla(&teclas[C2D2_X], event.type);
									break;
								case SDLK_c:
									C2D2_ProcessaTecla(&teclas[C2D2_C], event.type);
									break;*/

                                    // Processa as teclas de letras do jeito mais f·cil
                                  default:  
                                        if(event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z)
                                            C2D2_ProcessaTecla(&teclas[C2D2_A+event.key.keysym.sym-SDLK_a], event.type);
                                        else if(event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)
                                            C2D2_ProcessaTecla(&teclas[C2D2_0+event.key.keysym.sym-SDLK_0], event.type);
                                  	break;
                        }
                        break;


			  case SDL_MOUSEMOTION:
				  mouse.x=event.motion.x;
				  mouse.y=event.motion.y;
				  break;
			  case SDL_MOUSEBUTTONDOWN:
				  mouse.x=event.button.x;
				  mouse.y=event.button.y;
				  switch(event.button.button)
				  {
				  case SDL_BUTTON_LEFT:
					  C2D2_PressionaBotao(&mouse.botoes[C2D2_MESQUERDO]);
					  break;
				  case SDL_BUTTON_RIGHT:
					  C2D2_PressionaBotao(&mouse.botoes[C2D2_MDIREITO]);
					  break;
				  case SDL_BUTTON_MIDDLE:
					  C2D2_PressionaBotao(&mouse.botoes[C2D2_MMEIO]);
					  break;
				  }
				  break;
			  case SDL_MOUSEBUTTONUP:
				  mouse.x=event.button.x;
				  mouse.y=event.button.y;
				  switch(event.button.button)
				  {
				  case SDL_BUTTON_LEFT:
					  C2D2_LiberaBotao(&mouse.botoes[C2D2_MESQUERDO]);
					  break;
				  case SDL_BUTTON_RIGHT:
					  C2D2_LiberaBotao(&mouse.botoes[C2D2_MDIREITO]);
					  break;
				  case SDL_BUTTON_MIDDLE:
					  C2D2_LiberaBotao(&mouse.botoes[C2D2_MMEIO]);
					  break;
				  }
				  break;

			  case SDL_JOYBUTTONDOWN:
			  case SDL_JOYBUTTONUP:
				  if(event.jbutton.which >= C2D2_MAX_JOYSTICKS)
					  break;

				  if(event.jbutton.button >= C2D2_MAX_JBOTOES)
					  break;

				  if(joysticks[event.jbutton.which].joystick == NULL)
					  break;

				  (event.jbutton.state == SDL_PRESSED ? C2D2_PressionaBotao : C2D2_LiberaBotao)(&joysticks[event.jbutton.which].botoes[event.jbutton.button]);
				  break;	

			  case SDL_JOYHATMOTION:
				  if(event.jhat.which >= C2D2_MAX_JOYSTICKS)
					  break;

				  if(event.jhat.hat >= C2D2_MAX_DIRECIONAIS)
					  break;

				  {
					C2D2_Joystick *joy = joysticks + event.jhat.which;
					if(joy->joystick == NULL)
					  break;			

					C2D2_AtualizaDirecional(&joy->direcional[event.jhat.hat][C2D2_CIMA], event.jhat.value, SDL_HAT_UP);
					C2D2_AtualizaDirecional(&joy->direcional[event.jhat.hat][C2D2_DIR_DIREITA], event.jhat.value, SDL_HAT_RIGHT);
					C2D2_AtualizaDirecional(&joy->direcional[event.jhat.hat][C2D2_DIR_BAIXO], event.jhat.value, SDL_HAT_DOWN);
					C2D2_AtualizaDirecional(&joy->direcional[event.jhat.hat][C2D2_DIR_ESQUERDA], event.jhat.value, SDL_HAT_LEFT);
				  }
				  break;

			  case SDL_JOYBALLMOTION:
				  if(event.jball.which >= C2D2_MAX_JOYSTICKS)
					  break;

				  if(event.jball.ball >= C2D2_MAX_DIRECIONAIS)
					  break;

				  C2D2_LiberaBotao(&mouse.botoes[C2D2_MMEIO]);
				  break;
                                            
              default:
					break;
        }
     }
}

// FunÁ„o para recuperar as teclas
//
// Data: 04/05/2007
//
C2D2_Botao* C2D2_PegaTeclas()
{
	return teclas;
}

// FunÁ„o para recuperar o mouse
//
// Data: 04/05/2007
//
C2D2_Mouse* C2D2_PegaMouse()
{
	return &mouse;
}

// FunÁ„o para zerar os dados da Chien2D 2
//
// Data: 29/03/2007

void reset()
{
	// Zera os sprites
	for(int i=0;i<C2D2_MAX_SPRITESET; i++)
	{
		strcpy(sprites[i].apelido, "");
		sprites[i].imagem = 0;
		sprites[i].usuarios = 0;
		sprites[i].bmask = 0;
		sprites[i].textura=0;
		sprites[i].texturaLigada=false;
	}
	// Zera as fontes
	for(int i=0;i<C2D2_MAX_FONTES;i++)
	{
		strcpy(fontes[i].apelido, "");
		fontes[i].idSpriteSet = 0;
		fontes[i].usuarios = 0;
	}
}

// Função auxiliar para pontos transparentes
//
// Data: 25/04/2007
//

bool PontoTransparente(SDL_Surface *surface, int x, int y)
{
	if(x >= surface->w || y >= surface->h)
		return false;
    // Quantos bytes tem por pixel a imagem
    int bpp = surface->format->BytesPerPixel;
    // Endereço do ponto a recuperar
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
    // Só funciona em imagens com 32 bits e canal de alfa
    Uint32 cor = *(Uint32 *)p;
    Uint32 cor2 = SDL_MapRGBA(surface->format, 255, 0, 255, 255);
    if((cor != cor2) && ((cor&AMASK) != 0) ) //(cor&surface->format->Amask) != 0)
        return false;
    else
        return true;
    
    return false;
}

///////////////////////////////////////////////////
// FunÁıes relacionadas com as bitmasks dos sprites
///////////////////////////////////////////////////

// FunÁ„o para criar uma bit mask para um sprite
//
// Data: 03/05/2007
//
C2D2_BitMask* C2D2_CriaBitMask(int largura, int altura)
{
	// Cria a Bitmask para retornar
	C2D2_BitMask* temp = (C2D2_BitMask*)malloc(sizeof(C2D2_BitMask));
	// Se falhou retorna uma m·scara nula
	if(temp == 0)
		return 0;
	// SE deu certo toca o barco
	temp->altura = altura;
	temp->largura = largura;
	// O n˙mero de linhas est· correto. Falta saber quantos quadwords s„o necess·rios por linha
	int quads = largura/32;
	// Verifica se n„o precisa de um extra
	if(largura%32)
		quads++;
	// Aloca o vetor da m·scara em si
	temp->ldwords = quads;
	temp->mask = (Uint32*)malloc(sizeof(Uint32)*quads*altura);
	// Se falhou, remove e retorna nulo
	if(temp->mask == 0)
	{
		// Como falhou a alocaÁ„o da m·scara, tem que apagar o bit mask
		free(temp);
		// Retorna o ponteiro nulo
		return 0;
	}
	// Se chegou atÈ aqui, temp possui a bitmask completa
	return temp;
}

// FunÁ„o para remover uma bitmask de um sprite
//
// Data: 03/05/2007
//
void C2D2_RemoveBitMask(C2D2_BitMask* bmask)
{
	// Verifica se tem corretamente a m·scara
	if(bmask->mask)
		free(bmask->mask);
	// Remove o bitmask
	free(bmask);
}

// FunÁ„o para criar o vetor de bitmasks de um spriteset
//
// Data: 03/05/2007
//
C2D2_BitMask** C2D2_CriaVetorBitMasks(int spx, int spy, int limagem, int aimagem)
{
	// Cria as m·scaras de bits
	C2D2_BitMask **masks = 0;
	// Quantas m·scaras de bits s„o necess·rias?
	int colunas = limagem/spx;
	int linhas = aimagem/spy;
	int nummasks = colunas*linhas;
	// Aloca o vetor de m·scaras
	masks=(C2D2_BitMask**)malloc(sizeof(C2D2_BitMask*)*nummasks);
	// Se n„o alocou com sucesso deve retornar a falha. DifÌcil, mas vai que acontece o 0,00001% de chance de erro
	if(masks == 0)
		return 0;
	// Zera o vetor de ponteiro de m·scaras
	memset(masks, 0, sizeof(C2D2_BitMask*)*nummasks);
	// Uma vez inicializado o vetor de m·scaras, devemo palocar as m·scaras uma a uma
	int conta;
	for(conta=0;conta<nummasks;conta++)
	{
		masks[conta] = C2D2_CriaBitMask(spx, spy);
		if(masks[conta] == 0)
			break;
	}
	// Verifica se bugou
	if(conta != nummasks)
	{
		// Se entrou aqui, tem que apagar o que alocou atÈ agora e retornar nulo
		// Apaga as m·scaras que foram alocadas corretamente
		for(int i=0;i<conta-1;i++)
			C2D2_RemoveBitMask(masks[i]);
		// Remove o vetor de m·scaras
		free(masks);
		// Retorna o vetor nulo
		return 0;
	}
	// Se chegou aqui È porquÍ tudo est· certo. Retorna as m·scaras
	return masks;
}

// FunÁ„o para setar um bit do bitmask
//
// Data: 03/05/2007
//
void C2D2_SetBit(C2D2_BitMask *bmask, int x, int y, int val)
{
	// Testa se pode colocar um bit nesta posiÁ„o
	if(x<0 || (x>=bmask->largura && y<0 && y>=bmask->altura))
		return;
	// Determina em qual quadword da linha vai o bit
	int quad = x/32;
	// Determina em qual bit da quadword vai o bit do usu·rio
	int bitindex = x%32;
	// Cria a m·scara para acessar o bit
	Uint32 mask = 0x80000000;
	// Rotaciona os bits
	mask = mask >> bitindex;
	// Coloca o bit na m·scara
	if(val)
		bmask->mask[y*bmask->ldwords + quad] = bmask->mask[y*bmask->ldwords + quad] | mask;
	else
		bmask->mask[y*bmask->ldwords + quad] = bmask->mask[y*bmask->ldwords + quad] & ~mask;
}

// FunÁ„o para setar um bit do bitmask
//
// Data: 03/05/2007
//
bool C2D2_GetBit(C2D2_BitMask *bmask, int x, int y)
{
	// Testa se pode pegar um bit nesta posiÁ„o
	if(x<0 || (x>=bmask->largura && y<0 && y>=bmask->altura))
		return false;
	// Determina em qual quadword da linha vai o bit
	int quad = x/32;
	// Determina em qual bit da quadword vai o bit do usu·rio
	int bitindex = x%32;
	// Cria a m·scara para acessar o bit
	Uint32 mask = 0x80000000;
	// Rotaciona os bits
	mask = mask >> bitindex;
	// Coloca o bit na m·scara
	if(bmask->mask[y*bmask->ldwords + quad] & mask)
		return true;
	else
		return false;
}


// Função para preencher o vetor de bitmasks de um spriteset
//
// Data: 03/05/2007
//
void C2D2_PreencheVetorBitMasks(SDL_Surface *imagem, int spx, int spy, int limagem, int aimagem, C2D2_BitMask** bmask)
{
	// Quantas máscaras de bits são necessárias?
	int colunas = limagem/spx;
	int linhas = aimagem/spy;
	// A máscara local
	C2D2_BitMask* lbmask;
	// As coordenadas x e y iniciais no sprite dentro da imagem
	int posx,posy;
	// As coordenadas x e y dentro do sprite
	int x,y;
	// Trava a surface SDL
    SDL_LockSurface(imagem);
	// Para cada sprite no spriteset
	for(int l=0;l<linhas;l++)
		for(int c=0;c<colunas;c++)
		{
			// Pega os pontos dentro da imagem
			posx=c*spx;
			posy=l*spy;
			// Pega a máscara sendo acessada
			lbmask = bmask[l*colunas+c];
			// Pega as posições do sprite (ufa)
			for(y=0;y<spy;y++)
                for(x=0;x<spx;x++) {

					if(PontoTransparente(imagem, x+posx, y+posy))
						C2D2_SetBit(lbmask, x, y, 0);
					else
						C2D2_SetBit(lbmask, x, y, 1);
                }
		}

	// Destrava a surface SDL
    SDL_UnlockSurface(imagem);
}


// FunÁ„o para testar a colis„o entre dois bitmasks, dado um offset x e y da segunda em relaÁ„o ‡ primeira
//
// Data: 03/05/2007
//
int C2D2_SobrepoeBitMasks(C2D2_BitMask *a, C2D2_BitMask *b, int offx, int offy)
{
	// Coisa idiota, faz um dump das m·scaras
	/*printf("Sprite1:\n");
	for(int i=0;i<a->altura;i++)
	{
		for(int j=0;j<a->ldwords;j++)
			printf("%i ", a->mask[i*a->ldwords + j]);
		printf("\n");
	}*/
			
	// A largura e altura a comparar
	int largura, altura;
	// O ponto DENTRO das bitmasks a comparar
	int xa, ya, xb, yb;
	
	// Calcula a largura e os pontos x dentro da imagem. Esta parte de longe È a mais embaÁada
	// do processo. Mas passando isso È sossegado.

	// Se a imagem b est· ‡ direita
	if(offx>=0)
	{
		// Como o offset È positivo, ele representa o ponto inicial na imagem 1
		xa=offx;
		// Como a imagem b est· ‡ direita, comeÁa no ponto 0
		xb=0;
		// A largura a princÌpio È a da m·scara a menos o offset
		largura = a->largura-offx;
		// Se a largura for negativa ou zero, retorna
		if(largura <= 0)
			return 0;
		// Caso a largura da colis„o seja menor que a largura da m·scara b, esta passa a ser a largura a testar
		if(largura>b->largura)
			largura=b->largura;
	}
	// Sen„o ela est· ‡ esquerda
	else
	{
		// Como a imagem a est· ‡ direita, comeÁa no ponto 0
		xa=0;
		// Como o offset È negativo, o xb È o mÛdulo do offset
		xb=-offx;
		// A largura a princÌpio È a da m·scara b mais o offset
		largura = b->largura+offx;
		// Se a largura for negativa ou zero, retorna
		if(largura <= 0)
			return 0;
		// Caso a largura da colis„o seja menor que a largura da m·scara a, esta passa a ser a largura a testar
		if(largura>a->largura)
			largura=a->largura;
	}
	// Se a imagem a est· acima
	if(offy>=0)

	{

		// Como o offset y È positivo, ele È o ponto y inicial da imagem a
		ya = offy;
		// Como a imagem b est· abaixo, o ponto inicial È 0
		yb=0;
		// A largura a princÌpio È a altura da m·scara a menos o offset
		altura = a->altura - offy;
		// Se a altura for zero ou menor, pula fora
		if(altura <= 0)
			return 0;
		// Caso a altura da colis„o seja maior que a altura da m·scara b, esta passa a ser a altura a testar
		if(altura>b->altura)
			altura=b->altura;
	}
	// Caso contr·rio, a imagem b est· acima
	else
	{
		// Como a imagem a est· abaixo, 0 È o ponto inicial y dela
		ya=0;
		// Como o offset È negativo, o mÛdulo dele È o ponto y inicial na imagem b
		yb=-offy;
		// A altura È a da m·scara b mais o offset
		altura=b->altura+offy;
		// Se a altura for zero ou menor, pula fora
		if(altura<=0)
			return 0;
		// Caso a altura da colis„o for mais alta que a imagem a, esta passa a ser a altura da colis„o
		if(altura>a->altura)
			altura=a->altura;
	}

	// Enfim, com as m·scaras devidamente alinhadas, podemos comeÁar as comparaÁıes de verdade

	// Faz um for para os quadwords inteiros que ele busca
	int quads=largura/32;
	// Os quads e bits a indexar
	int quada, quadb, bita, bitb;
	// Os valores a testar
	int vquada, vquadb, temp;
	// Calcula o offset de bits
	bita=xa%32;
	bitb=xb%32;
	// A m·scara de bits
	Uint32 mask;
	// Calcula o quad inicial a testar
	quada=xa/32+ya*a->ldwords;
	quadb=xb/32+yb*b->ldwords;

	for(int l=0;l<altura;l++)
		for(int c=0;c<quads;c++)
		{
			//
			// Recupera o valor do quad da m·scara A
			//

			// Coloca todos os bits da m·scara em 1
			mask = 0xFFFFFFFF;
			// Rotaciona para a direita os bits da m·scara em bita bits
			mask = mask >> bita;
			// Recupera a primeira parte do valor quad
			vquada = a->mask[quada + c +l*a->ldwords] & mask;
			// Coloca os bits no comeÁo (importante)
			vquada = vquada << bita;
			// Testa se tem que recuperar outra parte do valor do quad (offset em bits diferente de 0)
			if(bita>0)
			{
				// Coloca todos os bits da m·scara em 1
				mask = 0xFFFFFFFF;
				// Rotaciona para a esquerda os bits da m·scara em 32-bita bits
				mask = mask << (32-bita);
				// Recupera os bits
				temp = a->mask[quada + c + 1 + l*a->ldwords] & mask;
				// Coloca os bits para a direita
				temp = temp >> (32-bita);
				// Consolida com o resto da m·scara o valor do quad
				vquada = vquada | temp;
			}
			//
			// Recupera o valor do quad da m·scara B
			//

			// Coloca todos os bits da m·scara em 1
			mask = 0xFFFFFFFF;
			// Rotaciona para a direita os bits da m·scara em bita bits
			mask = mask >> bitb;
			// Recupera a primeira parte do valor quad
			vquadb = b->mask[quadb + l*b->ldwords] & mask;
			// Coloca os bits no comeÁo (importante)
			vquadb = vquadb << bitb;

			// Testa se tem que recuperar outra parte do valor do quad (offset em bits diferente de 0)
			if(bitb>0)
			{
				// Coloca todos os bits da m·scara em 1
				mask = 0xFFFFFFFF;
				// Rotaciona para a esquerda os bits da m·scara em 32-bita bits
				mask = mask << (32-bitb);
				// Recupera os bits
				temp = b->mask[quadb + c + 1 + l*b->ldwords] & mask;
				// Coloca os bits para a direita
				temp = temp >> (32-bitb);
				// Consolida com o resto da m·scara o valor do quad
				vquadb = vquadb | temp;
			}

			// testa, enfim, se colidiu
			if(vquada & vquadb)
				return -1;
		}
	// Faz um ˙ltimo for para o quadword quebrado que tem que recuperar
	if(largura%32)
	{
		// Calcula o quad inicial a testar
		quada=xa/32+quads+ya*a->ldwords;
		quadb=xb/32+quads+yb*b->ldwords;
		// Calcula o quadword inicial
		for(int l=0;l<altura;l++)
		{
			//
			// Recupera o valor do quad da m·scara A
			//

			// Coloca todos os bits da m·scara em 1
			mask = 0xFFFFFFFF;
			// Rotaciona para a direita os bits da m·scara em bita bits
			mask = mask >> bita;
			// Recupera a parte final do valor quad
			vquada = a->mask[quada + l*a->ldwords] & mask;
			// Rotaciona o quad para colocar na posiÁ„o inicial os bits
			vquada = vquada << bita;
			// Testa se tem que recuperar outra parte do valor do quad (offset em bits diferente de 0)
			if(32-bita < largura%32)
			{
				// Coloca todos os bits da m·scara em 1
				mask = 0xFFFFFFFF;
				// Rotaciona para a esquerda os bits da m·scara em 32-bita bits
				mask = mask << (largura%32 - (32-bita));
				// Recupera os bits
				temp = a->mask[quada + 1 + l*a->ldwords] & mask;
				// Coloca os bits para a direita
				temp = temp >> (largura%32 - (32-bita));
				// Consolida com o resto da m·scara o valor do quad
				vquada = vquada | temp;
			}


			//
			// Recupera o valor do quad da m·scara B
			//

			// Coloca todos os bits da m·scara em 1
			mask = 0xFFFFFFFF;
			// Rotaciona para a direita os bits da m·scara em bita bits
			mask = mask >> bitb;
			// Recupera a primeira parte do valor quad
			vquadb = b->mask[quadb + l*b->ldwords] & mask;
			// Rotaciona o quad para colocar na posiÁ„o inicial os bits
			vquadb = vquadb << bitb;
			// Testa se tem que recuperar outra parte do valor do quad (offset em bits diferente de 0)
			if(32-bitb < largura%32)
			{
				// Coloca todos os bits da m·scara em 1
				mask = 0xFFFFFFFF;
				// Rotaciona para a esquerda os bits da m·scara em 32-bita bits
				mask = mask << (largura%32 - (32-bitb));
				// Recupera os bits
				temp = b->mask[quadb + 1 + l*b->ldwords] & mask;
				// Coloca os bits para a direita
				temp = temp >> (largura%32 - (32-bitb));
				// Consolida com o resto da m·scara o valor do quad
				vquadb = vquadb | temp;
			}


			// Coloca a m·scara para pegar apenas a diferenÁa de pixels
			mask = 0xFFFFFFFF;
			mask = mask << (32-largura%32);
			vquada = vquada & mask;
			vquadb = vquadb & mask;

			// testa, enfim, se colidiu
			if(vquada & vquadb)
				return -1;
		}
	}
	// Se chegou atÈ aqui, È porquÍ n„o colidiu. Ent„o retorna falso
	return 0;
}

// FunÁ„o para dar uma pausa na aplicaÁ„o.
//
// Data: 17/05/2009
//
void C2D2_Pausa(Uint32 pausa)
{
	SDL_Delay(pausa);
}

// FunÁ„o que indica qual a funÁ„o de sincronizaÁ„o do usu·rio vai ser usada.
//
// Data: 24/07/2010
void C2D2_DefineSincronizaUsuario(void (*funcao)())
{
    C2D2_SincronizaUsuario = funcao;
}

//
//
//Lists os Joysticks no console
int C2D2_ListaJoysticks()
{
	int num = SDL_NumJoysticks();
	printf("Foram econtrados %i joysticks.\n\n", num );

	if(num > 0)
	{
		printf("Sendo nomeados:\n");
			
		for( int i=0; i < num; i++ ) 
		{
			printf("    %s\n", SDL_JoystickNameForIndex(i));
		}
	}

	return num;
}

bool C2D2_LigaJoystick(int index)
{
	C2D2_Joystick *joystick = C2D2_PegaJoystick(index);
	if(joystick == NULL)
		return false;

	if(joystick->joystick != NULL)
		return true;

	joysticks[index].joystick = SDL_JoystickOpen(index);
	if(joysticks[index].joystick == NULL)
		return false;

	printf(
		"Ligando joystick: %s, numBotoes: %d, numDirecionais: %d, numBalls: %d, numAxes: %d\n", 
		SDL_JoystickNameForIndex(index), 
		SDL_JoystickNumButtons(joysticks[index].joystick),
		SDL_JoystickNumHats(joysticks[index].joystick),
		SDL_JoystickNumBalls(joysticks[index].joystick),
		SDL_JoystickNumAxes(joysticks[index].joystick)
	);

	return joysticks[index].joystick != NULL;
}

void C2D2_DesligaJoystick(int index)
{
	C2D2_Joystick *joystick = C2D2_PegaJoystick(index);
	if(joystick == NULL)
		return;

	if(joystick->joystick == NULL)
		return;
	
	SDL_JoystickClose(joystick->joystick);	
	joystick->joystick = NULL;
}

C2D2_Joystick *C2D2_PegaJoystick(int index)
{
	assert(index < C2D2_MAX_JOYSTICKS);
	assert(index >= 0);

	if((index < 0) || (index >= C2D2_MAX_JOYSTICKS))
		return NULL;

	return joysticks + index;
}

