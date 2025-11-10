FROM ubuntu:22.04

# Install build tools, runtime deps and nmap
RUN apt-get update && apt-get install -y \
    build-essential gcc make pkg-config libsqlite3-dev nmap ca-certificates \
    libpq-dev wget curl git && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN mkdir -p bin data

# Build the server binary
RUN gcc -Iinclude -o bin/nmap_server \
    src/main.c src/server.c src/scanner.c src/db.c src/utils.c src/sqlite3.c \
    -pthread -ldl

# Expose the port the app listens on (server uses 8080 by default)
EXPOSE 8080

# Default command
CMD ["./bin/nmap_server"]
