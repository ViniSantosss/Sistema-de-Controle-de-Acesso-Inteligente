# Sistema de Controle de Acesso Inteligente

Sistema de controle de acesso baseado em **ESP32**, desenvolvido para a disciplina de **Performance em Sistemas Ciberfísicos** da PUCPR.

O projeto tem como objetivo desenvolver um sistema capaz de controlar e registrar acessos utilizando comunicação sem fio, autenticação por credencial NFC e, posteriormente, acionamento de uma fechadura elétrica.

> **Status atual:** início do desenvolvimento. A estrutura do projeto já foi criada e a implementação do firmware ainda está começando.

---

## 1. Objetivo

Desenvolver um sistema de controle de acesso que substitua o uso convencional de chaves físicas por um mecanismo de autenticação digital.

O sistema deverá permitir, ao longo do desenvolvimento:

* Cadastro de usuários;
* Associação de uma credencial NFC a cada usuário;
* Ativação e desativação de usuários;
* Definição de acessos permanentes ou temporários;
* Autenticação através de NFC;
* Liberação automática da fechadura quando o acesso for autorizado;
* Negação de acessos não autorizados, usuários desativados ou credenciais expiradas;
* Registro dos acessos realizados;
* Comunicação com servidor/API;
* Integração com banco de dados;
* Interface web administrativa;
* Notificações através de um bot do Telegram;
* Funcionamento básico em modo offline e posterior sincronização.

Essas funcionalidades fazem parte do escopo proposto para o projeto da disciplina.

---

# 2. Tecnologias

## Hardware

### Atualmente disponível

* ESP32 DevKit V1 / ESP32-WROOM-32

### Hardware previsto no projeto

* ESP32
* PN532 — leitor NFC
* Relé
* Fechadura elétrica de 12 V

> **Importante:** os componentes acima, com exceção do ESP32, ainda não estão disponíveis neste estágio do desenvolvimento. O código não deve assumir que esses componentes já estão conectados ou disponíveis para testes.

---

## Software

* C++
* Arduino Framework / ESP32
* PlatformIO
* HTML
* CSS
* JavaScript
* LittleFS
* Wi-Fi
* HTTP
* API REST
* Banco de dados
* Telegram Bot API
* Git / GitHub

Tecnologias adicionais poderão ser incorporadas conforme o projeto evoluir.

---

# 3. Estrutura do projeto

A estrutura inicial do projeto é:

```
sistema-controle-acesso/
│
├── src/
│   └── main.cpp
│
├── include/
│
├── data/
│   ├── index.html
│   ├── css/
│   │   └── style.css
│   └── js/
│       └── app.js
│
├── platformio.ini
├── .gitignore
└── README.md
```

## Responsabilidade de cada diretório

### `src/`

Contém o firmware C++ executado pelo ESP32.

Inicialmente haverá apenas:

```
src/main.cpp
```

Conforme o projeto crescer, o código poderá ser dividido em módulos de acordo com responsabilidades reais.

Não criar diversos arquivos vazios apenas para aumentar a organização aparente do projeto.

---

### `include/`

Local destinado a arquivos de cabeçalho (`.h` / `.hpp`) quando a divisão do firmware em módulos se tornar necessária.

---

### `data/`

Contém os arquivos da interface web que serão armazenados na memória flash do ESP32 através do LittleFS.

```
data/
├── index.html
├── css/
│   └── style.css
└── js/
    └── app.js
```

---

### `platformio.ini`

Arquivo de configuração do projeto PlatformIO.

Define informações como:

* placa utilizada;
* framework;
* monitor serial;
* bibliotecas;
* configurações de compilação.

---

### `.gitignore`

Deve impedir que arquivos locais, temporários, credenciais e outros arquivos que não devem ser versionados sejam enviados ao GitHub.

**Nunca colocar senhas de Wi-Fi, tokens de bots ou outras credenciais reais no repositório.**

---

# 4. Arquitetura inicial

A primeira versão do sistema será deliberadamente simples.

```
                    ESP32
                      │
             ┌────────┴────────┐
             │                 │
           Wi-Fi           Web Server
                               │
                             HTTP
                               │
                         ┌─────▼─────┐
                         │  LittleFS │
                         └─────┬─────┘
                               │
                  ┌────────────┼────────────┐
                  │            │            │
              index.html     CSS           JS
```

O objetivo inicial é fazer o ESP32:

1. iniciar corretamente;
2. conectar à rede Wi-Fi;
3. executar um servidor HTTP;
4. disponibilizar a interface web;
5. disponibilizar endpoints de API.

---

