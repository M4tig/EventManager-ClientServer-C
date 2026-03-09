# Event Reservation Server (ES)

**Redes de Computadores - 2025/2026**  
**LEIC Alameda**

Este é o servidor para a plataforma de reserva de eventos, desenvolvido em C para suportar múltiplas ligações simultâneas de utilizadores através dos protocolos UDP e TCP.

---

## Descrição

O Event-reservation Server (ES) é responsável por:
- Gestão de utilizadores (registo, login, logout, mudança de password, unregister)
- Gestão de eventos (criação, fecho, listagem)
- Sistema de reservas (criação e listagem de reservas)
- Armazenamento persistente de dados em sistema de ficheiros
- Suporte simultâneo de protocolos UDP (gestão rápida) e TCP (transferência de ficheiros)

O servidor opera num porto bem conhecido (default: 58053) e pode funcionar em modo verbose para debug.

---

## Estrutura de Ficheiros

```
server/
├── ES                      # Executável do servidor (após compilação)
├── Makefile               # Script de compilação
├── README.md              # Este ficheiro
├── src/
│   ├── server.c          # Main loop do servidor, gestão de sockets UDP/TCP
│   ├── commands.c        # Handlers para todos os comandos (UDP e TCP)
│   ├── commands.h        # Interface dos handlers
│   ├── protocol.c        # Funções de comunicação de rede
│   ├── protocol.h        # Interface de protocolo
│   ├── database.c        # Operações de base de dados (filesystem)
│   ├── database.h        # Interface de base de dados
│   └── constants.h       # Constantes globais e definições
├── USERS/                # Diretório de utilizadores (criado automaticamente)
│   └── [UID]/           # Um diretório por utilizador (6 dígitos)
│       ├── pass.txt     # Password do utilizador
│       ├── login.txt    # Ficheiro indicador de login (existe quando logged in)
│       ├── CREATED/     # Eventos criados pelo utilizador
│       └── RESERVED/    # Eventos com reservas do utilizador
└── EVENTS/              # Diretório de eventos (criado automaticamente)
    └── [EID]/          # Um diretório por evento (001-999)
        ├── START       # Informação do evento (owner, nome, data, seats)
        ├── CLOSE       # Timestamp de fecho (se aplicável)
        ├── RES         # Número total de lugares reservados
        ├── DESCRIPTION/
        │   └── [file] # Ficheiro descritivo do evento
        └── RESERVATIONS/
            └── [UID]  # Ficheiro de reserva (data/hora e número de lugares)
```

---

## Compilação

```bash
make
```

Isto cria o executável `ES` no diretório atual.

Para limpar ficheiros compilados:
```bash
make clean
```

---

## Execução

```bash
./ES [-p ESport] [-v]
```

### Argumentos Opcionais:

- **`-p ESport`**: Porta onde o servidor aceita pedidos (UDP e TCP). Default: `58053` (58000 + 53, onde 53 é o número do grupo)
- **`-v`**: Modo verbose. Imprime informação de debug sobre cada pedido recebido (tipo, UID, IP, porto)

### Exemplo:

```bash
./ES                    # Usa porta 58053, modo normal
./ES -p 59000          # Usa porta 59000, modo normal
./ES -v                # Usa porta 58053, modo verbose
./ES -p 59000 -v       # Usa porta 59000, modo verbose
```

---

## Funcionalidades

O servidor processa dois tipos de protocolos simultaneamente:

### **Protocolo UDP** (Operações rápidas de gestão)
Suporta os seguintes comandos:

#### 1. **Login (LIN)**
- **Recebe:** `LIN UID password\n`
- **Responde:** `RLI status\n`
  - `OK` - Login bem-sucedido (utilizador existente)
  - `REG` - Novo utilizador registado
  - `NOK` - Password incorreta
  - `ERR` - Erro de sintaxe ou valores inválidos

#### 2. **Logout (LOU)**
- **Recebe:** `LOU UID password\n`
- **Responde:** `RLO status\n`
  - `OK` - Logout bem-sucedido
  - `NOK` - Utilizador não estava logged in
  - `UNR` - Utilizador não registado
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

#### 3. **Unregister (UNR)**
- **Recebe:** `UNR UID password\n`
- **Responde:** `RUR status\n`
  - `OK` - Utilizador removido com sucesso
  - `NOK` - Utilizador não estava logged in
  - `UNR` - Utilizador não registado
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

#### 4. **My Events (LME)**
- **Recebe:** `LME UID password\n`
- **Responde:** `RME status[ EID state]*\n`
  - `OK` - Lista de eventos (EID e state para cada evento)
  - `NOK` - Utilizador não criou eventos
  - `NLG` - Utilizador não está logged in
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe
- **state**: `1` (ativo), `0` (passado), `2` (sold out), `3` (fechado pelo owner)

#### 5. **My Reservations (LMR)**
- **Recebe:** `LMR UID password\n`
- **Responde:** `RMR status[ EID date value]*\n`
  - `OK` - Lista de reservas (EID, data/hora, número de lugares)
  - `NOK` - Utilizador não fez reservas
  - `NLG` - Utilizador não está logged in
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe
- Máximo de 50 reservas (as 50 mais recentes)

---

### **Protocolo TCP** (Transferência de ficheiros e operações complexas)
Suporta os seguintes comandos:

#### 6. **Create Event (CRE)**
- **Recebe:** `CRE UID password name event_date attendance Fname Fsize Fdata\n`
  - `name`: Máx. 10 caracteres alfanuméricos
  - `event_date`: Formato `dd-mm-yyyy hh:mm`
  - `attendance`: 10-999 (número de lugares)
  - `Fname`: Máx. 24 caracteres (incluindo `.xxx`)
  - `Fsize`: Máx. 10 MB (10.000.000 bytes)
  - `Fdata`: Conteúdo do ficheiro
