#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>
#include <cstdlib>
#include <ctime>

// Classe JogoDaVelha
class JogoDaVelha {
private:
    std::array<std::array<char, 3>, 3> tabuleiro; // Matriz do jogo
    std::mutex mutex_tabuleiro; // Mutex para controle de acesso ao tabuleiro
    std::condition_variable cv_turno; // Variável de condição para alternância de turnos
    char jogador_atual; // Jogador atual ('X' ou 'O')
    bool jogo_terminado; // Indica se o jogo acabou
    char vencedor; // Vencedor do jogo

public:
    JogoDaVelha() : jogador_atual('X'), jogo_terminado(false), vencedor(' ') {
        // Inicializa o tabuleiro com espaços vazios
        for (auto& linha : tabuleiro) {
            linha.fill(' ');
        }
    }

    void mostrar_tabuleiro() {
        std::cout << "\nEstado atual do tabuleiro:\n";
        for (int i = 0; i < 3; i++) {
            std::cout << " ";
            for (int j = 0; j < 3; j++) {
                std::cout << tabuleiro[i][j];
                if (j < 2) std::cout << " | ";
            }
            std::cout << "\n";
            if (i < 2) std::cout << "---+---+---\n";
        }
        std::cout << std::endl;
    }

    bool fazer_jogada(char jogador, int linha, int coluna) {
        std::unique_lock<std::mutex> lock(mutex_tabuleiro);

        // Espera até que seja a vez do jogador ou o jogo tenha terminado
        cv_turno.wait(lock, [this, jogador] { return jogador_atual == jogador || jogo_terminado; });

        // Se o jogo terminou, sai da função
        if (jogo_terminado) {
            return false;
        }

        if (tabuleiro[linha][coluna] == ' ') {
            tabuleiro[linha][coluna] = jogador;
            mostrar_tabuleiro();

            // Verifica vitória ou empate
            if (verificar_vitoria(jogador)) {
                jogo_terminado = true;
                vencedor = jogador;
            } else if (verificar_empate()) {
                jogo_terminado = true;
                vencedor = 'E'; // 'E' para empate
            } else {
                // Alterna para o próximo jogador
                jogador_atual = (jogador == 'X') ? 'O' : 'X';
            }

            // Notifica os outros jogadores
            cv_turno.notify_all();
            return true;
        }

        // Se a posição não está disponível, notifica e retorna false
        cv_turno.notify_all();
        return false;
    }

    bool verificar_vitoria(char jogador) {
        // Verifica linhas e colunas
        for (int i = 0; i < 3; i++) {
            if ((tabuleiro[i][0] == jogador && tabuleiro[i][1] == jogador && tabuleiro[i][2] == jogador) ||
                (tabuleiro[0][i] == jogador && tabuleiro[1][i] == jogador && tabuleiro[2][i] == jogador)) {
                return true;
            }
        }

        // Verifica diagonais
        if ((tabuleiro[0][0] == jogador && tabuleiro[1][1] == jogador && tabuleiro[2][2] == jogador) ||
            (tabuleiro[0][2] == jogador && tabuleiro[1][1] == jogador && tabuleiro[2][0] == jogador)) {
            return true;
        }

        return false;
    }

    bool verificar_empate() {
        // Se não há espaços vazios e ninguém venceu, é empate
        for (const auto& linha : tabuleiro) {
            for (const auto& celula : linha) {
                if (celula == ' ') return false;
            }
        }
        return true;
    }

    bool jogo_acabou() {
        std::unique_lock<std::mutex> lock(mutex_tabuleiro);
        return jogo_terminado;
    }

    char obter_vencedor() {
        return vencedor;
    }
};

// Classe Jogador
class Jogador {
private:
    JogoDaVelha& jogo; // Referência para o jogo
    char simbolo; // Símbolo do jogador ('X' ou 'O')
    std::string estrategia; // Estratégia do jogador

public:
    Jogador(JogoDaVelha& j, char s, const std::string& estrat)
        : jogo(j), simbolo(s), estrategia(estrat) {
        std::srand(static_cast<unsigned int>(std::time(nullptr))); // Inicializa a semente do gerador aleatório
    }

    void jogar() {
        while (!jogo.jogo_acabou()) {
            if (estrategia == "sequencial") {
                jogar_sequencial();
            } else {
                jogar_aleatorio();
            }

            // Pequena pausa para melhorar a legibilidade da saída
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    void jogar_sequencial() {
        // Percorre o tabuleiro e faz a primeira jogada possível
        for (int linha = 0; linha < 3; linha++) {
            for (int coluna = 0; coluna < 3; coluna++) {
                if (jogo.fazer_jogada(simbolo, linha, coluna)) {
                    return; // Sai da função após realizar a jogada
                }
            }
        }
    }

    void jogar_aleatorio() {
        // Tenta fazer uma jogada em posições aleatórias
        int linha, coluna;
        while (!jogo.jogo_acabou()) {
            linha = std::rand() % 3;
            coluna = std::rand() % 3;
            if (jogo.fazer_jogada(simbolo, linha, coluna)) {
                return; // Sai da função após realizar a jogada
            }
        }
    }
};

// Função principal
int main() {
    // Inicializa o jogo
    JogoDaVelha jogo;

    // Inicializa os jogadores
    Jogador jogador1(jogo, 'X', "sequencial");
    Jogador jogador2(jogo, 'O', "aleatorio");

    // Cria as threads para os jogadores
    std::thread thread1(&Jogador::jogar, &jogador1);
    std::thread thread2(&Jogador::jogar, &jogador2);

    // Aguarda o término das threads
    thread1.join();
    thread2.join();

    // Exibe o resultado final do jogo
    std::cout << "Fim do jogo! ";
    char vencedor = jogo.obter_vencedor();
    if (vencedor == 'E') {
        std::cout << "O jogo terminou em empate!" << std::endl;
    } else {
        std::cout << "O jogador " << vencedor << " venceu!" << std::endl;
    }

    return 0;
}
