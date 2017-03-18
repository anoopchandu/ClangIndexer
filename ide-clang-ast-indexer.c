/* This traverses AST and puts declarations and references into IdeClangindex */

#include <clang-c/Index.h>
#include <glib.h>

#include "ide-clang-ast-indexer.h"
#include "ide-clang-index.h"

struct _IdeClangASTIndexer
{
  GObject parent;
  IdeClangIndex *index;
  GHashTable *declarations;
  CXTranslationUnit tu;
};

G_DEFINE_TYPE (IdeClangASTIndexer, ide_clang_ast_indexer, G_TYPE_OBJECT)

struct _Declaration
{
  guint32 file_id;
  guint32 declaration_id;
};

typedef struct _Declaration Declaration;

guint
hash_cursor (gconstpointer key)
{
  const CXCursor *cursor = key;
  
  return clang_hashCursor (*cursor);
}

gboolean
equal_cursors (gconstpointer a, gconstpointer b)
{
  const CXCursor *cursora = a;
  const CXCursor *cursorb = b;

  return clang_equalCursors (*cursora, *cursorb);
}

static void
print_cursor (IdeClangASTIndexer *self,
             CXCursor cursor)
{
  CXString cursorKindName, spelling, fileName;
  CXFile file;
  CXSourceLocation location;
  enum CXCursorKind cursorKind;
  guint32 line, column, offset;

  g_assert (IDE_IS_CLANG_AST_INDEXER (self));

  cursorKind = clang_getCursorKind (cursor);
  cursorKindName = clang_getCursorKindSpelling (cursorKind);
  
  spelling = clang_getCursorSpelling (cursor);
  
  location = clang_getCursorLocation (cursor);
  clang_getSpellingLocation (location, &file, &line, &column, &offset);
  fileName = clang_getFileName (file);

  if (((cursorKind != CXCursor_MacroDefinition) ||
       (cursorKind == CXCursor_MacroDefinition && clang_getCString (fileName) != NULL)) &&
      (cursorKind != CXCursor_InvalidFile))
    {
      g_print ("%s (%s) file:%s line:%u column:%u offset:%u\n",
               clang_getCString (cursorKindName),
               clang_getCString (spelling),
               clang_getCString (fileName), 
               line, column, offset);
    }
  else
    {
      g_print ("\n");
    }

  clang_disposeString (cursorKindName);
  clang_disposeString (spelling);
  clang_disposeString (fileName);
}

/* For macro definition no unique string is required because it doesn't need to be searchable,
 * because always we can find definition of a macro reference during indexing.
 */
static void
record_declaration (IdeClangASTIndexer *self,
                    CXCursor decl_cursor,
                    guint32 *file_id, gint32 *declaration_id)
{
  Declaration *declaration;

  g_assert (IDE_IS_CLANG_AST_INDEXER (self));

  declaration = (Declaration*) g_hash_table_lookup (self->declarations, &decl_cursor);

  if (declaration == NULL)
    {
      // enum CXCursorKind cursor_kind;
      guint32 line, column, id, type;
      CXSourceLocation location;
      CXFile file;
      CXString file_name, USR;
      IdeClangIndex *index;
      gchar *unique_string;
      gboolean local;
      CXCursor *decl_cursor_cp;
      const gchar *cUSR;

      g_assert (IDE_IS_CLANG_AST_INDEXER (self));

      index = self->index;

      location = clang_getCursorLocation (decl_cursor);
      clang_getSpellingLocation (location, &file, &line, &column, NULL);

      // cursor_kind = clang_getCursorKind (decl_cursor);
      file_name = clang_getFileName (file);
      
      /* disposeString not required */
      if (clang_getCString (file_name) == NULL)
        return;

      USR = clang_getCursorUSR (decl_cursor);
      cUSR = clang_getCString(USR);
      /* for non local declarations USR will be like "c:@..." */
      while (*cUSR != ':')
        cUSR++;
      local = (cUSR[1] != '@');
      // g_print ("%s %d\n",clang_getCString (USR), local);
      type = clang_isCursorDefinition (decl_cursor);

      // g_print ("\t\t\t\t");
      // print_cursor (self, decl_cursor);
      // g_print ("\t\t\t\tUSR : %s\n", clang_getCString (USR));

      ide_clang_index_record_declaration (index, clang_getCString (file_name),
                                          clang_getCString (USR), line, column,
                                          type, local,
                                          file_id, declaration_id);

      declaration = g_slice_new (Declaration);

      declaration->file_id = *file_id;
      declaration->declaration_id = *declaration_id;

      decl_cursor_cp = g_slice_new (CXCursor);
      *decl_cursor_cp = decl_cursor;

      g_hash_table_insert (self->declarations, decl_cursor_cp, declaration);
      g_print (".");
      clang_disposeString (file_name);
      clang_disposeString (USR);
    }
  else
    {
      *file_id = declaration->file_id;
      *declaration_id = declaration->declaration_id;
    }
}

