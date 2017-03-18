#ifndef IDE_CLANG_AST_INDEXER_H
#define IDE_CLANG_AST_INDEXER_H

#include <glib-object.h>
#include <clang-c/Index.h>
#include "ide-clang-index.h"
#define IDE_TYPE_CLANG_AST_INDEXER (ide_clang_ast_indexer_get_type ())

G_DECLARE_FINAL_TYPE (IdeClangASTIndexer, ide_clang_ast_indexer, IDE, CLANG_AST_INDEXER, GObject)

enum RefType
{
  RT_Declaration,
  RT_Definition,
  RT_Reference,
  RT_Local
};

void
ide_clang_ast_indexer_index (IdeClangASTIndexer *self,
                             CXTranslationUnit tu,
                             IdeClangIndex *index);

#endif