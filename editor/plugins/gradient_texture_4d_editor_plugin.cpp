/**************************************************************************/
/*  gradient_texture_2d_editor_plugin.cpp                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "gradient_texture_4d_editor_plugin.h"

#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_undo_redo_manager.h"
#include "scene/gui/box_container.h"
#include "scene/gui/flow_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/color_picker.h"

void GradientTexture4DEditorRect::_notification(int p_what) {
	if (p_what == NOTIFICATION_DRAW) {
		if (texture.is_null()) {
			return;
		}
		
		// Get the size and position to draw the texture and handles at.
		// Subtract handle sizes so they stay inside the preview, but keep the texture's aspect ratio.
		Size2 rect_size = get_size();
		Size2 available_size = rect_size - handle_size;
		Size2 ratio = available_size / texture->get_size();
		size = MIN(ratio.x, ratio.y) * texture->get_size();
		offset = ((rect_size - size) / 2).round();

		checkerboard->set_rect(Rect2(offset, size));

		draw_set_transform(offset);
		draw_texture_rect(texture, Rect2(Point2(), size));
	}
}

void GradientTexture4DEditorRect::set_texture(Ref<GradientTexture4D> &p_texture) {
	texture = p_texture;
	texture->connect("changed", callable_mp((CanvasItem *)this, &CanvasItem::queue_redraw));
}

GradientTexture4DEditorRect::GradientTexture4DEditorRect() {
	checkerboard = memnew(TextureRect);
	checkerboard->set_stretch_mode(TextureRect::STRETCH_TILE);
	checkerboard->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	checkerboard->set_draw_behind_parent(true);
	add_child(checkerboard, false, INTERNAL_MODE_FRONT);

	set_custom_minimum_size(Size2(0, 250 * EDSCALE));
}

///////////////////////

void GradientTexture4DEditor::set_texture(Ref<GradientTexture4D> &p_texture) {
	texture = p_texture;
	texture_editor_rect->set_texture(p_texture);

	Vector<StringName> names = {"a", "b", "c", "d"};
	Dictionary colors = texture->get_colors();
	for (int i = 0; i < COLOR_COUNT; i++) {
		buttons[i]->set_pick_color(colors[names[i]]);
	}
}

void GradientTexture4DEditor::set_colors(const Color &a, const Color &b, const Color &c, const Color &d) {
	texture->set_colors(a,b,c,d);
}

void GradientTexture4DEditor::set_color(const Color &a, const ColorQuadType type) {
	Dictionary colors = texture->get_colors();
	colors["a"] = type == UPPER_LEFT ? a : static_cast<Color>(colors["a"]);
	colors["b"] = type == UPPER_RIGHT ? a : static_cast<Color>(colors["b"]);
	colors["c"] = type == LOWER_RIGHT ? a : static_cast<Color>(colors["c"]);
	colors["d"] = type == LOWER_LEFT ? a : static_cast<Color>(colors["d"]);
	texture->set_colors(colors["a"], colors["b"], colors["c"], colors["d"]);
}

GradientTexture4DEditor::GradientTexture4DEditor() {
	HFlowContainer *toolbar = memnew(HFlowContainer);
	add_child(toolbar);

	VBoxContainer *content = memnew(VBoxContainer);
	add_child(content);

	content->set_h_size_flags(SizeFlags::SIZE_EXPAND_FILL);
	texture_editor_rect = memnew(GradientTexture4DEditorRect);
	content->add_child(texture_editor_rect);

	for (int i = 0; i < COLOR_COUNT; i++) {
		buttons[i] = memnew(ColorPickerButton);
		buttons[i]->connect("color_changed", callable_mp((GradientTexture4DEditor*)this, &GradientTexture4DEditor::set_color).bind(i));
		buttons[i]->set_custom_minimum_size(Size2(0, 30 * EDSCALE));
		content->add_child(buttons[i]);
	}

	set_mouse_filter(MOUSE_FILTER_STOP);
}

///////////////////////

bool EditorInspectorPluginGradientTexture4D::can_handle(Object *p_object) {
	return Object::cast_to<GradientTexture4D>(p_object) != nullptr;
}

void EditorInspectorPluginGradientTexture4D::parse_begin(Object *p_object) {
	GradientTexture4D *texture = Object::cast_to<GradientTexture4D>(p_object);
	if (!texture) {
		return;
	}
	Ref<GradientTexture4D> t(texture);

	GradientTexture4DEditor *editor = memnew(GradientTexture4DEditor);
	editor->set_texture(t);
	add_custom_control(editor);
}

///////////////////////

GradientTexture4DEditorPlugin::GradientTexture4DEditorPlugin() {
	Ref<EditorInspectorPluginGradientTexture4D> plugin;
	plugin.instantiate();
	add_inspector_plugin(plugin);
}
