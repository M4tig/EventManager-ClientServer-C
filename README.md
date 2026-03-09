# Event Reservation Platform

**Redes de Computadores - 2025/2026**  
**LEIC Alameda**

Sistema distribuído de reserva de eventos implementado em C, utilizando a interface de Sockets para comunicação em rede através dos protocolos UDP e TCP.

---

## Descrição do Projeto

Este projeto implementa uma plataforma completa de reserva de eventos, composta por:

- **Event-reservation Server (ES)**: Servidor centralizado que gere utilizadores, eventos e reservas
- **User Application (User)**: Cliente interativo para interação com o servidor

O sistema permite:
- Registo e autenticação de utilizadores
- Criação e gestão de eventos
- Sistema de reservas com controlo de disponibilidade
- Transferência de ficheiros descritivos de eventos
- Listagem de eventos e reservas

---

## Estrutura do Projeto

```
.
├── README.md              # Este ficheiro
├── Makefile               # Makefile global (compila client e server)
├── client/                # Aplicação User (cliente)
│   ├── user              # Executável do cliente
│   ├── Makefile          # Makefile do cliente
│   ├── README.md         # Documentação detalhada do cliente
│   ├── downloads/        # Ficheiros recebidos do servidor
│   └── src/              # Código fonte do cliente
│       ├── client.c
│       ├── commands.c
│       ├── commands.h
│       ├── protocol.c
│       ├── protocol.h
│       └── constants.h
├── server/               # Event-reservation Server
│   ├── ES                # Executável do servidor
│   ├── Makefile          # Makefile do servidor
│   ├── README.md         # Documentação detalhada do servidor
│   ├── USERS/            # Base de dados de utilizadores
│   ├── EVENTS/           # Base de dados de eventos
│   └── src/              # Código fonte do servidor
│       ├── server.c
│       ├── commands.c
│       ├── commands.h
│       ├── protocol.c
│       ├── protocol.h
│       ├── database.c
│       ├── database.h
│       └── constants.h
└── SCRIPTS_25-26/        # Scripts de teste
```

---

## Compilação

### Compilar tudo (cliente e servidor):
```bash
make
```

### Compilar apenas o cliente:
```bash
make client
```

### Compilar apenas o servidor:
```bash
make server
```

### Limpar ficheiros de compilação:
```bash
make clean
```

### Recompilar tudo:
```bash
make rebuild
```

---

## Execução

### 1. Iniciar o Servidor

```bash
cd server
./ES [-p ESport] [-v]
```

**Argumentos opcionais:**
- `-p ESport`: Porto onde o servidor aceita pedidos (default: 58053)
- `-v`: Modo verbose (imprime informação de debug)

**Exemplo:**
```bash
./ES -p 58053 -v
```

Ou usando o Makefile global:
```bash
make run-server
```

### 2. Iniciar o Cliente

Em outro terminal:

```bash
cd client
./user [-n ESIP] [-p ESport]
```

**Argumentos opcionais:**
- `-n ESIP`: IP do servidor (default: 127.0.0.1)
- `-p ESport`: Porto do servidor (default: 58053)

**Exemplo:**
```bash
./user -n 127.0.0.1 -p 58053
```

Ou usando o Makefile global:
```bash
make run-client
```

---

## Funcionalidades Principais

### Gestão de Utilizadores
- **Login**: Autenticação ou registo automático de novos utilizadores
- **Logout**: Terminar sessão
- **Change Password**: Alterar password
- **Unregister**: Remover conta

### Gestão de Eventos
- **Create**: Criar evento com nome, data, lugares e ficheiro descritivo
- **Close**: Fechar evento (parar aceitação de reservas)
- **List**: Listar todos os eventos disponíveis
- **Show**: Ver detalhes de um evento e transferir ficheiro descritivo
- **My Events**: Listar eventos criados pelo utilizador

### Sistema de Reservas
- **Reserve**: Fazer reserva de lugares num evento
- **My Reservations**: Listar reservas do utilizador

---

## Protocolos de Comunicação

