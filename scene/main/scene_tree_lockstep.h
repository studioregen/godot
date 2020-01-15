#ifndef SCENE_TREE_LOCKSTEP_H
#define SCENE_TREE_LOCKSTEP_H

#include "core/array.h"
#include "core/object.h"
#include "main/scene_tree.h"

/* As a replacement for proper reusable code, just make some basic classes for testing. */
struct PFGInput {
	int left_right;
	int up_down;
	bool swap;
};

struct PFGState {
	int player1x;
	int player1y;
	int player2x;
	int player2y;
};

class Peer2PeerNetwork : public Object {
	GDCLASS(Peer2PeerNetwork, Object);

private:
	static Peer2PeerNetwork *instance = nullptr;
	SceneTreeLockstep *tree = nullptr;

public:
	static _FORCE_INLINE_ Peer2PeerNetwork *get_singleton() {
		if (!instance) {
			instance = memnew(Peer2PeerNetwork);
		}

		return instance;
	}

	/*class PlayerHandle {


        public:
            PlayerType player_type;
            StringName ip_address;
    };*/

	enum PlayerType {
		LOCAL_PLAYER,
		REMOTE_PLAYER
	};

	//These functions need to be defined, should wrap around GGPOs concept of a session.
	bool start_session(SceneTreeLockstep *tree, StringName name, uint_t num_players, StringName class_name, uint_t udp_port);
	bool add_player(PlayerType type, StringName ip_address = ""); /* Return player handle? */
	bool add_local_input(uint8_t index, PFGInput input);
	bool synchronize_inputs(Array input_array);
	void close_session();

	SceneTreeLockstep *get_tree() { return tree; }

	static bool begin_game(const char *game) {
		/* deprecated, but still necessary for some reason? */
		return true;
	}

	static bool advance_game(float p_time, PFGInput *input) {
		//Would be nice if we could use godot's signal system to handle these callbacks!
		Peer2PeerNetwork::get_singleton()->get_tree()->advance_game(p_time, input);
		return true;
	}

	static bool save_game_state(unsigned char **out_buffer, int *out_len,
			int *out_checksum, int frame) {
		PFGState *state = &Peer2PeerNetwork::get_singleton()->get_tree()->game_state;
		*out_len = sizeof(*state);
		copymem(*out_buffer, state, *out_len);
		return true;
	}

	static bool load_game_state(unsigned char *buffer, int len) {
		PFGState *state = &Peer2PeerNetwork::get_singleton()->get_tree()->game_state;
		copymem(state, buffer, len);
		return true;
	}

	static bool free_game_state(void *buffer) {
		memfree(buffer);
		return true;
	}

	static bool ggpo_event(void *info) {
		return true;
	}
};

class SceneTreeLockstep : public SceneTree {
	GDCLASS(Peer2PeerNetwork, Object)

public:
	PFGState game_state; // placeholder state for serialization
	bool iteration(float p_time) override;
	bool lockstep_iteration(float p_time, PFGInput *inputs);
};

#endif // SCENE_TREE_LOCKSTEP_H
