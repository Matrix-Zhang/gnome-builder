/* gb-source-auto-indenter-xml.c
 *
 * Copyright (C) 2014 Christian Hergert <christian@hergert.me>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define G_LOG_DOMAIN "ide-xml-indenter"

#include <gtksourceview/gtksource.h>
#include <string.h>

#include "ide-xml-indenter.h"

#define ENTRY
#define EXIT return
#define GOTO(_l) goto _l
#define RETURN(_v) return _v

struct _IdeXmlIndenter
{
  IdeIndenter parent_class;
};

/*
 * TODO:
 *
 * This is very naive. But let's see if it gets the job done enough to not
 * be super annoying. Things like indent_width belong as fields in a private
 * structure.
 *
 * Anywho, if you want to own this module, go for it.
 */

#define INDENT_WIDTH 2

G_DEFINE_TYPE (IdeXmlIndenter, ide_xml_indenter, IDE_TYPE_INDENTER)

static gunichar
text_iter_peek_next_char (const GtkTextIter *location)
{
  GtkTextIter iter = *location;

  if (gtk_text_iter_forward_char (&iter))
    return gtk_text_iter_get_char (&iter);

  return 0;
}

static gunichar
text_iter_peek_prev_char (const GtkTextIter *location)
{
  GtkTextIter iter = *location;

  if (gtk_text_iter_backward_char (&iter))
    return gtk_text_iter_get_char (&iter);

  return 0;
}

static void
build_indent (IdeXmlIndenter *xml,
              guint                    line_offset,
              GtkTextIter             *matching_line,
              GString                 *str)
{
  GtkTextIter iter;
  gunichar ch;

  if (!line_offset)
    return;

  gtk_text_buffer_get_iter_at_line (gtk_text_iter_get_buffer (matching_line),
                                    &iter,
                                    gtk_text_iter_get_line (matching_line));

  do {
    ch = gtk_text_iter_get_char (&iter);

    switch (ch)
      {
      case '\t':
      case ' ':
        g_string_append_unichar (str, ch);
        break;

      default:
        g_string_append_c (str, ' ');
        break;
      }
  } while (gtk_text_iter_forward_char (&iter) &&
           (gtk_text_iter_compare (&iter, matching_line) <= 0) &&
           (str->len < line_offset));

  while (str->len < line_offset)
    g_string_append_c (str, ' ');
}

static gboolean
text_iter_in_cdata (const GtkTextIter *location)
{
  GtkTextIter iter = *location;
  gboolean ret = FALSE;

  if (gtk_text_iter_backward_search (&iter, "<![CDATA[",
                                     GTK_TEXT_SEARCH_TEXT_ONLY,
                                     NULL, &iter, NULL))
    {
      if (!gtk_text_iter_forward_search (&iter, "]]>",
                                         GTK_TEXT_SEARCH_TEXT_ONLY,
                                         NULL, NULL, location))
        {
          ret = TRUE;
          GOTO (cleanup);
        }
    }

cleanup:
  return ret;
}

static gboolean
text_iter_backward_to_element_start (const GtkTextIter *iter,
                                     GtkTextIter       *match_begin)
{
  GtkTextIter tmp = *iter;
  gboolean ret = FALSE;
  gint depth = 0;

  g_return_val_if_fail (iter, FALSE);
  g_return_val_if_fail (match_begin, FALSE);

  while (gtk_text_iter_backward_char (&tmp))
    {
      gunichar ch;

      ch = gtk_text_iter_get_char (&tmp);

      if ((ch == '/') && (text_iter_peek_prev_char (&tmp) == '<'))
        {
          gtk_text_iter_backward_char (&tmp);
          depth++;
        }
      else if ((ch == '/') && (text_iter_peek_next_char (&tmp) == '>'))
        {
          depth++;
        }
      else if ((ch == '<') && (text_iter_peek_next_char (&tmp) != '!'))
        {
          depth--;
          if (depth < 0)
            {
              *match_begin = tmp;
              ret = TRUE;
              GOTO (cleanup);
            }
        }
    }

cleanup:
  return ret;
}

static gchar *
ide_xml_indenter_indent (IdeXmlIndenter *xml,
                         GtkTextIter    *begin,
                         GtkTextIter    *end,
                         gint           *cursor_offset,
                         guint           tab_width)
{
  GtkTextIter match_begin;
  GString *str;
  guint offset;

  g_return_val_if_fail (IDE_IS_XML_INDENTER (xml), NULL);
  g_return_val_if_fail (begin, NULL);
  g_return_val_if_fail (end, NULL);

  str = g_string_new (NULL);

  if (text_iter_backward_to_element_start (begin, &match_begin))
    {
      offset = gtk_text_iter_get_line_offset (&match_begin);
      build_indent (xml, offset + INDENT_WIDTH, &match_begin, str);

      /*
       * If immediately after our cursor is a closing tag, we will move it to
       * a line after our indent line.
       */
      if ('<' == gtk_text_iter_get_char (end) &&
          '/' == text_iter_peek_next_char (end))
        {
          GString *str2;

          str2 = g_string_new (NULL);
          build_indent (xml, offset, &match_begin, str2);

          g_string_append (str, "\n");
          g_string_append (str, str2->str);

          *cursor_offset = -str2->len - 1;

          g_string_free (str2, TRUE);
        }

      GOTO (cleanup);
    }

  /* do nothing */

cleanup:
  return g_string_free (str, (str->len == 0));
}

