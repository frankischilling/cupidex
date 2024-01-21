#!/bin/sh

CFLAGS='-Wall -Wextra -pedantic -Wshadow -Werror -Wstrict-overflow -fsanitize=address -fsanitize=undefined' make

echo 'COMPILED! PRESS ENTER TO CONTINUE'
read something

./cupidfetch 2> log.txt

