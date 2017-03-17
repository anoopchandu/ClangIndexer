#include <glib.h>
#include <stdio.h>

#include "ide-clang-indexer.h"
#include "ide-clang-index.h"

int main (int numargs, char* args[])
{
  IdeClangIndexer *indexer;
  IdeClangIndex *index;
  const gchar* commands[20][20];
  gchar inp[100];
  guint32 line, column;
  guint32 num, nargs;
  FILE *file;
  gchar *dest_file_name;
  guint32 dest_line, dest_column, dest_type;

  if (numargs < 2)
    return -1;

  file = fopen (args[1], "r");
  indexer = g_object_new (IDE_TYPE_CLANG_INDEXER,
                          NULL);

  fscanf (file, "%d", &num);

  for (guint32 i = 0; i < num; i++)
  {
    fscanf (file, "%s", inp);
    commands[i][0] = g_strdup (inp);
    fscanf (file, "%d", &nargs);
    for (guint32 j = 1; j <= nargs; j++)
    {
      fscanf (file, "%s", inp);
      commands[i][j] = g_strdup (inp);
    }
    ide_clang_indexer_insert_command (indexer, commands[i][0], commands[i], nargs);
  }

  // printf ("Enter to start\n");
  // getchar ();
  ide_clang_indexer_start (indexer);

  index = ide_clang_indexer_get_index (indexer);

  g_print ("Enter <file_name line column> to search\n> ");

  while (scanf ("%s %d %d", inp, &line, &column) != EOF)
  {
    ide_clang_index_search_by_location (index, 
                                        inp, line, column,
                                        &dest_file_name, &dest_line, &dest_column,
                                        &dest_type);
    g_print ("\tFile : %s\n\tLine : %u\n\tColumn : %u\n\ttype : %u\n",
             dest_file_name, dest_line, dest_column, dest_type); 
    g_print ("> ");
  }
  printf("\n");
}