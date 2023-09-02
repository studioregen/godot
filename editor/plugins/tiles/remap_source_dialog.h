#ifndef RENAME_SOURCE_DIALOG_H
#define RENAME_SOURCE_DIALOG_H

#include "editor/editor_properties.h"
#include "scene/2d/tile_map.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/tree.h"


class RemapSourceDialog : public ConfirmationDialog {
	GDCLASS(RemapSourceDialog, ConfirmationDialog);
private:
    TileMap* tile_map;
    int target_layer;
    RBSet<Vector2i> target_tiles;
    HashSet<int> original_sources;
    
    Tree* source_remap_tree;
    TreeItem* source_remap_root;

protected:
	virtual void ok_pressed() override;

    void _reconstruct_remap_window();
        
public:
    void set_original_sources(HashSet<int> p_sources);
    void set_selected_tiles(RBSet<Vector2i> p_tiles);
    void set_target_layer(int p_layer);
    void set_tile_map(TileMap* p_tile_map);

    RemapSourceDialog();
};

#endif 