# CHIP-8 Emulator

![Language](https://img.shields.io/badge/language-C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Library](https://img.shields.io/badge/library-SDL2-FF3D00?style=for-the-badge&logo=sdl&logoColor=white)

Um emulador **CHIP-8** robusto escrito em C puro utilizando a biblioteca **SDL2** para renderização gráfica e áudio. Este projeto foca em emulação precisa, performance e recursos extras visuais como interpolação de cores (lerping).

---

## Funcionalidades

* **Emulação do Core:** Suporte completo ao set de instruções original do CHIP-8.
* **Vídeo:** Renderização acelerada por hardware via SDL2 com suporte a scaling.
* **Áudio:** Sintetizador de onda quadrada (Square Wave) gerado matematicamente em tempo real.
* **Efeitos Visuais:** *Color Lerping* configurável para suavização de transição de pixels (ghosting).
* **Save States:** Sistema de Salvar/Carregar estado da máquina (`F5`/`F9`). 
* **Debug/Controle:** Pausa, Reset e ajuste de volume em tempo real.
* **Compatibilidade:** Tratamento de quirks (diferenças de comportamento) entre CHIP-8 original e implementações modernas (Shift, Load/Store).

---

## Screenshots
<img width="1282" height="681" alt="image" src="https://github.com/user-attachments/assets/653417dd-eae5-4cda-80d9-97af984f8f80" />

---

## Instalação e Compilação

### Pré-requisitos

Você precisará de:

 - Compilador (gcc ou clang)

 - Meson e Ninja (sistema de build)

 - Biblioteca SDL2.

**Debian/Ubuntu:**

```bash
sudo apt-get install build-essential meson ninja libsdl2-dev
```

**Compilando**

```bash
meson setup build/
ninja -C build/
Você precisará do compilador `gcc`, `make` e da biblioteca de desenvolvimento da `SDL2`.
Procure o equivalente desse comando em sua distribuição


```bash
sudo apt-get install build-essential libsdl2-dev meson ninja-build
```

##TODO: Como compilar com meson

**Utilização**

```bash
./build/src/tracua-chip8 'ROM_DESEJADA'
```
