/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include <config.h>

#include <giomm.h>
#include <giomm/themedicon.h>
#include <gtk/gtk.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
#include <glib/gi18n.h>

#include "procman-app.h"
#include "disks.h"
#include "util.h"
#include "interface.h"
#include "iconthemewrapper.h"

enum DiskColumns
{
    /* string columns* */
    DISK_DEVICE,
    DISK_DIR,
    DISK_TYPE,
    DISK_TOTAL,
    DISK_FREE,
    DISK_AVAIL,
    /* USED has to be the last column */
    DISK_USED,
    // then unvisible columns
    /* PixBuf column */
    DISK_ICON,
    /* numeric columns */
    DISK_USED_PERCENTAGE,
    DISK_N_COLUMNS
};

static void
cb_sort_changed (GtkTreeSortable *model, gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;

    procman_save_tree_state (app->settings,
                             GTK_WIDGET (app->disk_list),
                             "disktreenew");
}

static void
fsusage_stats(const glibtop_fsusage *buf,
              guint64 *bused, guint64 *bfree, guint64 *bavail, guint64 *btotal,
              gint *percentage)
{
    guint64 total = buf->blocks * buf->block_size;

    if (!total) {
        /* not a real device */
        *btotal = *bfree = *bavail = *bused = 0ULL;
        *percentage = 0;
    } else {
        int percent;
        *btotal = total;
        *bfree = buf->bfree * buf->block_size;
        *bavail = buf->bavail * buf->block_size;
        *bused = *btotal - *bfree;
        /* percent = 100.0f * *bused / *btotal; */
        percent = 100 * *bused / (*bused + *bavail);
        *percentage = CLAMP(percent, 0, 100);
    }
}


namespace
{
    string get_icon_for_path(const std::string& path)
    {
        using namespace Glib;
        using namespace Gio;

        // FIXME: I don't know whether i should use Volume or Mount or UnixMount
        // all i need an icon name.
        RefPtr<VolumeMonitor> monitor = VolumeMonitor::get();

        std::vector<RefPtr<Mount> > mounts = monitor->get_mounts();

        for (size_t i = 0; i != mounts.size(); ++i) {
            if (mounts[i]->get_name() != path)
                continue;

            RefPtr<Icon> icon = mounts[i]->get_icon();
            RefPtr<ThemedIcon> themed_icon = RefPtr<ThemedIcon>::cast_dynamic(icon);

            if (themed_icon) {
                char* name = 0;
                // FIXME: not wrapped yet
                g_object_get(G_OBJECT(themed_icon->gobj()), "name", &name, NULL);
                return make_string(name);
            }
        }

        return "";
    }
}


static Glib::RefPtr<Gdk::Pixbuf>
get_icon_for_device(const char *mountpoint)
{
    procman::IconThemeWrapper icon_theme;
    string icon_name = get_icon_for_path(mountpoint);
    if (icon_name == "")
        // FIXME: defaults to a safe value
        icon_name = "drive-harddisk"; // get_icon_for_path("/");
    return icon_theme->load_icon(icon_name, 24, Gtk::ICON_LOOKUP_USE_BUILTIN);
}


static gboolean
find_disk_in_model(GtkTreeModel *model, const char *mountpoint,
                   GtkTreeIter *result)
{
    GtkTreeIter iter;
    gboolean found = FALSE;

    if (gtk_tree_model_get_iter_first(model, &iter)) {
        do {
            char *dir;

            gtk_tree_model_get(model, &iter,
                               DISK_DIR, &dir,
                               -1);

            if (dir && !strcmp(dir, mountpoint)) {
                *result = iter;
                found = TRUE;
            }

            g_free(dir);

        } while (!found && gtk_tree_model_iter_next(model, &iter));
    }

    return found;
}



static void
remove_old_disks(GtkTreeModel *model, const glibtop_mountentry *entries, guint n)
{
    GtkTreeIter iter;

    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;

    while (true) {
        char *dir;
        guint i;
        gboolean found = FALSE;

        gtk_tree_model_get(model, &iter,
                           DISK_DIR, &dir,
                           -1);

        for (i = 0; i != n; ++i) {
            if (!strcmp(dir, entries[i].mountdir)) {
                found = TRUE;
                break;
            }
        }

        g_free(dir);

        if (!found) {
            if (!gtk_list_store_remove(GTK_LIST_STORE(model), &iter))
                break;
            else
                continue;
        }

        if (!gtk_tree_model_iter_next(model, &iter))
            break;
    }
}



