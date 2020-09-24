#ifndef FG_COLLISION_NODE_H
#define FG_COLLISION_NODE_H

#include "scene/2d/node_2d.h"
#include "core/variant.h"
#include "core/list.h"
#include "core/map.h"
#include "core/vset.h"

class CollisionBox : public Resource {
    GDCLASS(CollisionBox, Resource)

public:

    void set_x(int p_x);
    int get_x() const;

    void set_y(int p_y);
    int get_y() const;

    void set_width(int p_width);
    int get_width() const;

    void set_height(int p_height);
    int get_height() const;

    CollisionBox() {
        x = 0;
        y = 0;
        width = 0;
        height = 0;
    }

protected:
    static void _bind_methods();

private:
    int x;
    int y;
    int width;
    int height;
};

class FGCollisionNode : public Node2D {
    GDCLASS(FGCollisionNode, Node2D)

    enum BoxType {
        HITBOX,
        HURTBOX
    };

public:
    FGCollisionNode();
    ~FGCollisionNode();

    void _notification(int p_what);

    Ref<CollisionBox> add_box(int p_x, int p_y, int p_width, int p_height);
    void remove_box(Ref<CollisionBox> p_box);

    void set_hit_boxes(List<Ref<CollisionBox>> boxes) {
        rewrite_box_list(hitboxes, boxes);
        emit_signal("changed");
        update();
    }

    void set_hit_box_variants(const Vector<Variant> boxes) {
        List<Ref<CollisionBox>> vals;
        for (int i = 0; i < boxes.size(); i++) {
            Ref<CollisionBox> effect = Ref<CollisionBox>(boxes[i]);
            vals.push_back(effect);
        }

        set_hit_boxes(vals);
    }

    List<Ref<CollisionBox>> get_hit_boxes() const {
        return hitboxes;
    }

    Vector<Variant> get_hit_box_variants() const {
        Vector<Variant> r;
        for (const List<Ref<CollisionBox>>::Element *E = hitboxes.front(); E; E = E->next()) {
            r.push_back(E->get().get_ref_ptr());
        }
        return r;
    }

    Ref<CollisionBox> query_point_for_collision_box(Vector2i pos);

    void set_layer_flags(uint32_t p_flags);
    uint32_t get_layer_flags() const;

    void set_mask_flags(uint32_t p_flags);
    uint32_t get_mask_flags() const;

    void set_monitoring(bool p_enable);
    bool is_monitoring() const;

    void set_monitorable(bool p_enable);
    bool is_monitorable() const;

    void body_in_out(Physics2DServer::AreaBodyStatus status, RID entered_rid, int p_instance, int p_body_shape, int p_area_shape );
    void area_in_out(Physics2DServer::AreaBodyStatus status, RID entered_rid, int p_instance, int p_body_shape, int p_area_shape);
    void _remove_area_interrupt(int p_instance);

    int get_overlap_count();
    Array get_overlapping_nodes();

    List<Ref<CollisionBox>> get_box_list() { return hitboxes; }

    _FORCE_INLINE_ RID get_rid() const { return area_id; }
protected:
    static void _bind_methods();

private:
    // Implementation for set_hit_boxes and set_hurt_boxes, should preserve references and remove the unused refs.
    void rewrite_box_list(List<Ref<CollisionBox>> &list_to_change, const List<Ref<CollisionBox>> &reference_list);

    List<Ref<CollisionBox>> hitboxes;
    List<RID> shapes;
    RID area_id;

    int layer;
    int mask;

    bool monitoring;
    bool monitorable;

    bool locked;
    List<ObjectID> overlapping_areas;

    bool debug_draw;
    // ========== COPY FROM AREA_2D ============ //
};

#endif // FG_COLLISION_NODE_H
