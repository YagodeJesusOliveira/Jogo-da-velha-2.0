#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define LINHA 3
#define COLUNA 3
#define ARQUIVO_REPLAY "historico_partida.txt"
#define LARGURA 44   // largura interna das caixas decorativas

// ---------- Cores (codigos ANSI) ----------
#define RESET    "\033[0m"
#define NEGRITO  "\033[1m"
#define VERMELHO "\033[31m"
#define VERDE    "\033[32m"
#define AMARELO  "\033[33m"
#define CIANO    "\033[36m"
#define CINZA    "\033[90m"

// ======================================================
//  FUNCOES DE APOIO (terminal, entrada e visual)
// ======================================================

// Liga o suporte a cores no terminal do Windows
void ativarCores(){
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &modo)) {
        SetConsoleMode(h, modo | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    }
#endif
}

void limparTela(){
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Le uma linha inteira do teclado. Se o teclado for fechado (EOF), encerra o programa.
// Se o usuario digitar mais do que cabe no buffer, descarta o resto da linha
// (assim nao sobra lixo para a proxima leitura).
void lerLinha(char *buffer, int tamanho){
    fflush(stdout);
    if (fgets(buffer, tamanho, stdin) == NULL) {
        exit(0);
    }
    if (strchr(buffer, '\n') == NULL) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }
}

// Le um numero inteiro de forma segura.
// Devolve -1 quando a entrada for invalida (letras, vazio, numero + texto...).
int lerInteiro(){
    char buffer[64];
    char *fim;
    long valor;

    lerLinha(buffer, sizeof(buffer));
    valor = strtol(buffer, &fim, 10);

    if (fim == buffer) {            // nenhum digito foi lido
        return -1;
    }
    while (*fim == ' ' || *fim == '\t' || *fim == '\r' || *fim == '\n') {
        fim++;
    }
    if (*fim != '\0') {             // sobrou texto depois do numero
        return -1;
    }
    if (valor < 0 || valor > 1000) {
        return -1;
    }
    return (int) valor;
}

// Espera o usuario apertar ENTER
void pausar(){
    char buffer[64];
    printf("\nPressione ENTER para continuar...");
    lerLinha(buffer, sizeof(buffer));
}

// Cor de cada peca do tabuleiro
const char *corPeca(char peca){
    if (peca == 'X') return CIANO;
    if (peca == 'O') return AMARELO;
    return CINZA;
}

void linhaCaixa(){
    printf(CIANO "  +");
    for (int i = 0; i < LARGURA; i++) {
        printf("-");
    }
    printf("+" RESET "\n");
}

// Escreve um titulo centralizado dentro da caixa
void tituloCaixa(const char *texto){
    int tam = (int) strlen(texto);
    int esq = (LARGURA - tam) / 2;
    int dir = LARGURA - tam - esq;
    printf(CIANO "  |" RESET "%*s" NEGRITO "%s" RESET "%*s" CIANO "|" RESET "\n", esq, "", texto, dir, "");
}

void opcaoMenu(const char *numero, const char *texto){
    printf(CIANO "  |" RESET " " AMARELO "[%s]" RESET " %-38s " CIANO "|" RESET "\n", numero, texto);
}

void secao(const char *titulo){
    printf("\n" AMARELO NEGRITO "%s" RESET "\n", titulo);
    printf(CINZA "--------------------------------------------------" RESET "\n");
}

// ======================================================
//  MENU E TABULEIRO
// ======================================================

void menu(){
    printf("\n");
    linhaCaixa();
    tituloCaixa("JOGO DA VELHA 2.0");
    linhaCaixa();
    opcaoMenu("1", "JOGADOR X JOGADOR");
    opcaoMenu("2", "JOGADOR X COMPUTADOR");
    opcaoMenu("3", "AJUDA");
    opcaoMenu("4", "REPLAY");
    opcaoMenu("5", "AUTORES");
    opcaoMenu("6", "SAIR");
    linhaCaixa();
    printf("\n  Escolha uma das opcoes: ");
}