static gchar *
ide_xml_indenter_maybe_unindent (IdeXmlIndenter *xml,
                                            GtkTextIter             *begin,
                                            GtkTextIter             *end)
{
  GtkTextIter tmp;
  gunichar ch;

  g_return_val_if_fail (IDE_IS_XML_INDENTER (xml), NULL);
  g_return_val_if_fail (begin, NULL);
  g_return_val_if_fail (end, NULL);

  tmp = *begin;

  if (!gtk_text_iter_backward_char (&tmp))
    return NULL;

  if (('/' == gtk_text_iter_get_char (&tmp)) &&
      gtk_text_iter_backward_char (&tmp) &&
      ('<' == gtk_text_iter_get_char (&tmp)) &&
      (ch = text_iter_peek_prev_char (&tmp)) &&
      ((ch == ' ') || (ch == '\t')))
    {
      if (ch == '\t')
        {
          gtk_text_iter_backward_char (&tmp);
          *begin = tmp;
          return g_strdup ("</");
        }
      else
        {
          gint count = INDENT_WIDTH;

          while (count)
            {
              if (!gtk_text_iter_backward_char (&tmp) ||
                  !(ch = gtk_text_iter_get_char (&tmp)) ||
                  (ch != ' '))
                return NULL;
              count--;
              if (count == 0)
                GOTO (success);
            }
        }
    }

  return NULL;

success:
  *begin = tmp;
  return g_strdup ("</");
}

static gboolean
find_end (gunichar ch,
          gpointer user_data)
{
  return (ch == '>' || g_unichar_isspace (ch));
}

static gchar *
ide_xml_indenter_maybe_add_closing (IdeXmlIndenter *xml,
                                               GtkTextIter             *begin,
                                               GtkTextIter             *end,
                                               gint                    *cursor_offset)
{
  GtkTextIter match_begin;
  GtkTextIter match_end;
  GtkTextIter copy;

  g_return_val_if_fail (IDE_IS_XML_INDENTER (xml), NULL);
  g_return_val_if_fail (begin, NULL);
  g_return_val_if_fail (end, NULL);

  copy = *begin;

  gtk_text_iter_backward_char (&copy);
  gtk_text_iter_backward_char (&copy);

  if (gtk_text_iter_get_char (&copy) == '/')
    return NULL;

  copy = *begin;

  if (gtk_text_iter_backward_search (&copy, "<", GTK_TEXT_SEARCH_TEXT_ONLY,
                                     &match_begin, &match_end, NULL))
    {
      gchar *text;

      /* avoid closing elements on spurious > */
      gtk_text_iter_backward_char (&copy);
      text = gtk_text_iter_get_slice (&match_begin, &copy);
      if (strchr (text, '>'))
        {
          g_free (text);
          return NULL;
        }
      g_free (text);

      gtk_text_iter_forward_char (&match_begin);
      if (gtk_text_iter_get_char (&match_begin) == '/')
        return NULL;

      match_end = match_begin;

      if (gtk_text_iter_forward_find_char (&match_end, find_end, NULL, begin))
        {
          gchar *slice;
          gchar *ret = NULL;

          slice = gtk_text_iter_get_slice (&match_begin, &match_end);

          if (slice && *slice && *slice != '!')
            {
              ret = g_strdup_printf ("</%s>", slice);
              *cursor_offset = -strlen (ret);
            }

          g_free (slice);

          return ret;
        }
    }

  return NULL;
}

static gboolean
ide_xml_indenter_is_trigger (IdeIndenter *indenter,
                             GdkEventKey *event)
{
  switch (event->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_slash:
    case GDK_KEY_greater:
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static gchar *
ide_xml_indenter_format (IdeIndenter *indenter,
                         GtkTextView *view,
                         GtkTextIter *begin,
                         GtkTextIter *end,
                         gint        *cursor_offset,
                         GdkEventKey *trigger)
{
  IdeXmlIndenter *xml = (IdeXmlIndenter *)indenter;
  guint tab_width = 2;
  gint indent_width = -1;

  g_return_val_if_fail (IDE_IS_XML_INDENTER (xml), NULL);

  *cursor_offset = 0;

  if (GTK_SOURCE_IS_VIEW (view))
    {
      tab_width = gtk_source_view_get_tab_width (GTK_SOURCE_VIEW (view));
      indent_width = gtk_source_view_get_indent_width (GTK_SOURCE_VIEW (view));
      if (indent_width != -1)
        tab_width = indent_width;
    }

  /* do nothing if we are in a cdata section */
  if (text_iter_in_cdata (begin))
    return NULL;

  switch (trigger->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      if ((trigger->state & GDK_SHIFT_MASK) == 0)
        return ide_xml_indenter_indent (xml, begin, end, cursor_offset,
                                        tab_width);
      return NULL;

    case GDK_KEY_slash:
      return ide_xml_indenter_maybe_unindent (xml, begin, end);

    case GDK_KEY_greater:
      return ide_xml_indenter_maybe_add_closing (xml, begin, end,
                                                 cursor_offset);

    default:
      g_return_val_if_reached (NULL);
    }

  return NULL;
}

static void
ide_xml_indenter_class_init (IdeXmlIndenterClass *klass)
{
  IdeIndenterClass *parent = IDE_INDENTER_CLASS (klass);

  parent->format = ide_xml_indenter_format;
  parent->is_trigger = ide_xml_indenter_is_trigger;
}

static void
ide_xml_indenter_init (IdeXmlIndenter *self)
{
}