static void 
record_reference (IdeClangASTIndexer *self,
                  CXCursor cursor, guint32 type)
{
  CXSourceLocation start_location, end_location;
  CXSourceRange range;
  CXCursor decl_cursor;
  CXFile file;
  guint32 start_line, end_line;
  guint32 start_column, end_column, offset, declaration_file_id;
  gint32 declaration_id;
  CXString file_name;
  IdeClangIndex *index;
  CXToken *tokens;
  guint32 num_tokens;

  g_assert (IDE_IS_CLANG_AST_INDEXER (self));
  
  index = self->index;

  // print_cursor (self, cursor);

  g_assert (IDE_IS_CLANG_INDEX (index));
  if (type == RT_Definition)
    {
     decl_cursor = cursor;
    }
  else if (type == RT_Declaration)
    {
      decl_cursor = clang_getCursorDefinition (cursor);
      if (clang_Cursor_isNull (decl_cursor))
        decl_cursor = cursor;
    }
  else if (type == RT_Reference)
    {
      decl_cursor = clang_getCursorDefinition (cursor);
      if (clang_Cursor_isNull (decl_cursor))
        decl_cursor = clang_getCursorReferenced (cursor); 
    }
  else
    {
      return;
    }

  record_declaration (self, decl_cursor, &declaration_file_id, &declaration_id);
  /* should optimize this */
  start_location = clang_getCursorLocation (cursor);
  clang_getSpellingLocation (start_location, NULL, &start_line, &start_column, NULL);

  range = clang_getCursorExtent (cursor);
  end_location = clang_getRangeEnd (range);
  clang_getSpellingLocation (end_location, NULL, &end_line, &end_column, NULL);

  range = clang_getRange (start_location, end_location);

  clang_tokenize (self->tu, range, &tokens, &num_tokens);
  if (num_tokens == 0)
    goto clean;

  /*TODO : For some constructs first few tokens needs to be taken like c++ destructor*/
  range = clang_getTokenExtent (self->tu, tokens[0]);

  start_location = clang_getRangeStart(range);
  clang_getSpellingLocation (start_location, &file, &start_line, &start_column, NULL);

  end_location = clang_getRangeEnd (range);
  clang_getSpellingLocation (end_location, NULL, &end_line, &end_column, NULL);

  file_name = clang_getFileName (file);

  /*assuming a token will be on single line*/

  ide_clang_index_record_reference (index, clang_getCString (file_name),
                                    start_line, start_column, end_column, type,
                                    declaration_file_id, declaration_id);
  g_print (".");

clean:
  clang_disposeTokens (self->tu, tokens, num_tokens);
  clang_disposeString (file_name);
}

enum CXChildVisitResult
visitor (CXCursor cursor, CXCursor parent, CXClientData clientData)
{
  enum CXCursorKind cursorKind;
  IdeClangASTIndexer *self = IDE_CLANG_AST_INDEXER (clientData);
  guint32 file_id;
  gint32 declaration_id;

  g_assert (IDE_IS_CLANG_AST_INDEXER (self));

  // print_cursor (self, cursor);
  cursorKind = clang_getCursorKind (cursor);

//   if (clang_Location_isInSystemHeader (clang_getCursorLocation (cursor)))
//       goto end;

  if (cursorKind == CXCursor_DeclRefExpr || cursorKind == CXCursor_MemberRefExpr || 
      cursorKind == CXCursor_TypeRef || cursorKind == CXCursor_MacroExpansion)
    {
      record_reference (self, cursor, RT_Reference);
    }
  /* calling constructor during object creation */
  else if (cursorKind == CXCursor_CallExpr && clang_getCursorKind (parent) == CXCursor_TypeRef)
    {
      record_reference (self, cursor, RT_Reference);
    }
  else if ((cursorKind >= CXCursor_StructDecl) && (cursorKind <= CXCursor_TypeAliasDecl))
    {
      /* for going from declarations to definitions */
      if (!clang_isCursorDefinition (cursor))
        record_reference (self, cursor, RT_Declaration);
      // else
      //   record_reference (self, cursor, RT_Definition);

      record_declaration (self, cursor, &file_id, &declaration_id);
    }
  else if (cursorKind ==  CXCursor_MacroDefinition)
    {
      record_declaration (self, cursor, &file_id, &declaration_id);
    }

  return CXChildVisit_Recurse;
}

/* Extracts declarations and references from Translation Unit and 
 * inserts that into index.
 */
void
ide_clang_ast_indexer_index (IdeClangASTIndexer *self,
                             CXTranslationUnit tu,
                             IdeClangIndex *index)
{
  CXCursor rootCursor;

  g_assert (IDE_IS_CLANG_AST_INDEXER (self));

  self->index = index;
  self->tu = tu;
  rootCursor = clang_getTranslationUnitCursor (tu);

  clang_visitChildren (rootCursor, visitor, self);
}

static void
ide_clang_ast_indexer_init (IdeClangASTIndexer *self)
{
  /* destroying during finalizing */
  g_assert (IDE_IS_CLANG_AST_INDEXER (self));
  
  self->declarations = g_hash_table_new (hash_cursor, equal_cursors);
}

static void
ide_clang_ast_indexer_class_init (IdeClangASTIndexerClass *klass)
{
}