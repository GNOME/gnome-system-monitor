#ifndef _GSM_TREE_VIEW_H_
#define _GSM_TREE_VIEW_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GSM_TYPE_TREE_VIEW (gsm_tree_view_get_type ())
#define GSM_TREE_VIEW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_TREE_VIEW, GsmTreeView))
#define GSM_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GSM_TYPE_TREE_VIEW, GsmTreeViewClass))
#define GSM_IS_TREE_VIEW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_TREE_VIEW))
#define GSM_IS_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSM_TYPE_TREE_VIEW))
#define GSM_TREE_VIEW_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSM_TYPE_TREE_VIEW, GsmTreeViewClass))

typedef struct _GsmTreeView GsmTreeView;
typedef struct _GsmTreeViewClass GsmTreeViewClass;

struct _GsmTreeView
{
  GtkTreeView parent_instance;
};

struct _GsmTreeViewClass
{
  GtkTreeViewClass parent_class;
};

GType               gsm_tree_view_get_type (void) G_GNUC_CONST;
GsmTreeView *       gsm_tree_view_new (GSettings *settings,
                                       gboolean   store_column_order);
void                gsm_tree_view_save_state (GsmTreeView *tree_view);
void                gsm_tree_view_load_state (GsmTreeView *tree_view);
GtkTreeViewColumn * gsm_tree_view_get_column_from_id (GsmTreeView *tree_view,
                                                      gint         sort_id);
void                gsm_tree_view_add_excluded_column (GsmTreeView *tree_view,
                                                       gint         column_id);
void                gsm_tree_view_append_and_bind_column (GsmTreeView       *tree_view,
                                                          GtkTreeViewColumn *column);

G_END_DECLS

#endif /* _GSM_TREE_VIEW_H_ */
