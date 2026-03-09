# Event Reservation Platform

**Computer Networks - 2025/2026**  
**LEIC Alameda**

Distributed event reservation system implemented in C, using the Sockets interface for network communication through the UDP and TCP protocols.

---

## Project Description

This project implements a complete event reservation platform, composed of:

- **Event-reservation Server (ES)**: Centralized server that manages users, events, and reservations
- **User Application (User)**: Interactive client for communication with the server

The system supports:
- User registration and authentication
- Event creation and management
- Reservation system with availability control
- Transfer of event description files
- Listing of events and reservations

---

## Project Structure

```text
.
├── README.md              # This file
├── Makefile               # Global Makefile (builds client and server)
├── client/                # User application (client)
│   ├── user              # Client executable
│   ├── Makefile          # Client Makefile
│   ├── README.md         # Detailed client documentation
│   ├── downloads/        # Files received from the server
│   └── src/              # Client source code
│       ├── client.c
│       ├── commands.c
│       ├── commands.h
│       ├── protocol.c
│       ├── protocol.h
│       └── constants.h
├── server/               # Event-reservation Server
│   ├── ES                # Server executable
│   ├── Makefile          # Server Makefile
│   ├── README.md         # Detailed server documentation
│   ├── USERS/            # User database
│   ├── EVENTS/           # Event database
│   └── src/              # Server source code
│       ├── server.c
│       ├── commands.c
│       ├── commands.h
│       ├── protocol.c
│       ├── protocol.h
│       ├── database.c
│       ├── database.h
│       └── constants.h
└── SCRIPTS_25-26/        # Test scripts
```

---

## Compilation

### Build everything (client and server):
```bash
make
```

### Build only the client:
```bash
make client
```

### Build only the server:
```bash
make server
```

### Clean build files:
```bash
make clean
```

### Rebuild everything:
```bash
make rebuild
```

---

## Running the Project

### 1. Start the Server

```bash
cd server
./ES [-p ESport] [-v]
```

**Optional arguments:**
- `-p ESport`: Port where the server accepts requests (default: 58053)
- `-v`: Verbose mode (prints debug information)

**Example:**
```bash
./ES -p 58053 -v
```

Or using the global Makefile:
```bash
make run-server
```

### 2. Start the Client

In another terminal:

```bash
cd client
./user [-n ESIP] [-p ESport]
```

**Optional arguments:**
- `-n ESIP`: Server IP address (default: 127.0.0.1)
- `-p ESport`: Server port (default: 58053)

**Example:**
```bash
./user -n 127.0.0.1 -p 58053
```

Or using the global Makefile:
```bash
make run-client
```

---

## Main Features

### User Management
- **Login**: Authentication or automatic registration of new users
- **Logout**: End session
- **Change Password**: Update password
- **Unregister**: Remove account

### Event Management
- **Create**: Create an event with name, date, seats, and description file
- **Close**: Close an event (stop accepting reservations)
- **List**: List all available events
- **Show**: View event details and download the description file
- **My Events**: List events created by the user

### Reservation System
- **Reserve**: Reserve seats for an event
- **My Reservations**: List the user's reservations

---

## Communication Protocols

### UDP (Fast Operations)
Used for stateless and lightweight operations:
- Login/Logout
- Unregister
- Listing the user's own events
- Listing the user's own reservations

### TCP (File Transfer)
Used for more complex operations and data transfer:
- Event creation (file upload)
- Event closing
- General event listing
- Event viewing (file download)
- Reservations
- Password change

---

## Formats and Constraints

### Identifiers
- **UID**: 6 digits (e.g. `123456`)
- **Password**: 8 alphanumeric characters (e.g. `Pass1234`)
- **EID**: 3 digits (001-999)

### Events
- **Name**: Max. 10 alphanumeric characters
- **Date/Time**: `dd-mm-yyyy hh:mm` (e.g. `25-12-2025 20:00`)
- **Seats**: 10-999 (creation) or 1-999 (reservation)

### Files
- **Name**: Max. 24 characters (including `.xxx` extension)
- **Size**: Max. 10 MB (10,000,000 bytes)
- **Allowed characters**: Alphanumeric characters, `-`, `_`, `.`

## Global Makefile Commands

| Command | Description |
|---------|-------------|
| `make` or `make all` | Builds client and server |
| `make client` | Builds only the client |
| `make server` | Builds only the server |
| `make clean` | Removes all build files |
| `make clean-client` | Removes only client build files |
| `make clean-server` | Removes only server build files |
| `make rebuild` | Cleans and rebuilds everything |
| `make run-server` | Builds and runs the server in verbose mode |
| `make run-client` | Builds and runs the client |
| `make test` | Checks whether the executables were created |
| `make help` | Shows help for the available commands |

---

## System Requirements

- **Operating System**: Linux (tested on Ubuntu/Debian)
- **Compiler**: gcc with C99/C11 support
- **Libraries**: Standard C library, POSIX sockets
- **Make**: GNU Make

---

## Additional Documentation

For detailed information about each component, see:
- **[client/README.md](client/README.md)**: Full client documentation
- **[server/README.md](server/README.md)**: Full server documentation

---

## Data Structure

### User Database (USERS/)
```text
USERS/
└── [UID]/              # Directory for each user (6 digits)
    ├── pass.txt        # User password
    ├── login.txt       # Active session indicator
    ├── CREATED/        # Links to created events
    └── RESERVED/       # Reservation files
```

### Event Database (EVENTS/)
```text
EVENTS/
└── [EID]/              # Directory for each event (001-999)
    ├── START           # Metadata (owner, name, date, seats)
    ├── RES             # Reserved seats counter
    ├── CLOSE           # Closing timestamp (if applicable)
    ├── DESCRIPTION/    # Event description file
    └── RESERVATIONS/   # Reservations by user
        └── [UID]       # Date/time and number of seats
```

---

## Technical Characteristics

### Server (ES)
- Connection multiplexing with `select()`
- Simultaneous UDP and TCP support on the same port
- Multiple simultaneous TCP connections
- Persistent filesystem-based database
- Verbose mode for debugging
- Graceful signal handling (SIGINT, SIG