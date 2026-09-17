#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <cmath>

using namespace std;

struct Movimiento {
    int fOrigen, cOrigen;
    int fDestino, cDestino;
    bool esCaptura;
    int fCapturada, cCapturada;
};

class Tablero {
public:
    char casillas[8][8];

    Tablero() {
        for (int f = 0; f < 8; f++)
            for (int c = 0; c < 8; c++)
                casillas[f][c] = '.';

        // Posición fichas blancas
        for (int f = 0; f < 3; f++)
            for (int c = 0; c < 8; c++)
                if ((f + c) % 2 == 1)
                    casillas[f][c] = 'b';

        // Posicion fichas negras
        for (int f = 5; f < 8; f++)
            for (int c = 0; c < 8; c++)
                if ((f + c) % 2 == 1)
                    casillas[f][c] = 'n';
    }

	// Limitación
    bool limiteTablero(int f, int c) const {
        return f >= 0 && f < 8 && c >= 0 && c < 8;
    }

    bool esFinal() const {
        int blancas = 0, negras = 0;
        for (int f = 0; f < 8; f++)
            for (int c = 0; c < 8; c++) {
                if (casillas[f][c] == 'b') blancas++;
                if (casillas[f][c] == 'n') negras++;
            }
        return blancas == 0 || negras == 0;
    }

	// Evaluación Estática
    int evaluar() const {
        int blancas = 0, negras = 0;
        for (int f = 0; f < 8; f++) {
            for (int c = 0; c < 8; c++) {
                if (casillas[f][c] == 'b') blancas++;
                if (casillas[f][c] == 'n') negras++;
            }
        }
        return blancas - negras;
    }

	// Movimientos posibles
    vector<pair<int, int>> direcciones(bool turnoBlancas) const {
        if (turnoBlancas) return { {1,-1}, {1,1} };
        return { {-1,-1}, {-1,1} };
    }

    void buscarCapturas(int f, int c, bool turnoBlancas, vector<Movimiento>& resultado) const {
        vector<pair<int, int>> dirs = direcciones(turnoBlancas);
        for (auto& d : dirs) {
            int fm = f + d.first, cm = c + d.second;
            int fd = f + 2 * d.first, cd = c + 2 * d.second;

            if (!limiteTablero(fd, cd)) continue;

            char piezaIntermedia = casillas[fm][cm];
            char piezaDestino = casillas[fd][cd];
            bool esEnemiga = turnoBlancas ? (piezaIntermedia == 'n') : (piezaIntermedia == 'b');

            if (esEnemiga && piezaDestino == '.') {
                Movimiento m = {f, c, fd, cd, true, fm, cm};
                resultado.push_back(m);
            }
        }
    }

    vector<Movimiento> buscarMovimientos(bool turnoBlancas) const {
        vector<Movimiento> movimientos;
        for (int f = 0; f < 8; f++) {
            for (int c = 0; c < 8; c++) {
                char pieza = casillas[f][c];
                if (pieza == '.') continue;
                bool piezaBlanca = (pieza == 'b');
                if (piezaBlanca != turnoBlancas) continue;

                for (auto& d : direcciones(turnoBlancas)) {
                    int nf = f + d.first, nc = c + d.second;
                    if (limiteTablero(nf, nc) && casillas[nf][nc] == '.') {
                        Movimiento m = {f, c, nf, nc, false, 0, 0};
                        movimientos.push_back(m);
                    }
                }
            }
        }
        return movimientos;
    }

	/*
    vector<Movimiento> movimientosFichas(bool turnoBlancas) const {
		vector<Movimiento> capturas;
		for (int f = 0; f < 8; f++) {
			for (int c = 0; c < 8; c++) {
				char pieza = casillas[f][c];
				if (pieza == '.') continue;
				if ((pieza == 'b') != turnoBlancas) continue;
				buscarCapturas(f, c, turnoBlancas, capturas);
			}
		}
		
		if (!capturas.empty()) {
			return capturas;
		}

		return buscarMovimientos(turnoBlancas);
	}*/
	
	
	vector<Movimiento> movimientosFichas(bool turnoBlancas) const {
		vector<Movimiento> todosLosMovimientos;
		for (int f = 0; f < 8; f++) {
			for (int c = 0; c < 8; c++) {
				char pieza = casillas[f][c];
				if (pieza == '.') continue;
				if ((pieza == 'b') != turnoBlancas) continue;
				buscarCapturas(f, c, turnoBlancas, todosLosMovimientos);
			}
		}
		vector<Movimiento> simples = buscarMovimientos(turnoBlancas);
		todosLosMovimientos.insert(todosLosMovimientos.end(), simples.begin(), simples.end());
		return todosLosMovimientos;
	}
	

