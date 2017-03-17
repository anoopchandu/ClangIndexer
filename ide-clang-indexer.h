#ifndef IDE_CLANG_INDEXER_H
#define IDE_CLANG_INDEXER_H

#include <glib-object.h>
#include "ide-clang-index.h"

#define IDE_TYPE_CLANG_INDEXER (ide_clang_indexer_get_type ())

G_DECLARE_FINAL_TYPE (IdeClangIndexer, ide_clang_indexer, IDE, CLANG_INDEXER, GObject)

void           ide_clang_indexer_start           (IdeClangIndexer *self);
IdeClangIndex* ide_clang_indexer_get_index       (IdeClangIndexer *self);
void           ide_clang_indexer_insert_command  (IdeClangIndexer *self,
                                                  const gchar *source_file,
                                                  const gchar *command_line_args[],
                                                  gint32 num_args);
#endif