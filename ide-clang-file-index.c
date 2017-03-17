/* represents index of a single file */

#include <glib.h>

#include "ide-clang-file-index.h"
#include "ide-clang-ast-indexer.h"

struct _IdeClangFileIndex
{
  GObject parent;
  GHashTable *declarations;
  GHashTable *local_declarations;
  GHashTable *references;
};

G_DEFINE_TYPE (IdeClangFileIndex, ide_clang_file_index, G_TYPE_OBJECT)

/* Structures to store declarations and refereneces */
/* flag 0 represents it is not a declration*/
struct _Declaration
{
  guint32 file_id;
  gint32 declaration_id;
  gchar *USR;
  guint32 line, start_column;
  guint32 type;
};

// to save space negative value of declaration_id represents a local
struct _Reference
{
  guint32 line;
  guint32 start_column, end_column;
  guint32 declaration_file_id;
  gint32 declaration_id;
  guint32 type;
};

typedef struct _Declaration Declaration;
typedef struct _Reference Reference;

guint
hash_declaration (gconstpointer key)
{
  const Declaration *declaration = key;

  return g_str_hash (declaration->USR);
  
}

gboolean
equal_declarations (gconstpointer a, gconstpointer b)
{
  const Declaration *declarationa = a;
  const Declaration *declarationb = b;

  return (&declarationa == &declarationb);
}

guint
hash_reference (gconstpointer key)
{
  const Reference *reference = key;
  guint32 sum;

  sum = reference->line +
        reference->start_column + reference->end_column +
        reference->declaration_id + reference->declaration_file_id;

  return g_int_hash (&sum);
}

gboolean
equal_references (gconstpointer a, gconstpointer b)
{
  const Reference *referencea = a;
  const Reference *referenceb = b;

  return (&referencea == &referenceb);
}

/* This don't check for previous existence it should be checked by ASTIndexer */
void 
ide_clang_file_index_record_declaration (IdeClangFileIndex *self,
                                         const gchar *USR,
                                         guint32 line, guint32 start_column,
                                         guint32 type, gboolean local,
                                         guint32 file_id, gint32 *declaration_id)
{
  Declaration *declaration;

  if (local)
    *declaration_id = -1*(g_hash_table_size (self->local_declarations) + 1);
  else
    *declaration_id = g_hash_table_size (self->declarations) + 1;

  declaration = g_slice_new (Declaration);

  declaration->file_id = file_id;
  declaration->declaration_id = *declaration_id;

  declaration->line = line;
  declaration->start_column = start_column;
  // create a copy of string
  declaration->USR = g_strdup (USR);
  declaration->type = type;

  if (local)
    g_hash_table_add (self->local_declarations, declaration);
  else
    g_hash_table_add (self->declarations, declaration);
}

void 
ide_clang_file_index_record_reference (IdeClangFileIndex *self,
                                       guint32 line, 
                                       guint32 start_column, guint32 end_column, guint32 type,
                                       guint32 declaration_file_id, gint32 declaration_id)
{
  Reference *reference;

  reference = g_slice_new (Reference);
  reference->line = line;
  reference->start_column = start_column;
  reference->end_column = end_column;
  reference->type = type;
  reference->declaration_file_id = declaration_file_id;
  reference->declaration_id = declaration_id;

  g_hash_table_add (self->references, reference);
}

void     
ide_clang_file_index_get_declaration_referenced (IdeClangFileIndex *self,
                                                 guint32 line, guint32 column,
                                                 guint32 *reference_type,
                                                 guint32 *declaration_file_id, gint32 *declaration_id)
{
  GHashTableIter iter;
  gpointer key, value;
  Reference *reference;

  g_hash_table_iter_init (&iter, self->references);

  *declaration_file_id = 0;
  *declaration_id = 0;

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      reference = key;
      if (line == reference->line && reference->start_column <= column && reference->end_column>=column)
        {
          *reference_type = reference->type;
          *declaration_file_id = reference->declaration_file_id;
          *declaration_id = reference->declaration_id;
          return;
        }
    }
}

gchar*
ide_clang_file_index_get_declaration_by_id (IdeClangFileIndex *self,
                                            gint32 declaration_id,
                                            guint32 *line, guint32 *column, guint32 *type)
{
  GHashTableIter iter;
  gpointer key, value;
  Declaration *declaration;

  if (declaration_id < 0)
    g_hash_table_iter_init (&iter, self->local_declarations);
  else
    g_hash_table_iter_init (&iter, self->declarations);


  *type = 0;
  *line = 0;
  *column = 0;

  while (g_hash_table_iter_next (&iter, &key, &value))
  {
    declaration = key;
    if (declaration->declaration_id == declaration_id)
    {
      *line = declaration->line;
      *column = declaration->start_column;
      *type = declaration->type;
      return declaration->USR;
    }
  }
}

void
ide_clang_file_index_get_definition_by_USR (IdeClangFileIndex *self,
                                            gchar *USR,
                                            guint32 *line, guint32 *column)
{
  GHashTableIter iter;
  gpointer key, value;
  Declaration *declaration;

  *line = 0;
  *column = 0;
  g_hash_table_iter_init (&iter, self->declarations);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      declaration = key;
      if (declaration->type != RT_Definition)
        continue;
      if (g_strcmp0 (USR, declaration->USR) == 0)
        {
          *line = declaration->line;
          *column = declaration->start_column;
        }
    }
}

static void
ide_clang_file_index_init (IdeClangFileIndex *self)
{
  self->declarations = g_hash_table_new (hash_declaration, equal_declarations);
  self->local_declarations = g_hash_table_new (hash_declaration, equal_declarations);
  self->references = g_hash_table_new (hash_reference, equal_references);
}

static void
ide_clang_file_index_class_init (IdeClangFileIndexClass *klass)
{

}