    Tablero aplicarMovimiento(const Movimiento& mov) const {
        Tablero nuevo = *this;
        char pieza = nuevo.casillas[mov.fOrigen][mov.cOrigen];
        nuevo.casillas[mov.fOrigen][mov.cOrigen] = '.';
        nuevo.casillas[mov.fDestino][mov.cDestino] = pieza;
        if (mov.esCaptura) {
            nuevo.casillas[mov.fCapturada][mov.cCapturada] = '.';
        }
        return nuevo;
    }
};

struct NodoArbol {
    Tablero tablero;
    Movimiento movimientoPasado;
    bool turnoBlancas;
    int profundidad;
    int valor;
    bool bloqueado;
    NodoArbol* padre;
    vector<NodoArbol*> hijos;

    NodoArbol(const Tablero& t, bool turno, int prof, NodoArbol* p = nullptr)
        : tablero(t), turnoBlancas(turno), profundidad(prof), valor(0), bloqueado(false), padre(p) {}

    ~NodoArbol() {
        for (NodoArbol* h : hijos) delete h;
    }
};

void construirArbol(NodoArbol* nodo, int profundidadRestante) {
    if (profundidadRestante == 0 || nodo->tablero.esFinal()) return;
    vector<Movimiento> movimientos = nodo->tablero.movimientosFichas(nodo->turnoBlancas);
    if (movimientos.empty()) {
        nodo->bloqueado = true;
        return;
    }
    for (auto& mov : movimientos) {
        Tablero tHijo = nodo->tablero.aplicarMovimiento(mov);
        NodoArbol* hijo = new NodoArbol(tHijo, !nodo->turnoBlancas, nodo->profundidad + 1, nodo);
        hijo->movimientoPasado = mov;
        nodo->hijos.push_back(hijo);
        construirArbol(hijo, profundidadRestante - 1);
    }
}

int minimax(NodoArbol* nodo) {
    if (nodo->hijos.empty()) {
        if (nodo->bloqueado) nodo->valor = nodo->turnoBlancas ? -10 : 10;
        else nodo->valor = nodo->tablero.evaluar();
        return nodo->valor;
    }

    if (nodo->turnoBlancas) { // MAX
        int mejor = INT_MIN;
        for (NodoArbol* h : nodo->hijos) 
			mejor = max(mejor, minimax(h));
        nodo->valor = mejor;
    } else { // MIN
        int mejor = INT_MAX;
        for (NodoArbol* h : nodo->hijos) 
			mejor = min(mejor, minimax(h));
        nodo->valor = mejor;
    }
    return nodo->valor;
}

// OPENGL
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void revisarFinPartida();

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

Tablero tableroActual;
int profundidadMaxima = 5;
bool turnoBlancas = true;
bool juegoTerminado = false;
vector<Movimiento> movsUsuario;
int filaSel = -1, colSel = -1;
double timeTurnEnded = 0.0; 

unsigned int shaderProgram;
unsigned int VAO, VBO;
int colorLoc;

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 ourColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = ourColor;\n"
    "}\n\0";