static void
add_disk(GtkListStore *list, const glibtop_mountentry *entry, bool show_all_fs)
{
    Glib::RefPtr<Gdk::Pixbuf> pixbuf;
    GtkTreeIter iter;
    glibtop_fsusage usage;
    guint64 bused, bfree, bavail, btotal;
    gint percentage;

    glibtop_get_fsusage(&usage, entry->mountdir);

    if (not show_all_fs and usage.blocks == 0) {
        if (find_disk_in_model(GTK_TREE_MODEL(list), entry->mountdir, &iter))
            gtk_list_store_remove(list, &iter);
        return;
    }

    fsusage_stats(&usage, &bused, &bfree, &bavail, &btotal, &percentage);
    pixbuf = get_icon_for_device(entry->mountdir);

    /* if we can find a row with the same mountpoint, we get it but we
       still need to update all the fields.
       This makes selection persistent.
    */
    if (!find_disk_in_model(GTK_TREE_MODEL(list), entry->mountdir, &iter))
        gtk_list_store_append(list, &iter);

    gtk_list_store_set(list, &iter,
                       DISK_ICON, pixbuf->gobj(),
                       DISK_DEVICE, entry->devname,
                       DISK_DIR, entry->mountdir,
                       DISK_TYPE, entry->type,
                       DISK_USED_PERCENTAGE, percentage,
                       DISK_TOTAL, btotal,
                       DISK_FREE, bfree,
                       DISK_AVAIL, bavail,
                       DISK_USED, bused,
                       -1);
}

static void
mount_changed (GVolumeMonitor *monitor, GMount *mount, ProcmanApp *app)
{
    disks_update(app);
}

static gboolean
cb_timeout (gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;
    disks_update (app);

    return G_SOURCE_CONTINUE;
}

void
disks_update(ProcmanApp *app)
{
    GtkListStore *list;
    glibtop_mountentry * entries;
    glibtop_mountlist mountlist;
    guint i;
    gboolean show_all_fs;

    list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(app->disk_list)));
    show_all_fs = g_settings_get_boolean (app->settings, "show-all-fs");

    entries = glibtop_get_mountlist (&mountlist, show_all_fs);

    remove_old_disks(GTK_TREE_MODEL(list), entries, mountlist.number);

    for (i = 0; i < mountlist.number; i++)
        add_disk(list, &entries[i], show_all_fs);

    g_free(entries);
}

static void
init_volume_monitor (ProcmanApp *app)
{
    GVolumeMonitor *monitor = g_volume_monitor_get ();

    g_signal_connect (monitor, "mount-added", G_CALLBACK (mount_changed), app);
    g_signal_connect (monitor, "mount-changed", G_CALLBACK (mount_changed), app);
    g_signal_connect (monitor, "mount-removed", G_CALLBACK (mount_changed), app);
}

void
disks_freeze (ProcmanApp *app)
{
  if (app->disk_timeout) {
      g_source_remove (app->disk_timeout);
      app->disk_timeout = 0;
  }
}

void
disks_thaw (ProcmanApp *app)
{
  if (app->disk_timeout)
      return;

  app->disk_timeout = g_timeout_add (app->config.disks_update_interval,
                                     cb_timeout,
                                     app);
}

void
disks_reset_timeout (ProcmanApp *app)
{
    disks_freeze (app);
    disks_thaw (app);
}

static void
cb_disk_columns_changed(GtkTreeView *treeview, gpointer data)
{
    ProcmanApp *app = static_cast<ProcmanApp *>(data);

    procman_save_tree_state(app->settings,
                            GTK_WIDGET(treeview),
                            "disktreenew");
}


static void open_dir(GtkTreeView       *tree_view,
                     GtkTreePath       *path,
                     GtkTreeViewColumn *column,
                     gpointer          user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    char *dir, *url;

    model = gtk_tree_view_get_model(tree_view);

    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        char *p;
        p = gtk_tree_path_to_string(path);
        g_warning("Cannot get iter for path '%s'\n", p);
        g_free(p);
        return;
    }

    gtk_tree_model_get(model, &iter, DISK_DIR, &dir, -1);

    url = g_strdup_printf("file://%s", dir);

    GError* error = 0;
    if (!g_app_info_launch_default_for_uri(url, NULL, &error)) {
        g_warning("Cannot open '%s' : %s\n", url, error->message);
        g_error_free(error);
    }

    g_free(url);
    g_free(dir);
}

static void
cb_disk_list_destroying (GtkWidget *self, gpointer data)
{
    g_signal_handlers_disconnect_by_func(self, (gpointer) cb_disk_columns_changed, data);
    
    g_signal_handlers_disconnect_by_func (gtk_tree_view_get_model (GTK_TREE_VIEW(self)),
                                          (gpointer) cb_sort_changed,
                                          data);
}

static void
cb_show_all_fs_changed (GSettings *settings, const gchar *key, gpointer data)
{
    ProcmanApp *app = (ProcmanApp *) data;

    disks_update (app);
    disks_reset_timeout (app);
}


