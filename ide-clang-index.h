#ifndef IDE_CLANG_INDEX_H
#define IDE_CLANG_INDEX_H

#include <glib-object.h>

#define IDE_TYPE_CLANG_INDEX (ide_clang_index_get_type ())

G_DECLARE_FINAL_TYPE (IdeClangIndex, ide_clang_index, IDE, CLANG_INDEX, GObject)

void ide_clang_index_record_declaration (IdeClangIndex *self,
                                         const gchar *file_name, const gchar *unique_string,
                                         guint32 line, guint32 start_column,
                                         guint32 type, gboolean local,
                                         guint32 *file_id, gint32 *declaration_id);

void ide_clang_index_record_reference   (IdeClangIndex *self,
                                         const gchar *file_name,
                                         guint32 line, guint32 start_column, guint32 end_column, guint32 type,
                                         guint32 file_id, gint32 declaration_id);

void ide_clang_index_search_by_location (IdeClangIndex *self,
                                         gchar *file_name, guint32 ref_line, guint32 ref_column,
                                         gchar **dest_file_name, guint32 *dest_line, guint32 *dest_column,
                                         guint32 *dest_type);

#endif