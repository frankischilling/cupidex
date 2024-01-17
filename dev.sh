#!/bin/sh

CFLAGS='-Wall -Wextra -pedantic -Wshadow -Werror -Wstrict-overflow -fsanitize=address -fsanitize=undefined' make

