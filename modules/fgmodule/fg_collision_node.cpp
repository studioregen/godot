#include "fg_collision_node.h"

#include "core/engine.h"
#include "scene/scene_string_names.h"


void CollisionBox::set_x(int p_x)
{
    x = p_x;
    emit_changed();
}

int CollisionBox::get_x() const
{
    return x;
}

void CollisionBox::set_y(int p_y)
{
    y = p_y;
    emit_changed();
}

int CollisionBox::get_y() const
{
    return y;
}

void CollisionBox::set_width(int p_width)
{
    width = p_width;
    emit_changed();
}

int CollisionBox::get_width() const
{
    return width;
}

void CollisionBox::set_height(int p_height)
{
    height = p_height;
    emit_changed();
}

int CollisionBox::get_height() const
{
    return height;
}

void CollisionBox::_bind_methods()
{
    ClassDB::bind_method(D_METHOD("set_x", "x"), &CollisionBox::set_x);
    ClassDB::bind_method(D_METHOD("get_x"), &CollisionBox::get_x);
    ClassDB::bind_method(D_METHOD("set_y", "y"), &CollisionBox::set_y);
    ClassDB::bind_method(D_METHOD("get_y"), &CollisionBox::get_y);
    ClassDB::bind_method(D_METHOD("set_width", "width"), &CollisionBox::set_width);
    ClassDB::bind_method(D_METHOD("get_width"), &CollisionBox::get_width);
    ClassDB::bind_method(D_METHOD("set_height", "height"), &CollisionBox::set_height);
    ClassDB::bind_method(D_METHOD("get_height"), &CollisionBox::get_height);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "x"), "set_x", "get_x");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "y"), "set_y", "get_y");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "set_width", "get_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "set_height", "get_height");
}


FGCollisionNode::FGCollisionNode(){
    area_id = Physics2DServer::get_singleton()->area_create();
    Physics2DServer::get_singleton()->area_attach_object_instance_id(area_id, get_instance_id());
    set_notify_transform(true);
    set_notify_local_transform(true);
    layer = 0;
    mask = 0;
    locked = false;
}

FGCollisionNode::~FGCollisionNode(){
    Physics2DServer::get_singleton()->free(area_id);
}

void FGCollisionNode::_notification(int p_what) {
    switch (p_what) {

    case NOTIFICATION_POSTINITIALIZE:{
    } break;

    case NOTIFICATION_ENTER_TREE:{

        Transform2D global_transform = get_global_transform();
        Physics2DServer::get_singleton()->area_set_transform(area_id, global_transform);
        RID space = get_world_2d()->get_space();
        Physics2DServer::get_singleton()->area_set_space(area_id, space);
    } break;

    case NOTIFICATION_ENTER_CANVAS: {

        Physics2DServer::get_singleton()->area_attach_canvas_instance_id(area_id, get_canvas_layer_instance_id());
    } break;

    case NOTIFICATION_TRANSFORM_CHANGED:{

        Transform2D global_transform = get_global_transform();
        Physics2DServer::get_singleton()->area_set_transform(area_id, global_transform);
    } break;

    case NOTIFICATION_EXIT_TREE:{

        Physics2DServer::get_singleton()->area_set_space(area_id, RID());
    } break;

    case NOTIFICATION_DRAW:{

        if (/*Engine::get_singleton()->is_editor_hint()*/ true) {
            for(List<Ref<CollisionBox>>::Element *E = hitboxes.front(); E; E = E->next()) {
                if (E->get().is_null()) continue;
                draw_rect(Rect2(E->get()->get_x(), E->get()->get_y(), E->get()->get_width(), E->get()->get_height()), Color(1.0, 1.0, 1.0), false);
            }
        }
    } break;

    case NOTIFICATION_EXIT_CANVAS: {

        Physics2DServer::get_singleton()->area_attach_canvas_instance_id(area_id, 0);
    } break;

    }
}

Ref<CollisionBox> FGCollisionNode::add_box(int p_x, int p_y, int p_width, int p_height, FGCollisionNode::BoxType p_type)
{
    List<Ref<CollisionBox>> boxes = hitboxes;
    Ref<CollisionBox> box( memnew(CollisionBox) );
    box->set_x(p_x);
    box->set_y(p_y);
    box->set_width(p_width);
    box->set_height(p_height);
    boxes.push_back(box);

    rewrite_box_list(hitboxes, boxes);
    update();
    return box;
}

void FGCollisionNode::remove_box(Ref<CollisionBox> p_box) {
    List<Ref<CollisionBox>> boxes = hitboxes;

    List<Ref<CollisionBox>>::Element *e = boxes.find(p_box);
    if (e) {
        e->erase();
    }

    rewrite_box_list(hitboxes, boxes);
    update();
}

void FGCollisionNode::set_layer_flags(uint32_t p_flags)
{
    Physics2DServer* server = Physics2DServer::get_singleton();
    layer = p_flags;
    server->area_set_collision_layer(area_id, layer);
}

uint32_t FGCollisionNode::get_layer_flags() const
{
    return layer;
}

void FGCollisionNode::set_mask_flags(uint32_t p_flags)
{
    Physics2DServer* server = Physics2DServer::get_singleton();
    mask = p_flags;
    server->area_set_collision_mask(area_id, mask);
}

uint32_t FGCollisionNode::get_mask_flags() const
{
    return mask;
}

