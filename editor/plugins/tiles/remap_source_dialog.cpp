#include "remap_source_dialog.h"

#include "scene/gui/box_container.h"

void RemapSourceDialog::ok_pressed() {
	HashMap<int, int> map;
	for (int i = 0; i < source_remap_root->get_child_count(); i++) {
		TreeItem *item = source_remap_root->get_child(i);
		map.insert(item->get_range(0), item->get_range(2));
	}

	for (const Vector2i &tile : target_tiles) {
		const int sid = layer->get_cell_source_id(tile);
		const Vector2i atlasc = layer->get_cell_atlas_coords(tile);
		const int alt_tile = layer->get_cell_alternative_tile(tile);

		if (map.has(sid)) {
			layer->set_cell(tile, map[sid], atlasc, alt_tile);
		}
	}

	for (const KeyValue<int, int> &elem : map) {
		print_line(vformat("%d becomes %d", elem.key, elem.value));
	}
}

void RemapSourceDialog::_reconstruct_remap_window() {
	source_remap_tree->clear();
	source_remap_root = source_remap_tree->create_item();

	for (const int &source_id : original_sources) {
		TreeItem *item = source_remap_tree->create_item(source_remap_root);
		item->set_cell_mode(0, TreeItem::CELL_MODE_RANGE);
		item->set_range(0, source_id);
		item->set_text_alignment(0, HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
		item->set_text(1, "=>");
		item->set_cell_mode(2, TreeItem::CELL_MODE_RANGE);
		item->set_range(2, source_id);
		item->set_text_alignment(2, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
		item->set_editable(2, true);
	}
}

void RemapSourceDialog::set_original_sources(HashSet<int> sources) {
	original_sources = sources;
	_reconstruct_remap_window();
}

void RemapSourceDialog::set_selected_tiles(RBSet<Vector2i> p_tiles) {
	target_tiles = p_tiles;
}

void RemapSourceDialog::set_tile_map_layer(TileMapLayer *p_tile_map_layer) {
	layer = p_tile_map_layer;
}

RemapSourceDialog::RemapSourceDialog() {
	HBoxContainer *list_container = memnew(HBoxContainer);
	list_container->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	list_container->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	add_child(list_container);

	source_remap_tree = memnew(Tree);
	source_remap_tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	source_remap_tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	source_remap_tree->set_hide_root(true);
	source_remap_tree->set_allow_reselect(true);
	source_remap_tree->set_allow_rmb_select(true);
	source_remap_tree->set_columns(3);
	source_remap_tree->set_column_titles_visible(true);
	source_remap_tree->set_column_title(0, TTR("Original Source ID"));
	source_remap_tree->set_column_title(2, TTR("New Source ID"));
	source_remap_tree->set_column_expand(1, false);
	source_remap_tree->set_column_title_alignment(0, HorizontalAlignment::HORIZONTAL_ALIGNMENT_RIGHT);
	source_remap_tree->set_column_title_alignment(2, HorizontalAlignment::HORIZONTAL_ALIGNMENT_LEFT);
	list_container->add_child(source_remap_tree);

	source_remap_root = source_remap_tree->create_item();

	set_title(TTR("Remap Source IDs"));
}
