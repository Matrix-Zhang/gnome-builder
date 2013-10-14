/* gb-workspace-pane-group.c
 *
 * Copyright (C) 2013 Christian Hergert <christian@hergert.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>

#include "gb-workspace.h"
#include "gb-workspace-pane.h"
#include "gb-workspace-pane-group.h"

G_DEFINE_TYPE(GbWorkspacePaneGroup, gb_workspace_pane_group, GTK_TYPE_GRID)

struct _GbWorkspacePaneGroupPrivate
{
   GtkWidget *notebook;

   gboolean is_fullscreen;
};

enum
{
   PROP_0,
   LAST_PROP
};

enum
{
   TARGET_STRING,
   TARGET_URL,
};

#if 0
static GParamSpec *gParamSpecs[LAST_PROP];
#endif

void
gb_workspace_pane_group_close (GbWorkspacePaneGroup *group,
                               GbWorkspacePane      *pane)
{
   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));
   g_return_if_fail(GB_IS_WORKSPACE_PANE(pane));

   /*
    * TODO: Check can-save.
    */

   gtk_container_remove(GTK_CONTAINER(group->priv->notebook),
                        GTK_WIDGET(pane));
}

void
gb_workspace_pane_group_set_current_pane (GbWorkspacePaneGroup *group,
                                          GbWorkspacePane      *pane)
{
   GbWorkspacePaneGroupPrivate *priv;
   gint page;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));
   g_return_if_fail(GB_IS_WORKSPACE_PANE(pane));

   priv = group->priv;

   gtk_container_child_get(GTK_CONTAINER(priv->notebook),
                           GTK_WIDGET(pane),
                           "position", &page,
                           NULL);

   if (page >= 0) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page);
   }
}

static void
gb_workspace_pane_group_close_clicked (GtkButton            *button,
                                       GbWorkspacePaneGroup *group)
{
   GbWorkspacePane *pane;

   g_return_if_fail(GTK_IS_WIDGET(button));
   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));

   pane = g_object_get_data(G_OBJECT(button), "child");
   if (GB_IS_WORKSPACE_PANE(pane)) {
      gb_workspace_pane_group_close(group, pane);
   }
}

static void
gb_workspace_pane_group_notify_can_save (GbWorkspacePane      *pane,
                                         GParamSpec           *pspec,
                                         GbWorkspacePaneGroup *group)
{
   GbWorkspace *workspace;

   g_assert(GB_IS_WORKSPACE_PANE(pane));
   g_assert(GB_IS_WORKSPACE_PANE_GROUP(group));

   if ((workspace = GB_WORKSPACE(gtk_widget_get_toplevel(GTK_WIDGET(pane))))) {
      gb_workspace_update_actions(workspace);
   }
}

#if 0
static GbWorkspacePane *
gb_workspace_pane_group_get_current_pane (GbWorkspacePaneGroup *group)
{
   GbWorkspacePaneGroupPrivate *priv;
   GbWorkspacePane *pane = NULL;
   gint page;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));

   priv = group->priv;

   if (-1 != (page = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)))) {
      pane = GB_WORKSPACE_PANE(gtk_notebook_get_nth_page(GTK_NOTEBOOK(priv->notebook), page));
   }

   return pane;
}
#endif

void
gb_workspace_pane_group_set_page (GbWorkspacePaneGroup *group,
                                  gint                  page_num)
{
   GbWorkspacePaneGroupPrivate *priv;
   GtkNotebook *notebook;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));

   priv = group->priv;

   notebook = GTK_NOTEBOOK(priv->notebook);

   if ((page_num >= 0) && (page_num < gtk_notebook_get_n_pages(notebook))) {
      gtk_notebook_set_current_page(notebook, page_num);
   }
}

static void
gb_workspace_pane_group_add (GtkContainer *container,
                             GtkWidget    *child)
{
   GbWorkspacePaneGroupPrivate *priv;
   GbWorkspacePaneGroup *group = (GbWorkspacePaneGroup *)container;
   GtkWidget *hbox;
   GtkWidget *close_button;
   GtkWidget *icon;
   GtkWidget *label;
   GtkWidget *spinner;
   gint page;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));
   g_return_if_fail(GTK_IS_WIDGET(child));

   priv = group->priv;

   if (GB_IS_WORKSPACE_PANE(child)) {
      g_signal_connect(child, "notify::can-save",
                       G_CALLBACK(gb_workspace_pane_group_notify_can_save),
                       group);

      hbox = g_object_new(GTK_TYPE_BOX,
                          "hexpand", TRUE,
                          "orientation", GTK_ORIENTATION_HORIZONTAL,
                          "spacing", 0,
                          "visible", TRUE,
                          NULL);

      spinner = g_object_new(GTK_TYPE_SPINNER,
                             "hexpand", FALSE,
                             "vexpand", FALSE,
                             "visible", FALSE,
                             NULL);
      g_object_bind_property(child, "busy", spinner, "active",
                             G_BINDING_SYNC_CREATE);
      g_object_bind_property(child, "busy", spinner, "visible",
                             G_BINDING_SYNC_CREATE);
      gtk_container_add_with_properties(GTK_CONTAINER(hbox), spinner,
                                        "padding", 3,
                                        NULL);

      label = g_object_new(GTK_TYPE_LABEL,
                           "hexpand", FALSE,
                           "label", "*",
                           "single-line-mode", TRUE,
                           "visible", FALSE,
                           "xpad", 1,
                           NULL);
      g_object_bind_property(child, "can-save", label, "visible",
                             G_BINDING_SYNC_CREATE);
      gtk_container_add(GTK_CONTAINER(hbox), label);

      label = g_object_new(GTK_TYPE_LABEL,
                           "hexpand", TRUE,
                           "single-line-mode", TRUE,
                           "visible", TRUE,
                           "xalign", 0.5,
                           "xpad", 0,
                           "yalign", 0.5,
                           "ypad", 0,
                           NULL);
      g_object_bind_property(child, "title", label, "label",
                             G_BINDING_SYNC_CREATE);
      gtk_container_add(GTK_CONTAINER(hbox), label);

      close_button = g_object_new(GTK_TYPE_BUTTON,
                                  "hexpand", FALSE,
                                  "focus-on-click", FALSE,
                                  "name", "nautilus-tab-close-button",
                                  "relief", GTK_RELIEF_NONE,
                                  "visible", TRUE,
                                  NULL);
      g_object_set_data(G_OBJECT(close_button), "child", child);
      g_signal_connect(close_button, "clicked",
                       G_CALLBACK(gb_workspace_pane_group_close_clicked),
                       group);
      gtk_container_add(GTK_CONTAINER(hbox), close_button);

      icon = g_object_new(GTK_TYPE_IMAGE,
                          "icon-name", "window-close-symbolic",
                          "icon-size", GTK_ICON_SIZE_MENU,
                          "tooltip-text", _("Close tab"),
                          "visible", TRUE,
                          NULL);
      gtk_container_add(GTK_CONTAINER(close_button), icon);

      page = gtk_notebook_append_page(GTK_NOTEBOOK(priv->notebook),
                                      child, hbox);
      gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page);
   } else {
      GTK_CONTAINER_CLASS(gb_workspace_pane_group_parent_class)->
         add(container, child);
   }
}