void FGCollisionNode::set_monitoring(bool p_enable)
{
    if (p_enable == monitoring)
        return;
    ERR_FAIL_COND_MSG(locked, "Function blocked during in/out signal. Use set_deferred(\"monitoring\", true/false).");

    monitoring = p_enable;

    if (monitoring) {
        Physics2DServer::get_singleton()->area_set_monitor_callback(area_id, this, "test_body_callback");
        Physics2DServer::get_singleton()->area_set_area_monitor_callback(area_id, this, "test_area_callback");
    } else {
        Physics2DServer::get_singleton()->area_set_monitor_callback(area_id, NULL, StringName());
        Physics2DServer::get_singleton()->area_set_area_monitor_callback(area_id, NULL, StringName());
    }
}

bool FGCollisionNode::is_monitoring() const
{
    return monitoring;
}

void FGCollisionNode::set_monitorable(bool p_enable)
{
    ERR_FAIL_COND_MSG(locked || (is_inside_tree() && Physics2DServer::get_singleton()->is_flushing_queries()), "Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");

    if (p_enable == monitorable)
        return;

    monitorable = p_enable;

    Physics2DServer::get_singleton()->area_set_monitorable(area_id, monitorable);
}

bool FGCollisionNode::is_monitorable() const
{
    return monitorable;
}

void FGCollisionNode::test_body_callback(Physics2DServer::AreaBodyStatus status, RID entered_rid, int p_instance, int p_body_shape, int p_area_shape)
{
    print_line("mememe");
}

void FGCollisionNode::test_area_callback(Physics2DServer::AreaBodyStatus status, RID entered_rid, int p_instance, int p_body_shape, int p_area_shape)
{
    Array a;
    a.append(this->get_name());
    print_line(String("momomo {0}").format(a));
}

void FGCollisionNode::_bind_methods()
{

    ClassDB::bind_method(D_METHOD("set_hit_boxes", "hit_boxes"), &FGCollisionNode::set_hit_box_variants);
    ClassDB::bind_method(D_METHOD("get_hit_boxes"), &FGCollisionNode::get_hit_box_variants);

    ClassDB::bind_method(D_METHOD("set_layer_flags", "flags"), &FGCollisionNode::set_layer_flags);
    ClassDB::bind_method(D_METHOD("get_layer_flags"), &FGCollisionNode::get_layer_flags);

    ClassDB::bind_method(D_METHOD("set_mask_flags", "flags"), &FGCollisionNode::set_mask_flags);
    ClassDB::bind_method(D_METHOD("get_mask_flags"), &FGCollisionNode::get_mask_flags);

    ClassDB::bind_method(D_METHOD("set_monitoring", "enable"), &FGCollisionNode::set_monitoring);
    ClassDB::bind_method(D_METHOD("is_monitoring"), &FGCollisionNode::is_monitoring);

    ClassDB::bind_method(D_METHOD("set_monitorable", "enable"), &FGCollisionNode::set_monitorable);
    ClassDB::bind_method(D_METHOD("is_monitorable"), &FGCollisionNode::is_monitorable);

    ClassDB::bind_method(D_METHOD("test_body_callback", "status", "rid", "p_instance", "p_body_shape", "p_area_shape"), &FGCollisionNode::test_body_callback);
    ClassDB::bind_method(D_METHOD("test_area_callback", "status", "rid", "p_instance", "p_body_shape", "p_area_shape"), &FGCollisionNode::test_area_callback);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitoring"), "set_monitoring", "is_monitoring");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "monitorable"), "set_monitorable", "is_monitorable");

    ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "collision_hit_boxes", PROPERTY_HINT_RESOURCE_TYPE, "17/17:CollisionBox", (PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_SCRIPT_VARIABLE), "CollisionBox"), "set_hit_boxes", "get_hit_boxes");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_layer", PROPERTY_HINT_LAYERS_2D_PHYSICS), "set_layer_flags", "get_layer_flags");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "collision_mask", PROPERTY_HINT_LAYERS_2D_PHYSICS), "set_mask_flags", "get_mask_flags");
}

void FGCollisionNode::rewrite_box_list(List<Ref<CollisionBox>> &list_to_change, const List<Ref<CollisionBox>> &reference_list)
{
    Physics2DServer* physics_engine = Physics2DServer::get_singleton();
    physics_engine->area_clear_shapes(area_id);

    for (const List<RID>::Element *shape = shapes.front(); shape; shape = shape->next()) {
        physics_engine->free(shape->get());
    }

    list_to_change.clear();
    shapes.clear();

    for (const List<Ref<CollisionBox>>::Element *ref = reference_list.front(); ref; ref = ref->next()) {
        list_to_change.push_back(ref->get());

        if (ref->get().is_null()) continue;

        RID rect = physics_engine->rectangle_shape_create();
        const Vector2 halfext = Vector2(ref->get()->get_width()/2, ref->get()->get_height()/2);
        const Vector2 origin = Vector2(ref->get()->get_x(), ref->get()->get_y()) + halfext;
        physics_engine->shape_set_data(rect, halfext);
        physics_engine->area_add_shape(area_id, rect, Transform2D(0.0f, origin));
        shapes.push_back(rect);
    }

    for (List<Ref<CollisionBox>>::Element *e = list_to_change.front(); e; e = e->next()) {
        if (e->get().is_null()) continue;
        if (!e->get()->is_connected("changed", this, "update")) {
            e->get()->connect("changed", this, "update");
        }
    }
}

// =========== AREA_2D COPY ============