### UDP (Operações Rápidas)
Usado para operações stateless e rápidas:
- Login/Logout
- Unregister
- Listagem de eventos próprios
- Listagem de reservas próprias

### TCP (Transferência de Ficheiros)
Usado para operações complexas e transferência de dados:
- Criação de eventos (upload de ficheiros)
- Fecho de eventos
- Listagem geral de eventos
- Visualização de eventos (download de ficheiros)
- Reservas
- Alteração de password

---

## Formatos e Restrições

### Identificadores
- **UID**: 6 dígitos (ex: `123456`)
- **Password**: 8 caracteres alfanuméricos (ex: `Pass1234`)
- **EID**: 3 dígitos (001-999)

### Eventos
- **Nome**: Máx. 10 caracteres alfanuméricos
- **Data/Hora**: `dd-mm-yyyy hh:mm` (ex: `25-12-2025 20:00`)
- **Lugares**: 10-999 (criação) ou 1-999 (reserva)

### Ficheiros
- **Nome**: Máx. 24 caracteres (incluindo extensão `.xxx`)
- **Tamanho**: Máx. 10 MB (10.000.000 bytes)
- **Caracteres permitidos**: Alfanuméricos, `-`, `_`, `.`


## Comandos do Makefile Global

| Comando | Descrição |
|---------|-----------|
| `make` ou `make all` | Compila cliente e servidor |
| `make client` | Compila apenas o cliente |
| `make server` | Compila apenas o servidor |
| `make clean` | Remove todos os ficheiros de compilação |
| `make clean-client` | Remove apenas ficheiros do cliente |
| `make clean-server` | Remove apenas ficheiros do servidor |
| `make rebuild` | Limpa e recompila tudo |
| `make run-server` | Compila e executa o servidor em modo verbose |
| `make run-client` | Compila e executa o cliente |
| `make test` | Verifica se os executáveis foram criados |
| `make help` | Mostra ajuda dos comandos disponíveis |

---

## Requisitos do Sistema

- **Sistema Operativo**: Linux (testado em Ubuntu/Debian)
- **Compilador**: gcc com suporte para C99/C11
- **Bibliotecas**: Standard C library, POSIX sockets
- **Make**: GNU Make

---

## Documentação Adicional

Para informações detalhadas sobre cada componente, consulte:
- **[client/README.md](client/README.md)**: Documentação completa do cliente
- **[server/README.md](server/README.md)**: Documentação completa do servidor

---

## Estrutura de Dados

### Base de Dados de Utilizadores (USERS/)
```
USERS/
└── [UID]/              # Diretório por utilizador (6 dígitos)
    ├── pass.txt        # Password do utilizador
    ├── login.txt       # Indicador de sessão ativa
    ├── CREATED/        # Links para eventos criados
    └── RESERVED/       # Ficheiros de reservas
```

### Base de Dados de Eventos (EVENTS/)
```
EVENTS/
└── [EID]/              # Diretório por evento (001-999)
    ├── START           # Metadata (owner, nome, data, lugares)
    ├── RES             # Contador de lugares reservados
    ├── CLOSE           # Timestamp de fecho (se aplicável)
    ├── DESCRIPTION/    # Ficheiro descritivo do evento
    └── RESERVATIONS/   # Reservas por utilizador
        └── [UID]       # Data/hora e número de lugares
```

---

## Características Técnicas

### Servidor (ES)
- Multiplexação de conexões com `select()`
- Suporte simultâneo de UDP e TCP no mesmo porto
- Múltiplas conexões TCP simultâneas
- Base de dados persistente em filesystem
- Modo verbose para debug
- Gestão graceful de sinais (SIGINT, SIGPIPE)

### Cliente (User)
- Interface interativa por linha de comandos
- Validação local de inputs
- Gestão de estado de sessão
- Upload/download de ficheiros
- Comandos com aliases (`myevents`/`mye`, etc.)
- Mensagens de erro claras e informativas



## Autores

**João Agostinho** - ist1109324  
**Martim Afonso** - ist1106507