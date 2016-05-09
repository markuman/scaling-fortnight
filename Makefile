CC= gcc

build: ## compile scaling-fortnight
	@echo "comping scaling-fortnight"
	@$(CC) -pthread dyad/src/dyad.c -lhiredis -llua -O2 main.c -o sf

live: ## run live from source with tcc
	@tcc -pthread -lhiredis -llua dyad/src/dyad.c -run main.c


.PHONY: help

help:
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.DEFAULT_GOAL := help