void ExibirMatriz(char matriz[LINHA][COLUNA]){
    printf("\n      ");
    for(int j=0; j<COLUNA; j++){
        printf(CINZA "%d   " RESET, j);
    }
    printf("\n");

    for(int i=0; i<LINHA; i++){
        printf("    +");
        for(int j=0; j<COLUNA; j++){
            printf("---+");
        }
        printf("\n");

        printf(CINZA "  %d " RESET "|", i);
        for(int j=0; j<COLUNA; j++){
            printf(" %s%c" RESET " |", corPeca(matriz[i][j]), matriz[i][j]);
        }
        printf("\n");
    }

    printf("    +");
    for(int j=0; j<COLUNA; j++){
        printf("---+");
    }
    printf("\n\n");
}

bool ValidandoEscolha(int escolha){
    if(escolha >= 3 || escolha < 0 ){
        printf(VERMELHO "\nEscolha invalida.. Digite 0, 1 ou 2." RESET "\n");
        return false;
    }else{
        return true;
    }
}

bool VerificandoJogo(char matriz[LINHA][COLUNA], char jogador){
    for(int i=0;i < LINHA; i++){
        if(matriz[i][0] == jogador && matriz[i][1] == jogador && matriz[i][2] == jogador){
            return true;
        }
    }
    for(int j=0;j < COLUNA; j++){
        if(matriz[0][j] == jogador && matriz[1][j] == jogador && matriz[2][j] == jogador){
            return true;
        }
    }
    if(matriz[0][0] == jogador && matriz[1][1] == jogador && matriz[2][2] == jogador){
        return true;
    }
    if(matriz[0][2] == jogador && matriz[1][1] == jogador && matriz[2][0] == jogador){
        return true;
    }
    return false;
}

// ======================================================
//  JOGOS
// ======================================================

void JogadorxJogador(){
    char matriz[LINHA][COLUNA];
    int posicaoX[3][2], posicaoO[3][2];
    int qtdX = 0, qtdO = 0;
    char jogador = 'X';
    int linha, coluna;
    bool completo = false, valida = false, livre = true;

    // Abre/cria o arquivo em modo "w" (escrita), limpando qualquer partida antiga
    FILE *arq = fopen(ARQUIVO_REPLAY, "w");
    if (arq == NULL) {
        printf("Erro ao criar o arquivo de replay!\n");
    }

    for(int i=0; i<LINHA; i++){
        for(int j=0; j<COLUNA; j++){
            matriz[i][j] = '_';
        }
    }

    ExibirMatriz(matriz);

    do{
        do{
            do{
                printf("Jogador [%s%c" RESET "]\n", corPeca(jogador), jogador);
                printf("\nLINHA: ");
                linha = lerInteiro();
                valida = ValidandoEscolha(linha);
            }while(!valida);

            do{
                printf("\nCOLUNA: ");
                coluna = lerInteiro();
                valida = ValidandoEscolha(coluna);
            }while(!valida);

            if(matriz[linha][coluna] != '_'){
                printf(VERMELHO "\nEscolha ja ocupada.." RESET "\n");
                livre = false;
            }else{
                livre = true;
            }
        }while(!livre);

        matriz[linha][coluna] = jogador;

        // Salva o movimento atual direto dentro do arquivo de texto
        if (arq != NULL) {
            fprintf(arq, "%c %d %d\n", jogador, linha, coluna);
        }

        if(jogador == 'X'){
            if(qtdX < 3){
                posicaoX[qtdX][0] = linha;
                posicaoX[qtdX][1] = coluna;
                qtdX++;
            }else{
                matriz[posicaoX[0][0]][posicaoX[0][1]] = '_';
                for(int i=0; i<2; i++){
                    posicaoX[i][0] = posicaoX[i+1][0];
                    posicaoX[i][1] = posicaoX[i+1][1];
                }
                posicaoX[2][0] = linha;
                posicaoX[2][1] = coluna;
            }
        }else{
            if(qtdO < 3){
                posicaoO[qtdO][0] = linha;
                posicaoO[qtdO][1] = coluna;
                qtdO++;
            }else{
                matriz[posicaoO[0][0]][posicaoO[0][1]] = '_';
                for(int i=0; i<2; i++){
                    posicaoO[i][0] = posicaoO[i+1][0];
                    posicaoO[i][1] = posicaoO[i+1][1];
                }
                posicaoO[2][0] = linha;
                posicaoO[2][1] = coluna;
            }
        }

        ExibirMatriz(matriz);
        completo = VerificandoJogo(matriz,jogador);

        if(completo == true) break;
        jogador = (jogador == 'X') ? 'O' : 'X';

    }while(completo == false);

    // Fecha o arquivo ao finalizar a partida
    if (arq != NULL) {
        fclose(arq);
    }

    printf(VERDE "\n====JOGO COMPLETO====" RESET "\n");
    printf("JOGADOR [%s%c" RESET "] GANHOU!!!\n", corPeca(jogador), jogador);
}