static void
gb_workspace_pane_group_grab_focus (GtkWidget *widget)
{
   GbWorkspacePaneGroup *group = (GbWorkspacePaneGroup *)widget;
   GList *children;
   GList *iter;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));

   children = gtk_container_get_children(GTK_CONTAINER(group->priv->notebook));
   for (iter = children; iter; iter = iter->next) {
      if (gtk_widget_get_visible(iter->data)) {
         gtk_widget_grab_focus(iter->data);
         break;
      }
   }

   g_list_free(children);
}

void
gb_workspace_pane_group_fullscreen (GbWorkspacePaneGroup *group)
{
   GbWorkspacePaneGroupPrivate *priv;
   GtkWidget *widget;
   gint n_pages;
   gint i;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));

   priv = group->priv;

   priv->is_fullscreen = TRUE;

   n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook));
   for (i = 0; i < n_pages; i++) {
      widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(priv->notebook), i);
      if (GB_IS_WORKSPACE_PANE(widget)) {
         gb_workspace_pane_fullscreen(GB_WORKSPACE_PANE(widget));
      }
   }

   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(priv->notebook), FALSE);
}

void
gb_workspace_pane_group_unfullscreen (GbWorkspacePaneGroup *group)
{
   GbWorkspacePaneGroupPrivate *priv;
   GtkWidget *widget;
   gint n_pages;
   gint i;

   g_return_if_fail(GB_IS_WORKSPACE_PANE_GROUP(group));

   priv = group->priv;

   priv->is_fullscreen = FALSE;

   n_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook));
   for (i = 0; i < n_pages; i++) {
      widget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(priv->notebook), i);
      if (GB_IS_WORKSPACE_PANE(widget)) {
         gb_workspace_pane_unfullscreen(GB_WORKSPACE_PANE(widget));
      }
   }

   gtk_notebook_set_show_tabs(GTK_NOTEBOOK(priv->notebook), TRUE);
}

static void
gb_workspace_pane_group_finalize (GObject *object)
{
   G_OBJECT_CLASS(gb_workspace_pane_group_parent_class)->finalize(object);
}

static void
gb_workspace_pane_group_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
   //GbWorkspacePaneGroup *group = GB_WORKSPACE_PANE_GROUP(object);

   switch (prop_id) {
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
gb_workspace_pane_group_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
   //GbWorkspacePaneGroup *group = GB_WORKSPACE_PANE_GROUP(object);

   switch (prop_id) {
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
gb_workspace_pane_group_class_init (GbWorkspacePaneGroupClass *klass)
{
   GObjectClass *object_class;
   GtkWidgetClass *widget_class;
   GtkContainerClass *container_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = gb_workspace_pane_group_finalize;
   object_class->get_property = gb_workspace_pane_group_get_property;
   object_class->set_property = gb_workspace_pane_group_set_property;
   g_type_class_add_private(object_class, sizeof(GbWorkspacePaneGroupPrivate));

   widget_class = GTK_WIDGET_CLASS(klass);
   widget_class->grab_focus = gb_workspace_pane_group_grab_focus;

   container_class = GTK_CONTAINER_CLASS(klass);
   container_class->add = gb_workspace_pane_group_add;
}

static void
gb_workspace_pane_group_init (GbWorkspacePaneGroup *group)
{
   GbWorkspacePaneGroupPrivate *priv;

   group->priv = G_TYPE_INSTANCE_GET_PRIVATE(group,
                                             GB_TYPE_WORKSPACE_PANE_GROUP,
                                             GbWorkspacePaneGroupPrivate);

   priv = group->priv;

   priv->notebook = g_object_new(GTK_TYPE_NOTEBOOK,
                                 "hexpand", TRUE,
                                 "scrollable", TRUE,
                                 "show-border", FALSE,
                                 "show-tabs", TRUE,
                                 "vexpand", TRUE,
                                 "visible", TRUE,
                                 NULL);
   gtk_container_add(GTK_CONTAINER(group), priv->notebook);
}
