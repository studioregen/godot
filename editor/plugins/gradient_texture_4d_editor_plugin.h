/**************************************************************************/
/*  gradient_texture_2d_editor_plugin.h                                   */
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

#ifndef GRADIENT_TEXTURE_4D_EDITOR_PLUGIN_H
#define GRADIENT_TEXTURE_4D_EDITOR_PLUGIN_H

#include "editor/editor_inspector.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/resources/gradient_texture.h"

class GradientTexture4DEditorRect : public Control {
	GDCLASS(GradientTexture4DEditorRect, Control);

	enum Handle {
		HANDLE_NONE = -1,
		HANDLE_UP_LEFT,
		HANDLE_UP_RIGHT,
		HANDLE_DOWN_RIGHT,
		HANDLE_DOWN_LEFT,
		HANDLE_MAX
	};

	Ref<GradientTexture4D> texture;

	TextureRect *checkerboard = nullptr;

	Size2 handle_size;
	Point2 offset;
	Size2 size;

protected:
	void _notification(int p_what);

public:
	void set_texture(Ref<GradientTexture4D> &p_texture);

	GradientTexture4DEditorRect();
};

class GradientTexture4DEditor : public VBoxContainer {
	GDCLASS(GradientTexture4DEditor, VBoxContainer);

public:
	enum ColorQuadType {
		INVALID = -1,
		UPPER_LEFT,
		UPPER_RIGHT,
		LOWER_RIGHT,
		LOWER_LEFT,
		COLOR_COUNT
	};

private:
	Ref<GradientTexture4D> texture;

	GradientTexture4DEditorRect *texture_editor_rect = nullptr;

	class ColorPickerButton *buttons[COLOR_COUNT];

public:
	void set_texture(Ref<GradientTexture4D> &p_texture);
	void set_colors(const Color &a, const Color &b, const Color &c, const Color &d);
	void set_color(const Color &a, const ColorQuadType type);

	GradientTexture4DEditor();
};

VARIANT_ENUM_CAST(GradientTexture4DEditor::ColorQuadType);

class EditorInspectorPluginGradientTexture4D : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginGradientTexture4D, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_begin(Object *p_object) override;
};

class GradientTexture4DEditorPlugin : public EditorPlugin {
	GDCLASS(GradientTexture4DEditorPlugin, EditorPlugin);

public:
	GradientTexture4DEditorPlugin();
};

#endif // GRADIENT_TEXTURE_4D_EDITOR_PLUGIN_H
