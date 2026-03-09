# Event Reservation Client (User)

**Redes de Computadores - 2025/2026**  
**LEIC Alameda**

Este é o cliente para a plataforma de reserva de eventos, desenvolvido em C para interagir com o Event-reservation Server (ES) através dos protocolos UDP e TCP.

---

## Descrição

O User Application permite que utilizadores:
- Gerir autenticação (login, logout, change password, unregister)
- Criar e fechar eventos próprios
- Listar todos os eventos disponíveis ou apenas os eventos próprios
- Fazer reservas de lugares em eventos
- Consultar detalhes de eventos e transferir ficheiros descritivos
- Listar reservas próprias

O cliente conecta-se ao ES usando um IP e porto especificados (ou valores default) e executa comandos interativos através da linha de comandos.

---

## Estrutura de Ficheiros

```
client/
├── user                   # Executável do cliente (após compilação)
├── Makefile              # Script de compilação
├── README.md             # Este ficheiro
├── downloads/            # Diretório onde ficheiros recebidos são guardados
├── src/
│   ├── client.c         # Main loop, parsing de comandos, gestão de input
│   ├── commands.c       # Handlers para todos os comandos (UDP e TCP)
│   ├── commands.h       # Interface dos handlers
│   ├── protocol.c       # Funções de comunicação de rede
│   ├── protocol.h       # Interface de protocolo
│   └── constants.h      # Constantes globais e definições
└── obj/                 # Ficheiros objeto (criado durante compilação)
```

---

## Compilação

```bash
make
```

Isto cria o executável `user` no diretório atual.

Para limpar ficheiros compilados:
```bash
make clean
```

---

## Execução

```bash
./user [-n ESIP] [-p ESport]
```

### Argumentos Opcionais:

- **`-n ESIP`**: Endereço IP da máquina onde o Event-reservation Server (ES) está a correr. Default: `127.0.0.1` (localhost)
- **`-p ESport`**: Porto onde o ES aceita pedidos (UDP e TCP). Default: `58053` (58000 + 53, onde 53 é o número do grupo)

### Exemplos:

```bash
./user                           # Conecta a localhost:58053
./user -n 192.168.1.10          # Conecta a 192.168.1.10:58053
./user -p 59000                 # Conecta a localhost:59000
./user -n 192.168.1.10 -p 59000 # Conecta a 192.168.1.10:59000
```

---

## Funcionalidades

O cliente comunica com o ES usando dois protocolos:

### **Protocolo UDP** (Operações rápidas de gestão)

#### 1. **Login (login)**
- **Sintaxe:** `login UID password`
- **Descrição:** Autentica um utilizador no servidor. Se o UID não existir, regista um novo utilizador.
- **Envia:** `LIN UID password\n`
- **Recebe:** `RLI status\n`
  - `OK` - Login bem-sucedido (utilizador existente)
  - `REG` - Novo utilizador registado
  - `NOK` - Password incorreta
  - `ERR` - Erro de sintaxe ou valores inválidos

#### 2. **Logout (logout)**
- **Sintaxe:** `logout`
- **Descrição:** Termina a sessão do utilizador atualmente logged in.
- **Envia:** `LOU UID password\n`
- **Recebe:** `RLO status\n`
  - `OK` - Logout bem-sucedido
  - `NOK` - Utilizador não estava logged in
  - `UNR` - Utilizador não registado
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

#### 3. **Unregister (unregister)**
- **Sintaxe:** `unregister`
- **Descrição:** Remove a conta do utilizador atualmente logged in. Efetua logout automaticamente.
- **Envia:** `UNR UID password\n`
- **Recebe:** `RUR status\n`
  - `OK` - Utilizador removido com sucesso
  - `NOK` - Utilizador não estava logged in
  - `UNR` - Utilizador não registado
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

#### 4. **My Events (myevents / mye)**
- **Sintaxe:** `myevents` ou `mye`
- **Descrição:** Lista os eventos criados pelo utilizador logged in.
- **Envia:** `LME UID password\n`
- **Recebe:** `RME status[ EID state]*\n`
  - `OK` - Lista de eventos (EID e state para cada evento)
  - `NOK` - Utilizador não criou eventos
  - `NLG` - Utilizador não está logged in
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe
- **state**: `1` (ativo), `0` (passado), `2` (sold out), `3` (fechado pelo owner)

