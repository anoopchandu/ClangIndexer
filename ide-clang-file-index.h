#ifndef IDE_CLANG_FILE_INDEX_H
#define IDE_CLANG_FILE_INDEX_H

#include <glib-object.h>

#define IDE_TYPE_CLANG_FILE_INDEX (ide_clang_file_index_get_type ())

G_DECLARE_FINAL_TYPE (IdeClangFileIndex, ide_clang_file_index, IDE, CLANG_FILE_INDEX, GObject)

void     ide_clang_file_index_record_declaration            (IdeClangFileIndex *self,
                                                             const gchar *unique_string,
                                                             guint32 line, guint32 start_column,
                                                             guint32 type, gboolean local,
                                                             guint32 file_id, gint32 *declaration_id);

void     ide_clang_file_index_record_reference              (IdeClangFileIndex *self,
                                                             guint32 line, guint32 start_column, guint32 end_column, guint32 type,
                                                             guint32 declaration_file_id, gint32 declaration_id);

void     ide_clang_file_index_get_declaration_referenced    (IdeClangFileIndex *self,
                                                             guint32 line, guint32 column,
                                                             guint32 *reference_type,
                                                             guint32 *declaration_file_id, gint32 *declaration_id);

gchar*   ide_clang_file_index_get_declaration_by_id         (IdeClangFileIndex *self,
                                                             gint32 declaration_id,
                                                             guint32 *line, guint32 *column, guint32 *type);

void     ide_clang_file_index_get_definition_by_USR         (IdeClangFileIndex *self,
                                                             gchar *USR,
                                                             guint32 *line, guint32 *column);
#endif