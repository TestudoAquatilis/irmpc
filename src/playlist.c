#include "playlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* sorted playlist table (number -> playlist info) */
static GTree        *playlist_table        = NULL;
/* memory allocation for playlist names */
static GStringChunk *playlist_name_storage = NULL;

/* compare function for playlist keys (number) */
static gint irmpc_playlist_table_cmp_func (gconstpointer a, gconstpointer b)
{
    unsigned int aval = GPOINTER_TO_INT(a);
    unsigned int bval = GPOINTER_TO_INT(b);

    if (aval > bval) return  1;
    if (aval < bval) return -1;
    return 0;
}

/* add a new playlist entry into the table (number, name and random/not random) */
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

/* get playlist info of given number if available */
const struct playlist_info * irmpc_playlist_get (unsigned int number)
{
    if (playlist_table == NULL) return NULL;

    gpointer result = g_tree_lookup (playlist_table, GINT_TO_POINTER (number));

    return (struct playlist_info *) result;
}

/* user data for next/prev playlist lookup traversal */
struct irmpc_nextprev_playlist_traverse_data {
    int direction;
    const struct playlist_info *prev;
    const struct playlist_info *result;
    const char *lookup_name;
};

/* traversal function for next/prev playlist traversal */
static gboolean irmpc_nextprev_playlist_traverse (gpointer key, gpointer value, gpointer data)
{
    struct playlist_info *entry                         = (struct playlist_info *) value;
    struct irmpc_nextprev_playlist_traverse_data *tdata = (struct irmpc_nextprev_playlist_traverse_data *) data;

    if (tdata->direction >= 0) {
        /* search next */
        if (tdata->lookup_name == NULL) {
            /* no current playlist - use first one */
            tdata->result = entry;
            return true;
        }
        if (tdata->prev == NULL) {
            /* first entry: set result in case lookup_name matches none or last */
            tdata->result = entry;
        } else {
            /* check if previous entry matches lookup name */
            if (strcmp (tdata->prev->name, tdata->lookup_name) == 0) {
                tdata->result = entry;
                return true;
            }
        }
    } else {
        /* search prev */
        /* set result in case of last entry and no match */
        tdata->result = entry;

        /* first entry: on match use last entry anyway */
        if (tdata->prev != NULL) {
            /* check if current matches */
            if (tdata->lookup_name != NULL) {
                if (strcmp (entry->name, tdata->lookup_name) == 0) {
                    tdata->result = tdata->prev;
                    return true;
                }
            }

        }
    }

    tdata->prev   = entry;

    return false;
}

/* search next/prev playlist */
const struct playlist_info * irmpc_playlist_nextprev (int direction, const char *lookup_name)
{
    struct irmpc_nextprev_playlist_traverse_data tdata = {direction, NULL, NULL, lookup_name};

    g_tree_foreach (playlist_table, irmpc_nextprev_playlist_traverse, (gpointer) (&tdata));

    return tdata.result;
}



/* traversal function for freeing playlist table entries */
static gboolean irmpc_playlist_entry_free (gpointer key, gpointer value, gpointer data)
{
    if (value == NULL) return false;
    free (value);
    return false;
}

/* free playlist info (table + strings) */
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

/* traversal function for printing of playlist table */
static gboolean irmpc_playlist_entry_print_debug (gpointer key, gpointer value, gpointer data)
{
    unsigned int number = GPOINTER_TO_INT(key);
    struct playlist_info *entry = (struct playlist_info *) value;

    if (entry == NULL) return false;

    fprintf (stderr, " %04d -> \"%s\"%s\n", number, entry->name, (entry->random ? "  (random)" : ""));

    return false;
}

/* print playlist table - used for debugging */
void irmpc_playlist_print_debug ()
{
    if (playlist_table != NULL) {
        fprintf (stderr, "playlist:\n");
        g_tree_foreach (playlist_table, irmpc_playlist_entry_print_debug, NULL);
    } else {
        fprintf (stderr, "playlist empty\n");
    }
}
