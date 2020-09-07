#include "fg_scene_tree.h"

FGSceneTree::FGSceneTree()
{
    print_line("TEST");
}

FGSceneTree::~FGSceneTree()
{

}

bool FGSceneTree::idle(float p_time)
{
    print_line("BEFORE SCENE TICK");
    bool val = SceneTree::idle(p_time);
    print_line("AFTER_SCENE_TICK");
    return val;
}
