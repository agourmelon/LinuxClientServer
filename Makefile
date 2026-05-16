CC      = gcc
CFLAGS  = -Wall -Wextra -g

# Dossier source (ex: make VERSION=version1)
VERSION ?= version1

SRC_DIR   = $(VERSION)
BUILD_DIR = build/$(VERSION)

SERVER_SRC = $(SRC_DIR)/server.c
CLIENT_SRC = $(SRC_DIR)/client.c

SERVER_BIN = $(BUILD_DIR)/server
CLIENT_BIN = $(BUILD_DIR)/client

# ─────────────────────────────────────────────
.PHONY: all clean help

all: $(SERVER_BIN) $(CLIENT_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SERVER_BIN): $(SERVER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT_BIN): $(CLIENT_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build/$(VERSION)

help:
	@echo "Usage:"
	@echo "  make VERSION=<dossier>   Compile server et client depuis <dossier>/"
	@echo "  make clean VERSION=<dossier>   Supprime build/<dossier>"
	@echo ""
	@echo "Exemple:"
	@echo "  make VERSION=version1"