- **Responde:** `RCE status [EID]\n`
  - `OK EID` - Evento criado com sucesso (retorna EID de 001-999)
  - `NOK` - Falha na criação
  - `NLG` - Utilizador não está logged in
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe ou valores inválidos

#### 7. **Close Event (CLS)**
- **Recebe:** `CLS UID password EID\n`
- **Responde:** `RCL status\n`
  - `OK` - Evento fechado com sucesso
  - `NOK` - Utilizador não existe ou password incorreta
  - `NLG` - Utilizador não está logged in
  - `NOE` - Evento não existe
  - `EOW` - Evento não pertence ao utilizador
  - `SLD` - Evento já está sold out
  - `PST` - Evento já passou (data no passado)
  - `CLO` - Evento já estava fechado
  - `ERR` - Erro de sintaxe

#### 8. **List All Events (LST)**
- **Recebe:** `LST\n`
- **Responde:** `RLS status[ EID name state event_date]*\n`
  - `OK` - Lista de todos os eventos (EID, nome, state, data)
  - `NOK` - Nenhum evento criado
  - `ERR` - Erro de sintaxe
- **state**: `1` (ativo), `0` (passado), `2` (sold out), `3` (fechado)

#### 9. **Show Event Details (SED)**
- **Recebe:** `SED EID\n`
- **Responde:** `RSE status [UID name event_date attendance_size Seats_reserved Fname Fsize Fdata]\n`
  - `OK` - Detalhes do evento + ficheiro descritivo
  - `NOK` - Evento não existe
  - `ERR` - Erro de sintaxe

#### 10. **Reserve Seats (RID)**
- **Recebe:** `RID UID password EID people\n`
  - `people`: 1-999 (número de lugares a reservar)
- **Responde:** `RRI status [n_seats]\n`
  - `ACC` - Reserva aceite
  - `REJ n_seats` - Reserva rejeitada (lugares insuficientes, retorna lugares disponíveis)
  - `NOK` - Evento não ativo
  - `NLG` - Utilizador não está logged in
  - `CLS` - Evento fechado
  - `SLD` - Evento sold out
  - `PST` - Evento já passou
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

#### 11. **Change Password (CPS)**
- **Recebe:** `CPS UID oldPassword newPassword\n`
- **Responde:** `RCP status\n`
  - `OK` - Password alterada com sucesso
  - `NOK` - Password antiga incorreta
  - `NLG` - Utilizador não está logged in
  - `NID` - Utilizador não existe
  - `ERR` - Erro de sintaxe

---

## Formatos e Restrições

### **Identificadores:**
- **UID**: Exatamente 6 dígitos (ex: `123456`)
- **Password**: Exatamente 8 caracteres alfanuméricos (ex: `Pass1234`)
- **EID**: 3 dígitos (001-999)

### **Eventos:**
- **Nome**: Máx. 10 caracteres alfanuméricos
- **Data**: `dd-mm-yyyy` (ex: `25-12-2025`)
- **Hora**: `hh:mm` (ex: `14:30`)
- **Data/Hora completa**: `dd-mm-yyyy hh:mm`
- **Lugares**: 10-999

### **Ficheiros:**
- **Nome**: Máx. 24 caracteres (incluindo `.xxx`), formato `nome.ext`
- **Tamanho máx.**: 10 MB (10.000.000 bytes)
- **Extensão**: 3 letras após o ponto

### **Estados de Evento:**
- `1` - Evento ativo (no futuro, com lugares disponíveis)
- `0` - Evento no passado (data já passou)
- `2` - Evento sold out (no futuro mas sem lugares)
- `3` - Evento fechado pelo owner (não aceita mais reservas)

---

## Arquitetura

### **Multiplexação de Conexões**
O servidor usa `select()` para monitorizar simultaneamente:
- Socket UDP (pedidos LIN, LOU, UNR, LME, LMR)
- Socket TCP listener (aceita novas conexões)
- Múltiplas conexões TCP ativas (até FD_SETSIZE)

### **Base de Dados (Filesystem)**
Todos os dados são persistidos no sistema de ficheiros:
- **USERS/**: Diretório raiz de utilizadores
  - Cada utilizador tem subdiretório com UID
  - `pass.txt`: password do utilizador
  - `login.txt`: indicador de sessão ativa
  - `CREATED/`: links para eventos criados
  - `RESERVED/`: ficheiros de reservas

- **EVENTS/**: Diretório raiz de eventos
  - Cada evento tem subdiretório com EID
  - `START`: metadata do evento
  - `RES`: contador de lugares reservados
  - `CLOSE`: timestamp de fecho (se aplicável)
  - `DESCRIPTION/`: ficheiro descritivo
  - `RESERVATIONS/`: ficheiros de reserva por UID

### **Gestão de Erros**
O servidor não termina abruptamente:
- Mensagens de protocolo inválidas retornam `ERR`
- Erros de sistema são tratados graciosamente
- Signal SIGPIPE é ignorado (evita crash em TCP write)
- SIGINT (Ctrl+C) termina servidor graciosamente


## Dependências

- **Compilador**: gcc com suporte para C99/POSIX
- **Sistema Operativo**: Linux (usa POSIX APIs)
- **Bibliotecas**: Standard C library, POSIX sockets


## Autores

**João Agostinho** - ist1109324  
**Martim Afonso** - ist1106507