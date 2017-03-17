/* Generated AST and gives that AST to IdeClangASTIndexer */

#include <clang-c/Index.h>
#include <stdio.h>
#include "ide-clang-indexer.h"
#include "ide-clang-ast-indexer.h"
#include "ide-clang-index.h"

struct _IdeClangIndexer
{
  GObject parent;
  IdeClangASTIndexer *ast_indexer;
  IdeClangIndex *index;
  GList *compilation_commands;
};

struct CompilationCommand
{
  const gchar *source_file;
  const gchar ** command_line_args;
  guint32 num_args;
};
typedef struct CompilationCommand CompilationCommand;

G_DEFINE_TYPE (IdeClangIndexer, ide_clang_indexer, G_TYPE_OBJECT)


void
ide_clang_indexer_start (IdeClangIndexer *self)
{

/* 
 * this should get all compilation commands and index those automatically
 */

  CXIndex index;
  CompilationCommand *command;

  g_assert (IDE_IS_CLANG_INDEXER (self));

  index = clang_createIndex (1, 1);

  for (GList *list = self->compilation_commands; list != NULL; list = g_list_next (list))
    {
      CXTranslationUnit tu;

      command = (CompilationCommand*) list->data;

  // printf ("Enter to parse\n");
  // getchar ();
      clang_parseTranslationUnit2 (index,
                                   command->source_file, 
                                   command->command_line_args, command->num_args,
                                   NULL, 0,
                                   // 0,
                                   CXTranslationUnit_DetailedPreprocessingRecord,
                                   &tu);

      if (tu == NULL)
        {
          g_print ("cannot create translation unit for %s\n", command->source_file);
        }
  //         printf ("Enter to index\n");
  // getchar ();
      ide_clang_ast_indexer_index (self->ast_indexer, &tu, self->index);
      clang_disposeTranslationUnit (tu);
    }

  clang_disposeIndex (index);
}

void
ide_clang_indexer_insert_command (IdeClangIndexer *self,
                                  const gchar *source_file,
                                  const gchar* *command_line_args,
                                  gint num_args)
{
  /* This doesn't copy commands*/

  g_assert (IDE_IS_CLANG_INDEXER (self));

  CompilationCommand *compilation_command;

  compilation_command = g_slice_new (CompilationCommand);

  compilation_command->source_file = source_file;
  compilation_command->command_line_args = command_line_args;
  compilation_command->num_args = num_args;

  self->compilation_commands = g_list_prepend (self->compilation_commands,
                                               compilation_command);
}

IdeClangIndex*
ide_clang_indexer_get_index (IdeClangIndexer *self)
{
  g_assert (IDE_IS_CLANG_INDEXER (self));

  return self->index;
}

static void
ide_clang_indexer_init (IdeClangIndexer *self)
{
  g_assert (IDE_IS_CLANG_INDEXER (self));

  self->index = g_object_new (IDE_TYPE_CLANG_INDEX, NULL);

  self->ast_indexer = g_object_new (IDE_TYPE_CLANG_AST_INDEXER, NULL);

  self->compilation_commands = NULL;
}

static void
ide_clang_indexer_class_init (IdeClangIndexerClass *klass)
{

}