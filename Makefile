# gcc, clang or tcc should all work
CC= gcc
HIREDIS= /usr/include/hiredis/

build: ## compile scaling-fortnight – options=default: CC=gcc, HIREDIS=/usr/include/hiredis/
	@echo "comping scaling-fortnight"
	@$(CC) -Idyad/src/ -I $(HIREDIS) -lhiredis -llua dyad/src/dyad.c -O2 main.c -o sf

live: ## run live from source with tcc
	@tcc -Idyad/src/ -I /usr/include/hiredis/ -lhiredis -llua dyad/src/dyad.c -run main.c

.PHONY: help

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.DEFAULT_GOAL := help

