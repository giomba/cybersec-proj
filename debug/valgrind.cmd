valgrind --leak-check=full --track-origins=yes --gen-suppressions=all --log-file=debug/valgrind.log --suppressions=debug/valgrind.supp $COMMAND
