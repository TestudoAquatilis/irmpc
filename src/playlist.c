#include "playlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

static GTree        *playlist_table        = NULL;
static GStringChunk *playlist_name_storage = NULL;

static gint irmpc_playlist_table_cmp_func (gconstpointer a, gconstpointer b)
{
    unsigned int aval = GPOINTER_TO_INT(a);
    unsigned int bval = GPOINTER_TO_INT(b);

    if (aval > bval) return  1;
    if (aval < bval) return -1;
    return 0;
}

void irmpc_playlist_add (unsigned int number, const char *name, bool random)
{
    if (name == NULL) return;

    if (playlist_name_storage == NULL) {
        playlist_name_storage = g_string_chunk_new (64);
        if (playlist_name_storage == NULL) return;
    }

    gchar *entry_name = g_string_chunk_insert (playlist_name_storage, name);

    if (playlist_table == NULL) {
        playlist_table = g_tree_new (irmpc_playlist_table_cmp_func);
        if (playlist_table == NULL) return;
    }

    struct playlist_info *entry;

    entry = (struct playlist_info *) malloc (sizeof (struct playlist_info));
    if (entry == NULL) return;

    entry->name   = entry_name;
    entry->random = random;

    g_tree_insert (playlist_table, GINT_TO_POINTER(number), entry);
}

const struct playlist_info * irmpc_playlist_get (unsigned int number)
{
    if (playlist_table == NULL) return NULL;

    gpointer result = g_tree_lookup (playlist_table, GINT_TO_POINTER (number));

    return (struct playlist_info *) result;
}

static gboolean irmpc_playlist_entry_free (gpointer key, gpointer value, gpointer data)
{
    if (value == NULL) return false;
    free (value);
    return false;
}

void irmpc_playlist_free ()
{
    if (playlist_table != NULL) {
        g_tree_foreach (playlist_table, irmpc_playlist_entry_free, NULL);
        g_tree_destroy (playlist_table);
        playlist_table = NULL;
    }

    if (playlist_name_storage != NULL) {
        g_string_chunk_free (playlist_name_storage);
        playlist_name_storage = NULL;
    }
}

static gboolean irmpc_playlist_entry_print_debug (gpointer key, gpointer value, gpointer data)
{
    unsigned int number = GPOINTER_TO_INT(key);
    struct playlist_info *entry = (struct playlist_info *) value;

    if (entry == NULL) return false;

    fprintf (stderr, " %04d -> \"%s\"%s\n", number, entry->name, (entry->random ? "  (random)" : ""));

    return false;
}

void irmpc_playlist_print_debug ()
{
    if (playlist_table != NULL) {
        fprintf (stderr, "playlist:\n");
        g_tree_foreach (playlist_table, irmpc_playlist_entry_print_debug, NULL);
    } else {
        fprintf (stderr, "playlist empty\n");
    }
}
