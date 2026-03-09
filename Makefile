# Event Reservation Platform - Global Makefile
# Redes de Computadores 2025/2026
# LEIC Alameda

.PHONY: all client server clean clean-client clean-server run-server run-client help rebuild test

# Default target: build both client and server
all: client server

# Build client application
client:
	@echo "========================================"
	@echo "Building Client (User Application)..."
	@echo "========================================"
	@$(MAKE) -C client
	@echo ""

# Build server application
server:
	@echo "========================================"
	@echo "Building Server (ES)..."
	@echo "========================================"
	@$(MAKE) -C server
	@echo ""

# Clean all build artifacts
clean: clean-client clean-server
	@echo "========================================"
	@echo "Global clean complete"
	@echo "========================================"

# Clean client build artifacts
clean-client:
	@echo "Cleaning client..."
	@$(MAKE) -C client clean

# Clean server build artifacts
clean-server:
	@echo "Cleaning server..."
	@$(MAKE) -C server clean

# Rebuild everything
rebuild: clean all

# Run server (default port 58053)
run-server: server
	@echo "========================================"
	@echo "Starting Event Server (ES)..."
	@echo "========================================"
	@cd server && ./ES -v

# Run client (connects to localhost:58053)
run-client: client
	@echo "========================================"
	@echo "Starting User Application..."
	@echo "========================================"
	@cd client && ./user

# Run server in background
server-bg: server
	@echo "Starting Event Server in background..."
	@cd server && ./ES -v &
	@echo "Server started (PID: $$!)"

# Test: build both and run basic checks
test: all
	@echo "========================================"
	@echo "Running basic checks..."
	@echo "========================================"
	@test -f server/ES && echo "✓ Server executable exists" || echo "✗ Server build failed"
	@test -f client/user && echo "✓ Client executable exists" || echo "✗ Client build failed"
	@echo ""

# Display help information
help:
	@echo "Event Reservation Platform - Makefile"
	@echo "======================================"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build both client and server (default)"
	@echo "  client       - Build only the client application"
	@echo "  server       - Build only the server application"
	@echo "  clean        - Remove all build artifacts"
	@echo "  clean-client - Remove only client build artifacts"
	@echo "  clean-server - Remove only server build artifacts"
	@echo "  rebuild      - Clean and rebuild everything"
	@echo "  run-server   - Build and run the server in verbose mode"
	@echo "  run-client   - Build and run the client"
	@echo "  server-bg    - Build and run server in background"
	@echo "  test         - Build and verify executables"
	@echo "  help         - Display this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build everything"
	@echo "  make clean        # Clean all builds"
	@echo "  make run-server   # Start the server"
	@echo "  make run-client   # Start the client"
	@echo ""