# 5. Estratégia de desenvolvimento

O desenvolvimento deve seguir uma evolução incremental.

## Etapa 1 — Firmware básico

Primeiro deve ser validado que:

* o ESP32 está funcionando;
* o firmware compila;
* o firmware é enviado corretamente;
* o Serial Monitor funciona.

---

## Etapa 2 — Wi-Fi

O ESP32 deverá possuir uma estratégia de conexão Wi-Fi.

A solução definitiva ainda está em avaliação.

Uma possibilidade considerada é utilizar dois modos:

### STA

O ESP32 conecta-se a uma rede Wi-Fi existente:

```
ESP32 ──────► Roteador ──────► Internet
```

### AP

O próprio ESP32 cria uma rede Wi-Fi:

```
Celular/PC ──────► ESP32
```

O modo AP poderá ser utilizado para uma página inicial de configuração da rede Wi-Fi.

### Possível fluxo

```
ESP32 inicia
     │
     ▼
Possui configuração Wi-Fi?
     │
 ┌───┴───┐
 NÃO     SIM
 │        │
 ▼        ▼
AP      STA
 │        │
 ▼        ▼
Página  Conecta
de       à rede
config.    │
 │         │
 └────┬────┘
      ▼
Sistema normal
```

> Esta arquitetura ainda é uma decisão em desenvolvimento. Não assumir que ela está definitivamente implementada.

---

# 6. Servidor Web

Depois da conexão de rede, o ESP32 deverá executar um servidor HTTP.

Inicialmente, os principais recursos serão:

```
GET /
GET /css/style.css
GET /js/app.js
```

Posteriormente serão adicionados endpoints de API, por exemplo:

```
GET /api/status
```

Exemplo de resposta:

```json
{
    "status": "online"
}
```

A API deverá evoluir conforme as funcionalidades do sistema forem implementadas.

---

# 7. Interface Web

A interface web será inicialmente armazenada no LittleFS do ESP32.

Fluxo:

```
Navegador
    │
    │ HTTP
    ▼
  ESP32
    │
    ▼
 LittleFS
    │
    ├── index.html
    ├── style.css
    └── app.js
```

A interface deverá posteriormente evoluir para uma interface administrativa do sistema.

---

# 8. API e servidor externo

No sistema completo, o ESP32 deverá comunicar-se com uma API/servidor.

A arquitetura prevista é:

```
             Internet
                 │
        ┌────────▼────────┐
        │   API / Server  │
        │                 │
        │    Database     │
        └────────┬────────┘
                 │
              Internet
                 │
        ┌────────▼────────┐
        │      ESP32      │
        └────────┬────────┘
                 │
          ┌──────┴──────┐
          │             │
         NFC         Fechadura
```

O ESP32 deverá ser capaz de enviar e receber informações através da API.

Não será adotada, neste momento, a estratégia de simplesmente expor diretamente o ESP32 à internet através de port forwarding.

---

# 9. Controle de acesso

Quando o hardware estiver disponível, o fluxo esperado será aproximadamente:

´´´´´´
Usuário
   │
   ▼
Credencial NFC
   │
   ▼
ESP32 / PN532
   │
   ▼
Verificação de autorização
   │
   ├───────────────┐
   │               │
AUTORIZADO       NEGADO
   │               │
   ▼               ▼
Libera           Mantém
fechadura        fechada
   │
   ▼
Registra acesso
```

O sistema deverá considerar:

* usuário cadastrado;
* usuário ativo/inativo;
* validade da autorização;
* período de acesso;
* credencial NFC.

---

# 10. Offline e sincronização

O escopo do projeto prevê um modo offline básico.

Em uma etapa posterior, o ESP32 deverá conseguir continuar realizando determinadas operações localmente quando não houver comunicação com o servidor.

Quando a conexão for restabelecida:

```
Modo offline
     │
     ▼
Dados armazenados localmente
     │
     │ conexão restabelecida
     ▼
Sincronização
     │
     ▼
Servidor
```

A estratégia de armazenamento e sincronização ainda será definida durante o desenvolvimento.

---

# 11. FreeRTOS e processamento paralelo

A disciplina aborda:

* paralelismo;
* concorrência;
* comunicação entre processos e threads;
* coordenação;
* utilização dos recursos de hardware.

O projeto prevê posteriormente a utilização dessas técnicas no ESP32.

Uma possível arquitetura futura é separar responsabilidades como:

```text
ESP32
│
├── Tarefa de rede/comunicação
│   ├── Wi-Fi
│   ├── API
│   ├── sincronização
│   └── Telegram
│
└── Tarefa de controle de acesso
    ├── NFC
    ├── autorização local
    └── controle da fechadura
