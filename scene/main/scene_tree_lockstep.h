#ifndef SCENE_TREE_LOCKSTEP_H
#define SCENE_TREE_LOCKSTEP_H

#include "core/array.h"
#include "core/object.h"
#include "scene_tree.h"
#include "thirdparty/ggpo/src/include/ggponet.h"

/* As a replacement for proper reusable code, just make some basic classes for testing. */
class PFGInput : public Object {
    GDCLASS(PFGInput, Object);
public:
	int left_right;
	int up_down;
	bool swap;

    static void _bind_methods();
};

class PFGState : public Object {
    GDCLASS(PFGState, Object);
public:
	int player1x;
	int player1y;
	int player2x;
	int player2y;

    static void _bind_methods();

    void clone_data(PFGState& other) {
        player1x = other.player1x;
        player2x = other.player2x;

        player1y = other.player1y;
        player2y = other.player2y;
    }
};


class Peer2PeerNetwork : public Object {
    GDCLASS(Peer2PeerNetwork, Object);

private:
    static Peer2PeerNetwork* instance;
    class SceneTreeLockstep* tree;

    GGPOSession* session;
    GGPOPlayer players[8];
    GGPOPlayerHandle handles[8];

    uint8_t total_player_count = 0;
    uint8_t current_player_count = 0;
    uint16_t port = 0;

    void set_tree(SceneTreeLockstep* tree) { tree = tree; }

public:
    static Peer2PeerNetwork* get_singleton();

    enum PlayerType {
        LOCAL_PLAYER,
        REMOTE_PLAYER
    };

    //These functions need to be defined, should wrap around GGPOs concept of a session.
    bool start_session(Object *tree, String name, uint16_t num_players, uint16_t udp_port);
    bool add_player(PlayerType type, String ip_address = ""); /* Return player handle? */
    bool add_local_input(uint8_t index, PFGInput* input);
    bool synchronize_inputs(PFGInput* inputs);
    void close_session();

    SceneTreeLockstep *get_tree();
    uint8_t get_current_player_count() { return current_player_count; }
    bool is_player_local(uint8_t id);
    bool is_game_in_session();

    static bool cb_begin_game(const char *game);

    static bool cb_advance_frame(int);

    static bool cb_save_game_state(unsigned char **out_buffer, int *out_len,
            int *out_checksum, int frame);

    static bool cb_load_game_state(unsigned char *buffer, int len);

    static void cb_free_game_state(void *buffer);

    static bool cb_ggpo_event(GGPOEvent* info);

    static void _bind_methods();
};


class SceneTreeLockstep : public SceneTree {
    GDCLASS(SceneTreeLockstep, SceneTree)
private:
    float fixed_delta;

public:
    PFGState game_state; // placeholder state for serialization

    bool iteration(float p_time) override;
    bool advance_frame(int);
    void request_inputs_for_local_player(PFGInput* ob, uint8_t player_id);

    static void _bind_methods();

    Object* get_game_state() { return &game_state; }
    void set_game_state(Object* new_state_object) {
        PFGState* new_state_values = Object::cast_to<PFGState>(new_state_object);
        if( new_state_values != nullptr) {
            game_state.clone_data(*new_state_values);
        }
    }
};

#endif // SCENE_TREE_LOCKSTEP_H
