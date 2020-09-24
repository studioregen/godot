#ifndef FG_COLLISION_NODE_EDITOR_H
#define FG_COLLISION_NODE_EDITOR_H

#include "editor/editor_node.h"
#include "editor/editor_plugin.h"

class FGCollisionNode;
class CanvasItemEditor;
class CollisionBox;

class FGCollisionNodeEditor : public HBoxContainer {
    GDCLASS(FGCollisionNodeEditor, HBoxContainer);

    enum EditorMode {
        AddBox,
        RemoveBox,
        TranslateBox
    };

    EditorMode active_mode;

    ToolButton *button_add;
    ToolButton *button_remove;

    ToolButton *button_autokey;

    FGCollisionNode* _editing;
    CanvasItemEditor* canvas_item_editor;

    Vector<Vector2> addition_wip;
    Ref<CollisionBox> mouse_over;
    bool auto_key_enabled;

protected:
    static void _bind_methods();
    void _notification(int p_what);
    void set_editing(FGCollisionNode* p_node);
    FGCollisionNode* get_editing() const;

    void _update_viewport();
    void _on_hide();
    void _commit_addition();
    void _commit_remove();
    void _toggle_autokey();
    bool _active_addition_in_progress() const { return addition_wip.size() >= 2; }

    void _try_auto_key();

    void _tool_pressed(int p_option);
public:
    virtual void edit(FGCollisionNode* node);
    virtual void forward_canvas_draw_over_viewport(Control* p_overlay) const;
    virtual bool forward_gui_input(const Ref<InputEvent> &p_event);

    FGCollisionNodeEditor();
    ~FGCollisionNodeEditor();
};

class FGCollisionNodeEditorPlugin : public EditorPlugin {
    GDCLASS(FGCollisionNodeEditorPlugin, EditorPlugin);

    FGCollisionNodeEditor *fg_collision_node_editor;
    EditorNode *editor;

public:
    virtual String get_name() const { return "FGCollisionNode"; }
    bool has_main_screen() const { return false; }
    virtual void edit(Object *p_object);
    virtual bool handles(Object *p_object) const;
    virtual void make_visible(bool p_visible);
    virtual void forward_canvas_draw_over_viewport(Control *p_overlay);
    virtual bool forward_canvas_gui_input(const Ref<InputEvent> &p_event);

    FGCollisionNodeEditorPlugin(EditorNode *p_node);
    ~FGCollisionNodeEditorPlugin();
};

#endif // FG_COLLISION_NODE_EDITOR_H