#### 5. **My Reservations (myreservations / myr)**
- **Sintaxe:** `myreservations` ou `myr`
- **Descrição:** Lista as reservas feitas pelo utilizador logged in (máximo 50 reservas mais recentes).
- **Envia:** `LMR UID password\n`
- **Recebe:** `RMR status[ EID date value]*\n`
  - `OK` - Lista de reservas (EID, data/hora, número de lugares)
  - `NOK` - Utilizador não fez reservas
  - `NLG` - Utilizador não está logged in
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

---

### **Protocolo TCP** (Transferência de ficheiros e operações complexas)

#### 6. **Create Event (create)**
- **Sintaxe:** `create name event_fname event_date event_time num_attendees`
- **Descrição:** Cria um novo evento, enviando um ficheiro descritivo ao servidor.
  - `name`: Nome do evento (máx. 10 caracteres alfanuméricos)
  - `event_fname`: Caminho para o ficheiro local que descreve o evento
  - `event_date`: Data no formato `dd-mm-yyyy`
  - `event_time`: Hora no formato `hh:mm`
  - `num_attendees`: Número de lugares (10-999)
- **Envia:** `CRE UID password name event_date attendance_size Fname Fsize Fdata\n`
- **Recebe:** `RCE status [EID]\n`
  - `OK EID` - Evento criado com sucesso (retorna EID de 001-999)
  - `NOK` - Falha na criação
  - `NLG` - Utilizador não está logged in
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe ou valores inválidos
- **Nota:** O ficheiro é lido localmente e enviado ao servidor. Tamanho máximo: 10 MB.

#### 7. **Close Event (close)**
- **Sintaxe:** `close EID`
- **Descrição:** Fecha um evento criado pelo utilizador, impedindo novas reservas.
- **Envia:** `CLS UID password EID\n`
- **Recebe:** `RCL status\n`
  - `OK` - Evento fechado com sucesso
  - `NOK` - Utilizador não existe ou password incorreta
  - `NLG` - Utilizador não está logged in
  - `NOE` - Evento não existe
  - `EOW` - Evento não pertence ao utilizador
  - `SLD` - Evento já está sold out
  - `PST` - Evento já passou (data no passado)
  - `CLO` - Evento já estava fechado
  - `ERR` - Erro de sintaxe

#### 8. **List All Events (list)**
- **Sintaxe:** `list`
- **Descrição:** Lista todos os eventos disponíveis no servidor.
- **Envia:** `LST\n`
- **Recebe:** `RLS status[ EID name state event_date]*\n`
  - `OK` - Lista de todos os eventos (EID, nome, state, data)
  - `NOK` - Nenhum evento criado
  - `ERR` - Erro de sintaxe
- **state**: `1` (ativo), `0` (passado), `2` (sold out), `3` (fechado)

#### 9. **Show Event Details (show)**
- **Sintaxe:** `show EID`
- **Descrição:** Mostra detalhes de um evento e transfere o ficheiro descritivo.
- **Envia:** `SED EID\n`
- **Recebe:** `RSE status [UID name event_date attendance_size Seats_reserved Fname Fsize Fdata]\n`
  - `OK` - Detalhes do evento + ficheiro descritivo
  - `NOK` - Evento não existe
  - `ERR` - Erro de sintaxe
- **Nota:** O ficheiro é guardado na pasta `downloads/` do cliente.

#### 10. **Reserve Seats (reserve)**
- **Sintaxe:** `reserve EID value`
- **Descrição:** Faz uma reserva de lugares num evento.
  - `EID`: Identificador do evento (001-999)
  - `value`: Número de lugares a reservar (1-999)
- **Envia:** `RID UID password EID people\n`
- **Recebe:** `RRI status [n_seats]\n`
  - `ACC` - Reserva aceite
  - `REJ n_seats` - Reserva rejeitada (lugares insuficientes, retorna lugares disponíveis)
  - `NOK` - Evento não ativo
  - `NLG` - Utilizador não está logged in
  - `CLS` - Evento fechado
  - `SLD` - Evento sold out
  - `PST` - Evento já passou
  - `WRP` - Password incorreta
  - `ERR` - Erro de sintaxe

