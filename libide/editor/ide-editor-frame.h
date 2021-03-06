/* ide-editor-frame.h
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
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

#ifndef IDE_EDITOR_FRAME_H
#define IDE_EDITOR_FRAME_H

#include <gtk/gtk.h>

#include "buffers/ide-buffer.h"
#include "sourceview/ide-source-view.h"

G_BEGIN_DECLS

#define IDE_TYPE_EDITOR_FRAME (ide_editor_frame_get_type())

G_DECLARE_FINAL_TYPE (IdeEditorFrame, ide_editor_frame, IDE, EDITOR_FRAME, GtkBin)

IdeBuffer       *ide_editor_frame_get_document      (IdeEditorFrame *self);
void             ide_editor_frame_set_document      (IdeEditorFrame *self,
                                                     IdeBuffer      *buffer);
IdeSourceView   *ide_editor_frame_get_source_view   (IdeEditorFrame *self);

G_END_DECLS

#endif /* IDE_EDITOR_FRAME_H */
