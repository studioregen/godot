#include "register_types.h"

#include "core/class_db.h"
#include "fg_collision_node.h"

void register_fgmodule_types() {
    ClassDB::register_class<FGCollisionNode>();
    ClassDB::register_class<CollisionBox>();
}

void unregister_fgmodule_types() {
    //nothing to do here
}