void
create_disk_view(ProcmanApp *app, GtkBuilder *builder)
{
    GtkWidget *scrolled;
    GtkWidget *disk_tree;
    GtkListStore *model;
    GtkTreeViewColumn *col;
    GtkCellRenderer *cell;
    guint i;

    init_volume_monitor (app);
    const gchar * const titles[] = {
        N_("Device"),
        N_("Directory"),
        N_("Type"),
        N_("Total"),
        N_("Free"),
        N_("Available"),
        N_("Used")
    };

    GSettings * settings = g_settings_get_child (app->settings, "disktreenew");

    scrolled = GTK_WIDGET (gtk_builder_get_object (builder, "disks_scrolled"));

    model = gtk_list_store_new(DISK_N_COLUMNS,      /* n columns */
                               G_TYPE_STRING,       /* DISK_DEVICE */
                               G_TYPE_STRING,       /* DISK_DIR */
                               G_TYPE_STRING,       /* DISK_TYPE */
                               G_TYPE_UINT64,       /* DISK_TOTAL */
                               G_TYPE_UINT64,       /* DISK_FREE */
                               G_TYPE_UINT64,       /* DISK_AVAIL */
                               G_TYPE_UINT64,       /* DISK_USED */
                               GDK_TYPE_PIXBUF,     /* DISK_ICON */
                               G_TYPE_INT           /* DISK_USED_PERCENTAGE */
        );
    
    disk_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
    g_signal_connect(G_OBJECT(disk_tree), "row-activated", G_CALLBACK(open_dir), NULL);
    app->disk_list = disk_tree;
    gtk_container_add(GTK_CONTAINER(scrolled), disk_tree);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(disk_tree), TRUE);
    g_object_unref(G_OBJECT(model));

    /* icon + device */

    col = gtk_tree_view_column_new();
    cell = gtk_cell_renderer_pixbuf_new();
    
    gtk_tree_view_column_pack_start(col, cell, FALSE);
    gtk_tree_view_column_set_attributes(col, cell, "pixbuf", DISK_ICON,
                                        NULL);

    cell = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(col, cell, FALSE);
    gtk_tree_view_column_set_attributes(col, cell, "text", DISK_DEVICE,
                                        NULL);
    gtk_tree_view_column_set_title(col, _(titles[DISK_DEVICE]));
    gtk_tree_view_column_set_sort_column_id(col, DISK_DEVICE);
    bind_column_to_gsetting (settings, col);  
    gtk_tree_view_column_set_reorderable(col, TRUE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_append_column(GTK_TREE_VIEW(disk_tree), col);


    /* sizes - used */

    for (i = DISK_DIR; i <= DISK_AVAIL; i++) {
        cell = gtk_cell_renderer_text_new();
        col = gtk_tree_view_column_new();
        gtk_tree_view_column_pack_start(col, cell, TRUE);
        gtk_tree_view_column_set_title(col, _(titles[i]));
        gtk_tree_view_column_set_resizable(col, TRUE);
        gtk_tree_view_column_set_sort_column_id(col, i);
        bind_column_to_gsetting (settings, col);
        gtk_tree_view_column_set_reorderable(col, TRUE);
        gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column(GTK_TREE_VIEW(disk_tree), col);
        switch (i) {
            case DISK_TOTAL:
            case DISK_FREE:
            case DISK_AVAIL:
                g_object_set(cell, "xalign", 1.0f, NULL);
                gtk_tree_view_column_set_cell_data_func(col, cell,
                                                        &procman::size_si_cell_data_func,
                                                        GUINT_TO_POINTER(i),
                                                        NULL);
                break;

            default:
                gtk_tree_view_column_set_attributes(col, cell,
                                                    "text", i,
                                                    NULL);
                break;
        }
    }

    /* used + percentage */

    col = gtk_tree_view_column_new();
    cell = gtk_cell_renderer_text_new();
    g_object_set(cell, "xalign", 1.0f, NULL);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_pack_start(col, cell, FALSE);
    gtk_tree_view_column_set_cell_data_func(col, cell,
                                            &procman::size_si_cell_data_func,
                                            GUINT_TO_POINTER(DISK_USED),
                                            NULL);
    gtk_tree_view_column_set_title(col, _(titles[DISK_USED]));

    cell = gtk_cell_renderer_progress_new();
    gtk_tree_view_column_pack_start(col, cell, TRUE);
    gtk_tree_view_column_set_attributes(col, cell, "value",
                                        DISK_USED_PERCENTAGE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(disk_tree), col);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_sort_column_id(col, DISK_USED);
    bind_column_to_gsetting (settings, col);
    gtk_tree_view_column_set_reorderable(col, TRUE);

    /* numeric sort */

    procman_get_tree_state (app->settings, disk_tree,
                           "disktreenew");

    g_signal_connect (G_OBJECT(disk_tree), "destroy",
                      G_CALLBACK(cb_disk_list_destroying),
                      app);

    g_signal_connect (G_OBJECT(disk_tree), "columns-changed",
                      G_CALLBACK(cb_disk_columns_changed), app);
                      
    g_signal_connect (G_OBJECT (model), "sort-column-changed",
                      G_CALLBACK (cb_sort_changed), app);

    g_signal_connect (app->settings, "changed::show-all-fs",
                      G_CALLBACK (cb_show_all_fs_changed), app);

    gtk_widget_show (disk_tree);
}