```

A divisão entre os núcleos do ESP32 e a utilização de FreeRTOS deverão ser implementadas quando houver uma necessidade concreta e quando forem pertinentes aos conteúdos da disciplina.

**Não adicionar FreeRTOS ou paralelismo artificialmente na primeira versão apenas para aumentar a complexidade.**

---

# 12. Ordem de implementação

A ordem inicial planejada é:

```text
[1] Firmware básico
        ↓
[2] Serial Monitor
        ↓
[3] Wi-Fi
        ↓
[4] Web Server
        ↓
[5] LittleFS
        ↓
[6] Interface HTML/CSS/JS
        ↓
[7] API local
        ↓
[8] Configuração de Wi-Fi
        ↓
[9] API/servidor externo
        ↓
[10] Banco de dados
        ↓
[11] NFC
        ↓
[12] Controle da fechadura
        ↓
[13] Registro de acessos
        ↓
[14] Telegram
        ↓
[15] Modo offline/sincronização
        ↓
[16] FreeRTOS / concorrência / paralelismo
        ↓
[17] Otimizações e métricas
```

Essa ordem pode ser alterada conforme as necessidades do projeto.

---

# 13. Métricas e desempenho

O projeto deverá posteriormente considerar métricas relacionadas ao desempenho do sistema, incluindo:

* utilização de CPU;
* utilização de RAM;
* tempo de resposta;
* utilização de memória Flash.

Essas métricas serão utilizadas principalmente nas etapas relacionadas ao desempenho e à utilização de processamento paralelo/concurrente.

---

# 14. Segurança

Credenciais e informações privadas **não devem ser versionadas**.

Nunca colocar diretamente no GitHub:

```text
SSID real
Senha do Wi-Fi
Token do Telegram
Chaves de API
Senhas de banco de dados
Credenciais de servidor
```

Quando necessário, utilizar arquivos locais de configuração e versões de exemplo sem informações reais.

---

# 15. Regras para desenvolvimento

## Para agentes de IA e colaboradores

Antes de modificar o projeto:

1. Ler este README.
2. Verificar a estrutura atual do projeto.
3. Verificar o código existente antes de criar novos arquivos.
4. Não substituir uma arquitetura existente sem explicar a necessidade.
5. Não criar arquivos ou abstrações sem responsabilidade clara.
6. Não adicionar bibliotecas sem necessidade.
7. Não adicionar FreeRTOS, tarefas ou paralelismo apenas por estética.
8. Não inserir credenciais reais no código.
9. Não assumir que componentes de hardware ainda não disponíveis estão conectados.
10. Fazer alterações incrementais e fáceis de testar.

### Princípio principal

> **Primeiro fazer funcionar. Depois organizar. Depois otimizar.**

O projeto deve evitar complexidade prematura.

---

# 16. Estado atual

### Concluído

* [x] Estrutura inicial do projeto
* [x] Diretórios `src`, `include` e `data`
* [x] Estrutura inicial da interface web
* [x] Configuração inicial do projeto PlatformIO

### Em desenvolvimento

* [ ] Primeiro firmware
* [ ] Teste do ESP32
* [ ] Comunicação serial
* [ ] Conexão Wi-Fi
* [ ] Web Server
* [ ] LittleFS
* [ ] Interface web
* [ ] API

### Futuro

* [ ] Configuração de Wi-Fi através de página web
* [ ] Servidor/API externo
* [ ] Banco de dados
* [ ] PN532
* [ ] Autenticação NFC
* [ ] Relé
* [ ] Fechadura elétrica
* [ ] Controle de usuários
* [ ] Controle de acessos temporários
* [ ] Logs
* [ ] Telegram
* [ ] Modo offline
* [ ] Sincronização
* [ ] FreeRTOS
* [ ] Comunicação entre tarefas
* [ ] Paralelismo
* [ ] Métricas de desempenho

---

# 17. Contexto acadêmico

Projeto desenvolvido para a disciplina:

**Performance em Sistemas Ciberfísicos — PUCPR**

O projeto deve, além de cumprir as funcionalidades propostas, permitir a aplicação prática dos conceitos trabalhados na disciplina, especialmente:

* sistemas ciberfísicos;
* processamento paralelo;
* concorrência;
* comunicação entre tarefas;
* coordenação;
* utilização dos recursos de hardware;
* análise de desempenho.

O desenvolvimento deve priorizar uma implementação funcional e incremental, utilizando os conceitos de desempenho e concorrência de maneira justificável.
