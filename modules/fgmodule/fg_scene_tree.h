#ifndef FGSCENETREE_H
#define FGSCENETREE_H

#include "scene/main/scene_tree.h"

class FGSceneTree : public SceneTree
{
    GDCLASS(FGSceneTree, SceneTree);
public:
    virtual bool idle(float p_time);

    FGSceneTree();
    ~FGSceneTree();
};

#endif // FGSCENETREE_H
