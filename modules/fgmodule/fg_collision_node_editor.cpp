#include "fg_collision_node_editor.h"
#include "fg_collision_node.h"

#include "core/math/vector2.h"
#include "core/variant.h"

#include "editor/animation_track_editor.h"
#include "editor/plugins/canvas_item_editor_plugin.h"
#include "editor/plugins/animation_player_editor_plugin.h"


void FGCollisionNodeEditor::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("_update_viewport"), &FGCollisionNodeEditor::_update_viewport);
    ClassDB::bind_method(D_METHOD("_on_hide"), &FGCollisionNodeEditor::_on_hide);
    ClassDB::bind_method(D_METHOD("_tool_pressed", "p_mode"), &FGCollisionNodeEditor::_tool_pressed);
}

void FGCollisionNodeEditor::_notification(int p_what)
{
    if (p_what == NOTIFICATION_READY) {
        this->connect("hide", this, "_on_hide");
        button_add->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("CurveCreate", "EditorIcons"));
        button_remove->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("CurveDelete", "EditorIcons"));
        button_autokey->set_icon(EditorNode::get_singleton()->get_gui_base()->get_icon("AutoKey", "EditorIcons"));
    }
}

void FGCollisionNodeEditor::set_editing(FGCollisionNode *p_node)
{
    _editing = p_node;
}

FGCollisionNode* FGCollisionNodeEditor::get_editing() const
{
    return _editing;
}

void FGCollisionNodeEditor::_update_viewport()
{
    if (canvas_item_editor) {
        canvas_item_editor->update_viewport();
    }
}

void FGCollisionNodeEditor::_on_hide()
{
    button_add->set_pressed(false);
    button_remove->set_pressed(false);
}

void FGCollisionNodeEditor::_commit_addition()
{
    if (_active_addition_in_progress() && get_editing()) {

        const Vector2 min = addition_wip[0] < addition_wip[1] ? addition_wip[0] : addition_wip[1];
        const Vector2 max = addition_wip[0] < addition_wip[1] ? addition_wip[1] : addition_wip[0];
        get_editing()->add_box(min.x, min.y, max.x - min.x, max.y - min.y);

        if (button_autokey->is_pressed())
            _try_auto_key();
    }
    addition_wip.clear();
}

void FGCollisionNodeEditor::_commit_remove()
{
    if (get_editing() && mouse_over.is_valid()) {

        get_editing()->remove_box(mouse_over);
        mouse_over = Ref<CollisionBox>();

        if (button_autokey->is_pressed())
            _try_auto_key();
    }
}

void FGCollisionNodeEditor::_try_auto_key()
{
    if (get_editing()) {
        Vector<Variant> box_variants = get_editing()->get_hit_box_variants();
        AnimationPlayerEditor::singleton->get_track_editor()->insert_node_value_key(get_editing(), "collision_hit_boxes", box_variants);
    }
}

void FGCollisionNodeEditor::_tool_pressed(int p_option){
    active_mode = (EditorMode)p_option;
    button_add->set_pressed(active_mode == AddBox);
    button_remove->set_pressed(active_mode == RemoveBox);
}

void FGCollisionNodeEditor::edit(FGCollisionNode *node)
{
    if (!canvas_item_editor)
        canvas_item_editor = CanvasItemEditor::get_singleton();

    if (node == get_editing())
        return;

    if (get_editing())
        get_editing()->disconnect("changed", this, "_update_viewport");

    set_editing(node);

    if (get_editing())
        get_editing()->connect("changed", this, "_update_viewport");

    _update_viewport();
}

void FGCollisionNodeEditor::forward_canvas_draw_over_viewport(Control *p_overlay) const
{
    if (!get_editing())
        return;

    List<Ref<CollisionBox>> boxes = get_editing()->get_box_list();
    Ref<Font> font = get_font("font", "Label");
    const Transform2D localToCanvas = canvas_item_editor->get_canvas_transform() * get_editing()->get_global_transform();

    for (List<Ref<CollisionBox>>::Element *E = boxes.front(); E; E = E->next()) {

        if (!E->get().is_valid()) continue;

        Rect2 to_draw( E->get()->get_x(), E->get()->get_y(), E->get()->get_width(), E->get()->get_height());
        p_overlay->draw_rect(localToCanvas.xform(to_draw), get_editing()->get_modulate(), false, 2.0);

        if (mouse_over == E->get()) {

            Array a;
            a.append(E->get()->get_instance_id());
            p_overlay->draw_string(font, localToCanvas.xform(to_draw.position), String("#{0}").format(a), get_editing()->get_modulate());
        }
    }

    switch(active_mode) {
        case AddBox: {

            if (_active_addition_in_progress()) {

                const Vector2 min = addition_wip[0] < addition_wip[1] ? addition_wip[0] : addition_wip[1];
                const Vector2 max = addition_wip[0] < addition_wip[1] ? addition_wip[1] : addition_wip[0];
                Rect2 to_draw = Rect2(min, Size2(max.x - min.x, max.y - min.y));
                p_overlay->draw_rect(localToCanvas.xform(to_draw), Color(1.0,1.0,0.0), false, 1.5);
            }
        } break;
        default: {
        } break;
    }
}