void JogadorxComputador(){
    char matriz[LINHA][COLUNA];
    int posicaoX[3][2], posicaoO[3][2];
    int qtdX = 0, qtdO = 0;
    char jogador = 'X';
    int linha, coluna;
    bool completo = false, valida = false, livre = true;

    // Abre/cria o arquivo em modo "w" (escrita), limpando qualquer partida antiga
    FILE *arq = fopen(ARQUIVO_REPLAY, "w");
    if (arq == NULL) {
        printf("Erro ao criar o arquivo de replay!\n");
    }

    for(int i=0; i<LINHA; i++){
        for(int j=0; j<COLUNA; j++){
            matriz[i][j] = '_';
        }
    }

    ExibirMatriz(matriz);

    do{
        jogador = 'X';
        do{
            do{
                printf("Jogador [%s%c" RESET "]\n", corPeca(jogador), jogador);
                printf("\nLINHA: ");
                linha = lerInteiro();
                valida = ValidandoEscolha(linha);
            }while(!valida);

            do{
                printf("\nCOLUNA: ");
                coluna = lerInteiro();
                valida = ValidandoEscolha(coluna);
            }while(!valida);

            if(matriz[linha][coluna] != '_'){
                printf(VERMELHO "\nEscolha ja ocupada.." RESET "\n");
                livre = false;
            }else{
                livre = true;
            }
        }while(!livre);

        matriz[linha][coluna] = jogador;

        // Salva a jogada do Player no arquivo
        if (arq != NULL) {
            fprintf(arq, "%c %d %d\n", jogador, linha, coluna);
        }

        if(qtdX < 3){
            posicaoX[qtdX][0] = linha;
            posicaoX[qtdX][1] = coluna;
            qtdX++;
        }else{
            matriz[posicaoX[0][0]][posicaoX[0][1]] = '_';
            for(int i=0; i<2; i++){
                posicaoX[i][0] = posicaoX[i+1][0];
                posicaoX[i][1] = posicaoX[i+1][1];
            }
            posicaoX[2][0] = linha;
            posicaoX[2][1] = coluna;
        }

        completo = VerificandoJogo(matriz,'X');
        if(completo == true){
            ExibirMatriz(matriz);
            jogador = 'X';
            break;
        }

        do{
            linha = rand() % 3;
            coluna = rand() % 3;
        }while(matriz[linha][coluna] != '_');

        matriz[linha][coluna] = 'O';

        // Salva a jogada do Computador no arquivo
        if (arq != NULL) {
            fprintf(arq, "O %d %d\n", linha, coluna);
        }

        if(qtdO < 3){
            posicaoO[qtdO][0] = linha;
            posicaoO[qtdO][1] = coluna;
            qtdO++;
        }else{
            matriz[posicaoO[0][0]][posicaoO[0][1]] = '_';
            for(int i=0; i<2; i++){
                posicaoO[i][0] = posicaoO[i+1][0];
                posicaoO[i][1] = posicaoO[i+1][1];
            }
            posicaoO[2][0] = linha;
            posicaoO[2][1] = coluna;
        }

        printf("\nJogada do Computador:\n");
        ExibirMatriz(matriz);

        completo = VerificandoJogo(matriz,'O');
        if(completo == true){
            jogador = 'O';
            break;
        }

    }while(completo == false);

    // Fecha o arquivo ao finalizar a partida
    if (arq != NULL) {
        fclose(arq);
    }

    printf(VERDE "\n====JOGO COMPLETO====" RESET "\n");
    printf("JOGADOR [%s%c" RESET "] GANHOU!!!\n", corPeca(jogador), jogador);
}