void dibujarCuadrado(int f, int c, float r, float g, float b) {
    float x1 = -1.0f + (c * 0.25f);
    float x2 = x1 + 0.25f;
    float y1 = -1.0f + (f * 0.25f); 
    float y2 = y1 + 0.25f;

    float vertices[] = {
        x1, y1,  x2, y1,  x2, y2,
        x2, y2,  x1, y2,  x1, y1 
    };

    glUseProgram(shaderProgram);
    glUniform4f(colorLoc, r, g, b, 1.0f);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void dibujarCirculo(int f, int c, float r, float g, float b) {
    float cx = -1.0f + (c * 0.25f) + 0.125f;
    float cy = -1.0f + (f * 0.25f) + 0.125f;
    float radio = 0.09f;

    vector<float> vertices;
    int segmentos = 30;
    vertices.push_back(cx);
    vertices.push_back(cy);
    for(int i = 0; i <= segmentos; i++) {
        float angulo = 2.0f * 3.1415926f * float(i) / float(segmentos);
        vertices.push_back(cx + radio * cos(angulo));
        vertices.push_back(cy + radio * sin(angulo));
    }

    glUseProgram(shaderProgram);
    glUniform4f(colorLoc, r, g, b, 1.0f);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, segmentos + 2);
}

// Funciones Adicionales
void imprimirMovimiento(const Movimiento& mov) {
    cout << "(" << mov.cOrigen << "," << mov.fOrigen << ")";
    cout << " -> (" << mov.cDestino << "," << mov.fDestino << ")";
    if (mov.esCaptura)
        cout << "   [Pieza Capturada: (" << mov.cCapturada << "," << mov.fCapturada << ")]";
    cout << '\n';
}

long contarNodos(NodoArbol* nodo) {
    long total = 1;
    for (NodoArbol* h : nodo->hijos) total += contarNodos(h);
    return total;
}

void imprimirArbol(NodoArbol* nodo, string prefijo = "", bool esUltimo = true) {
    if (nodo == nullptr) return;
    cout << prefijo;
    cout << (esUltimo ? "\\-- " : "|-- ");
    if (nodo->padre == nullptr) {
        cout << "RAIZ (Valor Minimax: " << nodo->valor << ")\n";
    } else {
        // Se imprime primero la columna (X) y luego la fila (Y)
        cout << "Mov: (" << nodo->movimientoPasado.cOrigen << "," << nodo->movimientoPasado.fOrigen
             << ") -> (" << nodo->movimientoPasado.cDestino << "," << nodo->movimientoPasado.fDestino << ")";
        if (nodo->movimientoPasado.esCaptura) cout << " [Captura]";
        cout << " | Valor: " << nodo->valor;
        if (nodo->hijos.empty()) cout << " (Eval estatica)";
        cout << "\n";
    }
    for (size_t i = 0; i < nodo->hijos.size(); i++) {
        bool ultimoHijo = (i == nodo->hijos.size() - 1);
        imprimirArbol(nodo->hijos[i], prefijo + (esUltimo ? "    " : "|   "), ultimoHijo);
    }
}

