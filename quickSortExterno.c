#include "quickSortExterno.h"
#include "uteis.h"
#include <string.h>
#include <time.h>
#include <limits.h>
#include <float.h>

// ==================================================================================
// FUNÇÃO: quicksort (Wrapper / Gerente)
// ==================================================================================
// Esta função prepara o terreno para a recursão. Ela:
// 1. Cria uma cópia do arquivo original para não destruir os dados da fonte.
// 2. Abre os ponteiros de arquivo necessários.
// 3. Mede o tempo de execução.
// ==================================================================================
void quicksort(int quantidade, int situacao)
{
    FILE *copia;
    FILE *arquivo;
    Aluno bloco[10];

    // Ponteiros para manipular o arquivo.
    // LEs: Leitura/Escrita Superior (lado direito)
    // Li:  Leitura Inferior (lado esquerdo)
    // Ei:  Escrita Inferior (lado esquerdo)
    FILE *ArqLEs;
    FILE *ArqLi;
    FILE *ArqEi;

    Contadores conts;
    conts.comparacoes = 0;
    conts.transferencias = 0;
    clock_t inicio, fim;
    char name[50];

    // Define qual arquivo abrir com base na situação (Ascendente, Descendente, Aleatório)
    if (situacao == 1)
    {
        strcpy(name, "data/Ascendente_by_quicksort.dat");
        arquivo = fopen("data/arquivosBin/Ascendente.dat", "rb");
    }
    else if (situacao == 2)
    {
        strcpy(name, "data/Descendente_by_quicksort.dat");
        arquivo = fopen("data/arquivosBin/Descendente.dat", "rb");
    }
    else
    {
        strcpy(name, "data/Aleatorio_by_quicksort.dat");
        arquivo = fopen("data/arquivosBin/Aleatorio.dat", "rb");
    }

    // [PASSO 1] Faz a cópia física do arquivo original para um arquivo temporário de trabalho
    copia = fopen(name, "wb");
    for (int i = 0; i < quantidade; i++)
    {
        fread(bloco, sizeof(Aluno), 1, arquivo);
        fwrite(bloco, sizeof(Aluno), 1, copia);
    }
    fclose(copia);
    fclose(arquivo);

    // [PASSO 2] Abre o MESMO arquivo 3 vezes. Isso permite ter cabeçotes de leitura/escrita independentes.
    ArqLEs = fopen(name, "r+b"); // Vai manipular o final do arquivo
    ArqLi = fopen(name, "r+b");  // Vai ler do início
    ArqEi = fopen(name, "r+b");  // Vai escrever no início

    if (ArqLi == NULL || ArqEi == NULL || ArqLEs == NULL)
    {
        return;
    }

    inicio = clock();
    // Chama a função recursiva principal
    QuickSortExterno(&ArqLi, &ArqEi, &ArqLEs, 1, quantidade, &conts);

    fclose(ArqEi);
    fclose(ArqLi);
    fclose(ArqLEs);
    fim = clock();
    conts.tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;

    imprimeContadores(conts);
    geraArquivoTexto(name); // Gera txt para conferência visual
    remove(name);           // Limpa o arquivo temporário

    // Salva estatísticas globais
    ContadoresIndividuais.comparacoes = conts.comparacoes;
    ContadoresIndividuais.transferencias = conts.transferencias;
    ContadoresIndividuais.tempo = conts.tempo;
}

// ==================================================================================
// FUNÇÕES AUXILIARES DE BUFFER (PIVÔ)
// ==================================================================================
// Em vez de um único pivô, usamos um vetor (struct Pivo) que guarda até 10 elementos.
// Isso reduz o I/O, pois processamos "lotes" de pivôs.

void inicializaPivo(Pivo *p) { p->n = 0; }

// Insere um aluno no buffer de pivôs MANTENDO A ORDEM (Insertion Sort simples).
void inserePivo(Pivo *pivo, Aluno aluno, Contadores *conts)
{
    int i = pivo->n;
    // Empurra os maiores para a direita para abrir espaço
    while (i > 0)
    {
        conts->comparacoes++;
        if (aluno.nota >= pivo->vetor[i - 1].nota)
            break;
        pivo->vetor[i] = pivo->vetor[i - 1];
        i--;
    }
    pivo->vetor[i] = aluno;
    pivo->n++;
}