#### 11. **Change Password (changePass)**
- **Sintaxe:** `changePass oldPassword newPassword`
- **Descrição:** Altera a password do utilizador logged in.
- **Envia:** `CPS UID oldPassword newPassword\n`
- **Recebe:** `RCP status\n`
  - `OK` - Password alterada com sucesso
  - `NOK` - Password antiga incorreta
  - `NLG` - Utilizador não está logged in
  - `NID` - Utilizador não existe
  - `ERR` - Erro de sintaxe

---

### **Comando Local**

#### 12. **Exit (exit)**
- **Sintaxe:** `exit`
- **Descrição:** Termina a aplicação User.
- **Nota:** Se um utilizador estiver logged in, o cliente avisa que deve executar `logout` primeiro.
- **Não envia mensagem ao servidor** - é um comando local.

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
- **Lugares**: 10-999 (para criação) ou 1-999 (para reserva)

### **Ficheiros:**
- **Nome**: Máx. 24 caracteres (incluindo `.xxx`), formato `nome.ext`
- **Tamanho máx.**: 10 MB (10.000.000 bytes)
- **Extensão**: 3 letras após o ponto
- **Caracteres permitidos**: Alfanuméricos, `-`, `_`, `.`

### **Estados de Evento:**
- `1` - Evento ativo (no futuro, com lugares disponíveis)
- `0` - Evento no passado (data já passou)
- `2` - Evento sold out (no futuro mas sem lugares)
- `3` - Evento fechado pelo owner (não aceita mais reservas)

---

## Arquitetura

### **Gestão de Sessão**
- O cliente mantém estado local da sessão (UID e password do utilizador logged in)
- Cada comando que requer autenticação envia automaticamente as credenciais
- O logout limpa o estado local

### **Comunicação de Rede**
- **UDP**: Para comandos rápidos e stateless (login, logout, listas)
- **TCP**: Para transferência de ficheiros e operações complexas
  - Cada operação TCP abre uma nova conexão
  - A conexão é fechada após receber a resposta do servidor

### **Gestão de Ficheiros**
- **Upload** (create): Lê ficheiro local e envia ao servidor
- **Download** (show): Recebe ficheiro do servidor e guarda em `downloads/`
- Validação de tamanho (máx. 10 MB) antes do envio
- Validação de nome de ficheiro (formato, extensão, caracteres)

### **Validação de Input**
O cliente valida localmente:
- Formato de UID (6 dígitos)
- Formato de password (8 alfanuméricos)
- Formato de EID (3 dígitos, range 001-999)
- Formato de datas (dd-mm-yyyy) e horas (hh:mm)
- Range de valores (attendance, reservations)
- Formato de nomes de eventos e ficheiros

### **Gestão de Erros**
- Comandos inválidos são rejeitados localmente com mensagem de ajuda
- Erros de rede são reportados ao utilizador
- Respostas `ERR` do servidor são interpretadas e explicadas
- O cliente não termina abruptamente em caso de erro


## Dependências

- **Compilador**: gcc com suporte para C99/POSIX
- **Sistema Operativo**: Linux (usa POSIX APIs)
- **Bibliotecas**: Standard C library, POSIX sockets

---

## Notas de Desenvolvimento

1. **Robustez de I/O**: Funções `read()` e `write()` podem processar menos bytes que solicitado. O cliente usa `read_all()` e `write_all()` para garantir transferência completa.

2. **Validação Rigorosa**: Todos os inputs são validados antes de envio:
   - Formatos (UID 6 dígitos, password 8 alfanum, etc.)
   - Ranges (attendance 10-999, people 1-999, file size ≤ 10MB)
   - Datas no futuro (para criar eventos)
   - Existência de ficheiros locais (antes de upload)

3. **User Experience**:
   - Mensagens claras e informativas
   - Validação de input antes de comunicar com servidor
   - Comandos com aliases (`myevents`/`mye`, `myreservations`/`myr`)
   - Comandos case-insensitive

4. **Segurança**:
   - Passwords enviadas em cada operação crítica
   - Estado de sessão mantido localmente
   - Aviso ao utilizador se tentar sair sem logout

---

## Autores

**João Agostinho** - ist1109324  
**Martim Afonso** - ist1106507