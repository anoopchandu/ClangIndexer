/* This maintains IdeClanFileIndex for each file and inserts data into then and searches
 * those indexes.
 */

#include <glib.h>
#include <stdio.h>
#include "ide-clang-index.h"
#include "ide-clang-file-index.h"
#include "ide-clang-ast-indexer.h"

struct _IdeClangIndex
{
  GObject parent;
  GHashTable *file_indexes;
};

G_DEFINE_TYPE (IdeClangIndex, ide_clang_index, G_TYPE_OBJECT)

struct _FileIndex
{
  guint32 file_id;
  IdeClangFileIndex *index;
};
typedef struct _FileIndex FileIndex;

guint
hashFileIndex (gconstpointer key)
{
  const gchar *file_name = key;

  return g_str_hash (file_name);
}

gboolean
equalFileIndex (gconstpointer a, gconstpointer b)
{
  const gchar *file_name_a = a;
  const gchar *file_name_b = b;

  return g_strcmp0 (file_name_a, file_name_b);
}

void
ide_clang_index_record_declaration (IdeClangIndex *self,
                                    const gchar *file_name, const gchar *unique_string,
                                    guint32 line, guint32 start_column,
                                    guint32 type, gboolean local,
                                    guint32 *file_id, gint32 *declaration_id)
{
  // copying file name
  FileIndex *file_index;

  g_assert (IDE_IS_CLANG_INDEX (self));

  file_index = g_hash_table_lookup (self->file_indexes ,file_name);

  if (file_index == NULL)
    {
      file_index = g_slice_new (FileIndex);
      file_index->file_id = g_hash_table_size (self->file_indexes) + 1;
      file_index->index = g_object_new (IDE_TYPE_CLANG_FILE_INDEX,
                                        NULL);
      g_hash_table_insert (self->file_indexes,
                           g_strdup (file_name), file_index);
    }

  *file_id = file_index->file_id;

  // g_print ("recording decl : %s line : %u, start column : %u in file : %s\n", 
  //          unique_string, line, start_column, file_name);

  ide_clang_file_index_record_declaration (file_index->index, unique_string,
                                           line, start_column, 
                                           type, local,
                                           *file_id, declaration_id);

}

void 
ide_clang_index_record_reference (IdeClangIndex *self,
                                  const gchar *file_name,
                                  guint32 line,
                                  guint32 start_column, guint32 end_column, guint32 type,
                                  guint32 file_id, gint32 declaration_id)
{
  FileIndex *file_index;

  g_assert (IDE_IS_CLANG_INDEX (self));

  file_index = g_hash_table_lookup (self->file_indexes, file_name);

  if (file_index == NULL)
    {
      // file index should not be null here
      return;
    }

  // g_print ("recording ref line : %u, start column : %u, end column %u in file : %s\n", 
  //          line, start_column, end_column, file_name);

  ide_clang_file_index_record_reference (file_index->index, line, start_column, end_column, type,
                                         file_id, declaration_id);
}
static void
ide_clang_index_get_file_index_by_id (IdeClangIndex *self, guint32 file_id, 
                                      IdeClangFileIndex **ide_file_index, gchar **file_name)
{
  GHashTableIter iter;
  gpointer key, value;
  FileIndex *file_index;

  g_assert (IDE_IS_CLANG_INDEX (self));

  g_hash_table_iter_init (&iter, self->file_indexes);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      file_index = value;
      if (file_index->file_id == file_id)
        {
          *ide_file_index = file_index->index;
          *file_name = key;
        }
    }
}

static void
ide_clang_index_get_file_index_by_name (IdeClangIndex *self, gchar *file_name,
                                        IdeClangFileIndex **ide_file_index,
                                        guint32 *file_id)
{
  FileIndex *file_index;

  g_assert (IDE_IS_CLANG_INDEX (self));

  file_index = g_hash_table_lookup (self->file_indexes, file_name);

  if (file_index == NULL)
  {
    *ide_file_index = NULL;
    *file_id = 0;  
  }
  else
  {
    *ide_file_index = file_index->index;
    *file_id = file_index->file_id;
  }
}

/* Searched for definition using USR of declaration */
static void
ide_clang_index_search_by_USR (IdeClangIndex *self,
                               gchar *USR,
                               gchar **def_file_name, guint32 *def_line, guint32 *def_column)
{
  GHashTableIter iter;
  gpointer key, value;
  FileIndex *file_index;

  g_hash_table_iter_init (&iter, self->file_indexes);

  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      file_index = value;

      ide_clang_file_index_get_definition_by_USR (file_index->index, USR,
                                                  def_line, def_column);
      
      if (def_line)
        *def_file_name = key;        
      else
        continue; 
    }
}

/*
 * This will search Definition or Declaration for Declaration or Reference given a location
 * in file.
 */

void 
ide_clang_index_search_by_location (IdeClangIndex *self,
                                    gchar *file_name, guint32 ref_line, guint32 ref_column,
                                    gchar **dest_file_name, guint32 *dest_line, guint32 *dest_column,
                                    guint32 *dest_type)
{
  IdeClangFileIndex *decl_file_index, *ref_file_index;
  guint32 decl_file_id, decl_id, ref_file_id;
  guint32 decl_type, ref_type;
  guint32 decl_line, decl_column;
  gchar *USR, *decl_file_name;

  g_assert (IDE_IS_CLANG_INDEX (self));

  *dest_file_name = NULL;
  *dest_line = 0;
  *dest_column = 0;
  *dest_type = 0;

  /* Get information of refernce and declaration.
   * For this first get reference type, declaration file id and declaration id
   * Then get file name, USR, line, column, type of declaration referenced
   */

  ide_clang_index_get_file_index_by_name (self, file_name, &ref_file_index, &ref_file_id);

  if (ref_file_index == NULL)
  {
    g_print ("No file index found for given file\n");
    return;
  }

  ide_clang_file_index_get_declaration_referenced (ref_file_index, ref_line, ref_column,
                                                   &ref_type,
                                                   &decl_file_id, &decl_id);
  if(decl_file_id == 0)
    {
      g_print ("No declarations found\n");
      return;
    }

  if (decl_file_id == ref_file_id)
    {
      decl_file_index = ref_file_index;
      decl_file_name = file_name;
    }
  else
    {
      ide_clang_index_get_file_index_by_id (self, decl_file_id, &decl_file_index, &decl_file_name);
    }

  USR = ide_clang_file_index_get_declaration_by_id (decl_file_index, decl_id,
                                                    &decl_line, &decl_column, &decl_type);

  /*
   * After getting information of refernce and declaration
   * If declaration is local or it is definition then return declaration found,
   *         else search index using USR for defnition.
   * For reference if definition is not found then return declaration.
   * For Declaration of definition is not found then return nothing
   */
  *dest_file_name = decl_file_name;
  *dest_line = decl_line;
  *dest_column = decl_column;
  *dest_type = decl_type;

  if (decl_id < 0 || decl_type == RT_Definition)
    {
      return ;
    }
  else
    {

      ide_clang_index_search_by_USR (self, USR,
                                     &decl_file_name, &decl_line, &decl_column);
      if (decl_line)
        {
          *dest_file_name = decl_file_name;
          *dest_line = decl_line;
          *dest_column = decl_column;
          *dest_type = RT_Definition;
          return;
        }
      else if (decl_line == 0 && ref_type == RT_Reference)
        {
          return;
        }
    }
}

static void
ide_clang_index_init (IdeClangIndex *self)
{
  g_assert (IDE_IS_CLANG_INDEX (self));

  self->file_indexes = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
ide_clang_index_class_init (IdeClangIndexClass *klass)
{

}