// Remove o MAIOR elemento do buffer (última posição)
void retiraMaiorPivo(Pivo *pivo, Aluno *aluno)
{
    *aluno = pivo->vetor[(pivo->n) - 1];
    pivo->n--;
}

// Remove o MENOR elemento do buffer (posição 0) e desloca o resto (Shift Left)
void retiraMenorPivo(Pivo *pivo, Aluno *aluno)
{
    *aluno = pivo->vetor[0];
    pivo->n--;
    for (int i = 0; i < pivo->n; i++)
        pivo->vetor[i] = pivo->vetor[i + 1];
}

// ==================================================================================
// FUNÇÃO: QuickSortExterno (Recursiva)
// ==================================================================================
void QuickSortExterno(FILE **ArqLi, FILE **ArqEi, FILE **ArqLEs, int esq, int dir, Contadores *conts)
{
    int i, j;

    // Condição de parada: se a sub-partição tem 0 ou 1 elemento, já está ordenada.
    if (dir - esq < 1)
        return;

    // Aloca o buffer de pivôs na memória RAM
    Pivo *pivo = (Pivo *)malloc(sizeof(Pivo));
    if (!pivo)
        return;

    inicializaPivo(pivo);

    // [IMPORTANTE] Chama a Partição.
    // Após essa linha, o arquivo estará dividido: [Menores ... Pivôs ... Maiores]
    // 'i' e 'j' marcarão os limites onde a partição ocorreu.
    Particao(ArqLi, ArqEi, ArqLEs, pivo, esq, dir, &i, &j, conts);

    free(pivo);

    // Otimização de Recursão:
    // Sempre chama a recursão primeiro para a MENOR metade.
    // Isso evita estouro de pilha (Stack Overflow) em arquivos muito grandes.
    if (i - esq < dir - j)
    {
        QuickSortExterno(ArqLi, ArqEi, ArqLEs, esq, i, conts); // Lado esquerdo menor
        QuickSortExterno(ArqLi, ArqEi, ArqLEs, j, dir, conts);
    }
    else
    {
        QuickSortExterno(ArqLi, ArqEi, ArqLEs, j, dir, conts); // Lado direito menor
        QuickSortExterno(ArqLi, ArqEi, ArqLEs, esq, i, conts);
    }
}

