GOBJECT = `pkg-config --cflags --libs gobject-2.0`
CLANG = -I/usr/lib/llvm-3.8/include -L/usr/lib/llvm-3.8/lib -lclang

indexer : main.c ide-clang-indexer.c ide-clang-ast-indexer.c ide-clang-index.c ide-clang-file-index.c
	gcc -g main.c ide-clang-indexer.c ide-clang-ast-indexer.c ide-clang-index.c ide-clang-file-index.c $(GOBJECT) $(CLANG) -o $@