bool FGCollisionNodeEditor::forward_gui_input(const Ref<InputEvent> &p_event)
{
    Ref<InputEventMouseButton> mb = p_event;
    Ref<InputEventMouse> me = p_event;

    if (!get_editing()) {
        return false;
    }

    if (me.is_valid() && get_editing()) {
        const Transform2D canvasToLocal = get_editing()->get_global_transform().affine_inverse() * canvas_item_editor->get_canvas_transform().affine_inverse();
        mouse_over = get_editing()->query_point_for_collision_box(canvasToLocal.xform(me->get_position()));
        _update_viewport();
    }

    switch(active_mode) {
    case AddBox : {
        if(me.is_valid() && get_editing()) {

            const Transform2D canvasToLocal = get_editing()->get_global_transform().affine_inverse() * canvas_item_editor->get_canvas_transform().affine_inverse();
            if (addition_wip.size() >= 2) {

                const Vector2 position = canvasToLocal.xform(me->get_position());
                addition_wip.write[1] = position;
                _update_viewport();
            }
        }

        if(mb.is_valid() && get_editing()) {

            const bool leftClicked = mb->get_button_index() == BUTTON_LEFT;
            const bool pressEvent = mb->is_pressed();
            const Transform2D canvasToLocal = get_editing()->get_global_transform().affine_inverse() * canvas_item_editor->get_canvas_transform().affine_inverse();
            if (leftClicked && pressEvent) {

                if (!_active_addition_in_progress()) {
                    const Vector2 position = canvasToLocal.xform(mb->get_position());
                    addition_wip.push_back(position);
                    addition_wip.push_back(position);
                    return true;
                } else {

                    _commit_addition();
                    return true;
                }
            } else if (leftClicked) {

                if (_active_addition_in_progress()){

                    _commit_addition();
                    return true;
                }
            }
        }
    } break;
    case RemoveBox : {
        if(mb.is_valid() && get_editing()) {

            const bool leftClicked = mb->get_button_index() == BUTTON_LEFT;
            const bool pressEvent = mb->is_pressed();
            if (leftClicked && pressEvent) {

                _commit_remove();
                return true;
            }
        }
    }
    default: {

    } break;
    }
    return false;
}

FGCollisionNodeEditor::FGCollisionNodeEditor()
    : HBoxContainer()
{
    active_mode = RemoveBox;

    add_child(memnew(Separator));

    button_add = memnew(ToolButton);
    add_child(button_add);
    button_add->set_toggle_mode(true);
    button_add->set_pressed(false);
    button_add->connect("pressed", this, "_tool_pressed", varray(AddBox));

    button_remove = memnew(ToolButton);
    add_child(button_remove);
    button_remove->set_toggle_mode(true);
    button_remove->set_pressed(false);
    button_remove->connect("pressed", this, "_tool_pressed", varray(RemoveBox));

    button_autokey = memnew(ToolButton);
    add_child(button_autokey);
    button_autokey->set_toggle_mode(true);
    button_autokey->set_pressed(true);

}

FGCollisionNodeEditor::~FGCollisionNodeEditor()
{

}

//========================

void FGCollisionNodeEditorPlugin::edit(Object *p_object)
{
    fg_collision_node_editor->edit(Object::cast_to<FGCollisionNode>(p_object));
}

bool FGCollisionNodeEditorPlugin::handles(Object *p_object) const
{
    bool willHandle = p_object->is_class(get_name());

    if (!willHandle) {
        fg_collision_node_editor->edit(nullptr);
    }

    return willHandle;
}

void FGCollisionNodeEditorPlugin::make_visible(bool p_visible)
{
    if (p_visible) {
        fg_collision_node_editor->show();
    } else {
        fg_collision_node_editor->hide();
    }
}

void FGCollisionNodeEditorPlugin::forward_canvas_draw_over_viewport(Control *p_overlay) {
    fg_collision_node_editor->forward_canvas_draw_over_viewport(p_overlay);
}

bool FGCollisionNodeEditorPlugin::forward_canvas_gui_input(const Ref<InputEvent> &p_event)
{
    return fg_collision_node_editor->forward_gui_input(p_event);
}

FGCollisionNodeEditorPlugin::FGCollisionNodeEditorPlugin(EditorNode *p_node)
    : fg_collision_node_editor(memnew(FGCollisionNodeEditor())) {
    CanvasItemEditor::get_singleton()->add_control_to_menu_panel(fg_collision_node_editor);
    fg_collision_node_editor->hide();
}

FGCollisionNodeEditorPlugin::~FGCollisionNodeEditorPlugin()
{

}