// ==================================================================================
[cite_start] // FUNÇÃO: Particao (Coração do Algoritmo) [cite: 139]
    // ==================================================================================
    // É aqui que o arquivo é lido e reescrito.
    // - Ls, Es: Limites de leitura/escrita Superior (Direita)
    // - Li, Ei: Limites de leitura/escrita Inferior (Esquerda)
    void Particao(FILE **ArqLi, FILE **ArqEi, FILE **ArqLEs, Pivo *pivo, int esq, int dir, int *i, int *j, Contadores *conts)
{
    int Ls = dir, Es = dir, Li = esq, Ei = esq;
    double Linf = -DBL_MAX, Lsup = DBL_MAX; // Sentinelas para saber o menor/maior valor já escrito

    short OndeLer = TRUE; // Flag para alternar leitura entre Início e Fim
    Aluno leitura, escrita;

    // Posiciona os ponteiros de arquivo nas posições corretas para esta etapa da recursão
    fseek(*ArqLi, (Li - 1) * sizeof(Aluno), SEEK_SET);
    fseek(*ArqEi, (Ei - 1) * sizeof(Aluno), SEEK_SET);
    *i = esq - 1;
    *j = dir + 1; // Inicializa os índices de retorno

    // LOOP PRINCIPAL: Enquanto os ponteiros de leitura (Sup e Inf) não se cruzarem
    while (Ls >= Li)
    {

        // 1. ENCHER O BUFFER: Se o buffer de pivôs não está cheio (tem menos de 10)
        if (pivo->n < 10 - 1)
        {
            conts->transferencias++;
            // Alterna a leitura: uma vez do fim (Sup), outra do início (Inf)
            if (OndeLer)
                leSup(ArqLEs, &leitura, &Ls, &OndeLer);
            else
                leInf(ArqLi, &leitura, &Li, &OndeLer);

            // Só insere no buffer, não escreve no disco ainda.
            inserePivo(pivo, leitura, conts);
            continue; // Volta pro início do loop para pegar mais um
        }

        // 2. BUFFER CHEIO: Hora de processar e decidir quem sai
        conts->transferencias++;

        // Decide de onde ler o próximo elemento "desconhecido"
        if (Ls == Es)
            leSup(ArqLEs, &leitura, &Ls, &OndeLer); // Se só sobrou espaço no fim
        else if (Li == Ei)
            leInf(ArqLi, &leitura, &Li, &OndeLer); // Se só sobrou espaço no início
        else if (OndeLer)
            leSup(ArqLEs, &leitura, &Ls, &OndeLer); // Alternância normal
        else
            leInf(ArqLi, &leitura, &Li, &OndeLer);

        conts->comparacoes = conts->comparacoes + 2;

        // 3. DECISÃO DE ESCRITA (Para onde vai o dado lido?)

        // CASO A: O dado é MAIOR que qualquer coisa já escrita na direita?
        if (leitura.nota > Lsup)
        {
            *j = Es;
            conts->transferencias++;
            escreveMax(ArqLEs, leitura, &Es); // Escreve direto no fim
            continue;
        }

        // CASO B: O dado é MENOR que qualquer coisa já escrita na esquerda?
        if (leitura.nota < Linf)
        {
            *i = Ei;
            conts->transferencias++;
            escreveMin(ArqEi, leitura, &Ei); // Escreve direto no início
            continue;
        }

        // CASO C: O dado está "no meio". Insere no buffer ordenado.
        inserePivo(pivo, leitura, conts);

        // Agora o buffer transbordou (tem 11 itens virtualmente). Precisamos tirar um.
        // HEURÍSTICA DE BALANCEAMENTO:
        // Verifica qual lado escreveu MENOS dados até agora.
        if (Ei - esq < dir - Es)
        {
            // Se o lado esquerdo está menor, tira o MENOR do buffer e manda pra lá.
            retiraMenorPivo(pivo, &escrita);
            conts->transferencias++;
            escreveMin(ArqEi, escrita, &Ei);
            Linf = escrita.nota; // Atualiza a sentinela inferior
        }
        else
        {
            // Se o lado direito está menor, tira o MAIOR do buffer e manda pra lá.
            retiraMaiorPivo(pivo, &escrita);
            conts->transferencias++;
            escreveMax(ArqLEs, escrita, &Es);
            Lsup = escrita.nota; // Atualiza a sentinela superior
        }
    }

    // 4. FINALIZAÇÃO: Esvazia o que sobrou no buffer no "meio" do arquivo
    while (Ei <= Es)
    {
        conts->transferencias++;
        retiraMenorPivo(pivo, &escrita);
        escreveMin(ArqEi, escrita, &Ei);
    }
}

// ==================================================================================
// FUNÇÕES DE I/O (Leitura e Escrita Controlada)
// ==================================================================================

// Lê do topo (final do arquivo/partição)
void leSup(FILE **ArqLEs, Aluno *leitura, int *Ls, short *OndeLer)
{
    fseek(*ArqLEs, (*Ls - 1) * sizeof(Aluno), SEEK_SET); // Posiciona
    fread(leitura, sizeof(Aluno), 1, *ArqLEs);           // Lê
    (*Ls)--;                                             // Decrementa o ponteiro lógico
    *OndeLer = FALSE;                                    // Próxima leitura será do Início (alternância)
}

// Lê do fundo (início do arquivo/partição)
void leInf(FILE **ArqLi, Aluno *leitura, int *Li, short *OndeLer)
{
    fread(leitura, sizeof(Aluno), 1, *ArqLi); // Lê sequencial
    (*Li)++;                                  // Incrementa ponteiro lógico
    *OndeLer = TRUE;                          // Próxima leitura será do Topo (alternância)
}

// Escreve no início (parte dos menores)
void escreveMin(FILE **ArqEi, Aluno escrita, int *Ei)
{
    fwrite(&escrita, sizeof(Aluno), 1, *ArqEi);
    (*Ei)++;
}

// Escreve no fim (parte dos maiores)
void escreveMax(FILE **ArqLEs, Aluno escrita, int *Es)
{
    fseek(*ArqLEs, (*Es - 1) * sizeof(Aluno), SEEK_SET);
    fwrite(&escrita, sizeof(Aluno), 1, *ArqLEs);
    (*Es)--;
}