// ======================================================
//  AJUDA, REPLAY, AUTORES E SAIR
// ======================================================

void Ajuda() {
    printf("\n");
    linhaCaixa();
    tituloCaixa("AJUDA - JOGO DA VELHA 2.0");
    linhaCaixa();

    secao("COMO FUNCIONA O JOGO?");
    printf("O Jogo da Velha 2.0 e uma versao diferente do jogo\n");
    printf("da velha tradicional. Ele utiliza um tabuleiro 3x3,\n");
    printf("onde dois jogadores disputam para alinhar suas\n");
    printf("tres pecas na horizontal, vertical ou diagonal.\n");
    printf("\n");
    printf("A principal diferenca e que cada jogador pode ter\n");
    printf("somente 3 pecas no tabuleiro ao mesmo tempo.\n");

    secao("SIMBOLOS DO TABULEIRO");
    printf("X = primeiro jogador\n");
    printf("O = segundo jogador (ou o computador)\n");
    printf("_ = posicao vazia\n");

    secao("COMO JOGAR?");
    printf("1. O primeiro jogador (X) escolhe uma posicao vazia\n");
    printf("   informando a LINHA e a COLUNA.\n");
    printf("\n");
    printf("   ATENCAO: linhas e colunas vao de 0 a 2, ou seja,\n");
    printf("   a contagem comeca no 0. Exemplo: linha 0 e\n");
    printf("   coluna 2 e o canto superior direito.\n");
    printf("\n");
    printf("2. O segundo jogador (O) faz o mesmo.\n");
    printf("\n");
    printf("3. Os jogadores alternam suas jogadas ate que\n");
    printf("   cada um tenha colocado suas tres pecas.\n");
    printf("\n");
    printf("4. Ao colocar a quarta peca, o jogador deve mover\n");
    printf("   a primeira peca que colocou: ele escolhe a nova\n");
    printf("   posicao (vazia) e a peca mais antiga sai do\n");
    printf("   lugar onde estava.\n");
    printf("\n");
    printf("5. Na quinta jogada, move a segunda peca colocada,\n");
    printf("   e assim sucessivamente.\n");
    printf("\n");
    printf("6. Nao e permitido colocar a peca na mesma posicao\n");
    printf("   em que ela ja estava. O programa recusa a jogada,\n");
    printf("   pois essa posicao ainda esta ocupada.\n");

    secao("COMO VENCER?");
    printf("Vence o primeiro jogador que conseguir alinhar\n");
    printf("suas tres pecas na horizontal, vertical ou diagonal.\n");

    secao("OPCOES DO MENU");
    printf("1 - JOGADOR X JOGADOR:\n");
    printf("Permite que duas pessoas joguem uma contra a outra,\n");
    printf("alternando suas jogadas.\n");
    printf("\n");

    printf("2 - JOGADOR X COMPUTADOR:\n");
    printf("Permite que uma pessoa jogue contra o computador.\n");
    printf("Voce joga com X e o computador joga com O,\n");
    printf("fazendo as jogadas automaticamente.\n");
    printf("\n");

    printf("3 - AJUDA:\n");
    printf("Exibe as regras e instrucoes de funcionamento.\n");
    printf("\n");

    printf("4 - REPLAY:\n");
    printf("Mostra novamente a ultima partida jogada, jogada\n");
    printf("a jogada. Pressione ENTER para avancar para a\n");
    printf("proxima jogada, ou digite Q e ENTER para sair.\n");
    printf("\n");

    printf("5 - AUTORES:\n");
    printf("Exibe os nomes dos integrantes responsaveis pelo\n");
    printf("desenvolvimento do projeto.\n");
    printf("\n");

    printf("6 - SAIR:\n");
    printf("Encerra a execucao do programa.\n");

    printf("\n");
    linhaCaixa();
    tituloCaixa("BOM JOGO E BOA SORTE!");
    linhaCaixa();
}