void configurarPartida() {
    cout << "Profundidad del arbol minimax: ";
    while (!(cin >> profundidadMaxima) || profundidadMaxima <= 0) {
        cout << "Valor invalido: ";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    int opcion;
    cout << "\nQuien comienza la partida?\n"
         << "  1) Humano  (blancas)\n"
         << "  2) Maquina (negras)\n"
         << "Elige una opcion (1 o 2): ";
    while (!(cin >> opcion) || (opcion != 1 && opcion != 2)) {
        cout << "Opcion invalida.";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    turnoBlancas = (opcion == 1);
}

int main()
{
    srand((unsigned)time(nullptr));

    configurarPartida();

    movsUsuario = tableroActual.movimientosFichas(turnoBlancas);

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Damas OpenGL Core", NULL, NULL);
    if (window == NULL) {
        std::cout << "Fallo al crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cout << "Fallo al inicializar GLAD" << std::endl;
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    colorLoc = glGetUniformLocation(shaderProgram, "ourColor");

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 150 * sizeof(float), NULL, GL_DYNAMIC_DRAW); 

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    timeTurnEnded = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        if (!turnoBlancas && !juegoTerminado) {
            if (glfwGetTime() - timeTurnEnded > 0.2) {
                cout << "\nTurno de la maquina\n";
                NodoArbol* raiz = new NodoArbol(tableroActual, false, 0);
                construirArbol(raiz, profundidadMaxima);
                int valorRaiz = minimax(raiz);

                
                //imprimirArbol(raiz);

                if (raiz->hijos.empty()) {
                    delete raiz;
                    juegoTerminado = true;
                    cout << "\n===================================\n";
                    cout << "JUEGO TERMINADO\n";
                } else {
                    vector<NodoArbol*> mejores;
                    for (NodoArbol* h : raiz->hijos)
                        if (h->valor == valorRaiz) mejores.push_back(h);

                    NodoArbol* elegido = mejores[rand() % mejores.size()];
                    cout << "La maquina juega: ";
                    imprimirMovimiento(elegido->movimientoPasado);
                    tableroActual = elegido->tablero;
                    delete raiz;

                    turnoBlancas = true;
                    movsUsuario = tableroActual.movimientosFichas(true);
                    revisarFinPartida();
                }
            }
        }

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 1. Dibujar Tablero (Ocupa todo)
        for (int f = 0; f < 8; f++) {
            for (int c = 0; c < 8; c++) {
                if (f == filaSel && c == colSel) {
                    dibujarCuadrado(f, c, 0.8f, 0.8f, 0.2f);
                } else if ((f + c) % 2 == 0) {
                    dibujarCuadrado(f, c, 0.9f, 0.8f, 0.7f);
                } else {
                    dibujarCuadrado(f, c, 0.5f, 0.3f, 0.1f);
                }
            }
        }

        // 2. Dibujar Fichas
        for (int f = 0; f < 8; f++) {
            for (int c = 0; c < 8; c++) {
                char pieza = tableroActual.casillas[f][c];
                if (pieza == 'b') dibujarCirculo(f, c, 0.9f, 0.9f, 0.9f);
                else if (pieza == 'n') dibujarCirculo(f, c, 0.1f, 0.1f, 0.1f);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

// Callbacks
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && turnoBlancas && !juegoTerminado) {
        
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float ndcX = (xpos / (float)width) * 2.0f - 1.0f;
        float ndcY = 1.0f - (ypos / (float)height) * 2.0f;

        if (ndcX >= -1.0f && ndcX <= 1.0f && ndcY >= -1.0f && ndcY <= 1.0f) {
            
            int cClic = (int)floor((ndcX + 1.0f) / 0.25f);
            int fClic = (int)floor((ndcY + 1.0f) / 0.25f);

            if(cClic >= 8) cClic = 7;
            if(fClic >= 8) fClic = 7;

            if (filaSel == -1) { 
                if (tableroActual.casillas[fClic][cClic] == 'b') {
                    filaSel = fClic;
                    colSel = cClic;
                }
            } else { 
                bool movido = false;
                for (auto& mov : movsUsuario) {
                    if (mov.fOrigen == filaSel && mov.cOrigen == colSel && 
                        mov.fDestino == fClic && mov.cDestino == cClic) {
                        
                        tableroActual = tableroActual.aplicarMovimiento(mov);
                        turnoBlancas = false; 
                        movido = true;
                        filaSel = -1; colSel = -1;
                        
                        movsUsuario = tableroActual.movimientosFichas(false);
                        revisarFinPartida();
                        
                        timeTurnEnded = glfwGetTime(); 
                        break;
                    }
                }

                if (!movido) {
                    if (tableroActual.casillas[fClic][cClic] == 'b') {
                        filaSel = fClic; colSel = cClic;
                    } else {
                        filaSel = -1; colSel = -1;
                    }
                }
            }
        }
    }
}

void revisarFinPartida() {
    if (tableroActual.esFinal() || movsUsuario.empty()) {
        juegoTerminado = true;
        cout << "\n===================================\n";
        cout << "JUEGO TERMINADO\n";
        int blancas = 0, negras = 0;
        for (int f = 0; f < 8; f++)
            for (int c = 0; c < 8; c++) {
                if (tableroActual.casillas[f][c] == 'b') blancas++;
                if (tableroActual.casillas[f][c] == 'n') negras++;
            }
        
        if (blancas == 0 && negras == 0) cout << "Empate\n";
        else if (blancas == 0 || (turnoBlancas && movsUsuario.empty())) cout << "La maquina gana.\n";
        else cout << "El humano gana.\n";
    }
}