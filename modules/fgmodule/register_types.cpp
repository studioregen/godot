#include "register_types.h"

#include "core/class_db.h"
#include "fg_collision_node.h"
#include "fg_collision_node_editor.h"

void register_fgmodule_types() {
    ClassDB::register_class<FGCollisionNode>();
    ClassDB::register_class<CollisionBox>();

#ifdef TOOLS_ENABLED
    EditorPlugins::add_by_type<FGCollisionNodeEditorPlugin>();
#endif
}

void unregister_fgmodule_types() {
    //nothing to do here
}

