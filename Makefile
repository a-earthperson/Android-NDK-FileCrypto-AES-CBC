# Android-NDK-FileCrypto-AES-CBC
# Arjun Arjun
# July 2016

CC = gcc
CFLAGS = -O3 -Wall -Wextra -Wno-unused -Werror
LDLIBS = -lm

DISTDIR = Android-NDK-FileCrypto-AES-CBC

all: encrypt

SRC = openssl/aes_core.c openssl/aes_cbc.c openssl/cbc128.c encrypt.c
OBJ = $(subst .c,.o,$(SRC))

DIST_SOURCES = Makefile README.md openssl/ encrypt.c LICENSE

encrypt: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

dist: $(DISTDIR).tar.gz

$(DISTDIR).tar.gz: $(DIST_SOURCES)
	@rm -rf $(DISTDIR)
	@export GZIP=-9
	@tar -cvzf $@.tmp $(DIST_SOURCES)
	@mv $@.tmp $@
	@echo "tarball created :: $@"

clean:
	rm -rf *.log */*.o *.o *~ *.bak *.tar.gz core *.core encrypt *.tmp $(DISTDIR)

.PHONY: all dist check clean
