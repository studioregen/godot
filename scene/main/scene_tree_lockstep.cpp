#include "main/scene_tree_lockstep.h"
#include "engine.h"

bool SceneTreeLockstep::iteration(float p_time) {
	if (Engine::get_singleton()->is_editor_hint()) return SceneTree::iteration(p_time);

	//We need to poll for local inputs

	lockstep_network.add_local_input(input_arr[0]); //Sends this frame's local input to ggpo (or any other rollback network)
	if (lockstep_network.synchronize_inputs(input_arr)) { //Gets both remote and local (+frameDelay) inputs into array.
		bool shouldQuit = lockstep_iteration(p_time, input_arr);
		return shouldQuit;
	} else {
		return false;
	}
}

/* This function wraps the SceneTree version of iteration so that,
 * when the system needs to rollback and resimulate n-frames,
 * it can this function as a callback within GGPO.
 *
 * Iteration must be fixed & deterministic.
 */
bool SceneTreeLockstep::lockstep_iteration(float p_time, Array inputs) {
	return SceneTree::iteration(p_time);
}