void Replay(){
    // Abre o arquivo em modo de leitura ("r")
    FILE *arq = fopen(ARQUIVO_REPLAY, "r");
    if (arq == NULL) {
        printf(VERMELHO "\nNenhum historico de jogo em arquivo foi encontrado!" RESET "\n");
        return;
    }

    char matriz[LINHA][COLUNA];
    int posicaoX[3][2], posicaoO[3][2];
    int qtdX = 0, qtdO = 0;
    char entrada[64];
    char jogador;
    int linha, coluna;
    int jogada_num = 1;

    for(int i=0; i<LINHA; i++){
        for(int j=0; j<COLUNA; j++){
            matriz[i][j] = '_';
        }
    }

    printf("\n");
    linhaCaixa();
    tituloCaixa("REPLAY DA ULTIMA PARTIDA");
    linhaCaixa();

    // Le linha por linha do arquivo
    while (fscanf(arq, " %c %d %d", &jogador, &linha, &coluna) == 3) {
        matriz[linha][coluna] = jogador;

        // Aplica as regras de remocao apos 3 pecas
        if(jogador == 'X'){
            if(qtdX < 3){
                posicaoX[qtdX][0] = linha;
                posicaoX[qtdX][1] = coluna;
                qtdX++;
            }else{
                matriz[posicaoX[0][0]][posicaoX[0][1]] = '_';
                for(int i=0; i<2; i++){
                    posicaoX[i][0] = posicaoX[i+1][0];
                    posicaoX[i][1] = posicaoX[i+1][1];
                }
                posicaoX[2][0] = linha;
                posicaoX[2][1] = coluna;
            }
        }else{
            if(qtdO < 3){
                posicaoO[qtdO][0] = linha;
                posicaoO[qtdO][1] = coluna;
                qtdO++;
            }else{
                matriz[posicaoO[0][0]][posicaoO[0][1]] = '_';
                for(int i=0; i<2; i++){
                    posicaoO[i][0] = posicaoO[i+1][0];
                    posicaoO[i][1] = posicaoO[i+1][1];
                }
                posicaoO[2][0] = linha;
                posicaoO[2][1] = coluna;
            }
        }

        printf("\nMovimento %d: Jogador [%s%c" RESET "] jogou na linha %d, coluna %d\n",
               jogada_num++, corPeca(jogador), jogador, linha, coluna);
        ExibirMatriz(matriz);

        printf("ENTER para continuar (ou digite 'q' e ENTER para sair): ");
        lerLinha(entrada, sizeof(entrada));

        char *p = entrada;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == 'q' || *p == 'Q') {
            break;
        }
    }

    fclose(arq); // Fecha o arquivo lido

    if (jogada_num == 1) {
        printf(VERMELHO "\nO arquivo de historico esta vazio." RESET "\n");
    }
    printf(VERDE "\n======== FIM DO REPLAY ========" RESET "\n");
}

void Autores(){
    printf("\n");
    linhaCaixa();
    tituloCaixa("AUTORES");
    linhaCaixa();
    printf("\n");
    printf("  Carlos Misael Macambira Pereira - RA: 12632519\n");
    printf("  Paulo Henrique Martins - RA: 12632524\n");
    printf("  Yago de Jesus Oliveira - RA: 12632388\n");
}

void Sair(){
    printf(AMARELO "\nFechando o jogo..." RESET "\n");
}

// ======================================================
//  PROGRAMA PRINCIPAL
// ======================================================

int main(){
    int opcao;

    ativarCores();
    srand((unsigned int) time(NULL)); // semente do rand(): computador joga diferente a cada execucao

    do {
        limparTela();
        menu();

        opcao = lerInteiro();
        printf("\n");

        switch (opcao){
            case 1:
                JogadorxJogador();
                break;

            case 2:
                JogadorxComputador();
                break;

            case 3:
                Ajuda();
                break;

            case 4:
                Replay();
                break;

            case 5:
                Autores();
                break;

            case 6:
                Sair();
                break;

            default:
                printf(VERMELHO "Opcao invalida. Digite um numero de 1 a 6." RESET "\n");
                break;
        }

        if (opcao != 6) {
            pausar();   // deixa o usuario ler o resultado antes de limpar a tela
        }

    } while (opcao != 6);

    return 0;
}