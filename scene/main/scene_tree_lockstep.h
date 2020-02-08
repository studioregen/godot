#ifndef SCENE_TREE_LOCKSTEP_H
#define SCENE_TREE_LOCKSTEP_H

#include "core/array.h"
#include "core/object.h"
#include "scene_tree.h"
#include "thirdparty/ggpo/src/include/ggponet.h"

struct GGPOInput {
    uint32_t buttons = 0;
};

struct GGPOState {
    bool test = false;
};

class GGPONetwork : public Object {
    GDCLASS(GGPONetwork, Object)

private:
    static GGPONetwork* instance;
    class SceneTreeLockstep* tree;

    GGPOSession* session = nullptr;
    GGPOPlayer players[8];
    GGPOPlayerHandle handles[8];

    uint8_t total_player_count = 0;
    uint8_t current_player_count = 0;
    uint16_t port = 0;

    void set_tree(SceneTreeLockstep* tree) { tree = tree; }

public:
    static GGPONetwork* get_singleton();

    enum PlayerType {
        LOCAL_PLAYER,
        REMOTE_PLAYER
    };

    //These functions need to be defined, should wrap around GGPOs concept of a session.
    bool start_session(Object *tree, String name, uint16_t num_players, uint16_t udp_port);
    bool add_player(PlayerType type, String ip_address = ""); /* Return player handle? */
    bool add_local_input(uint8_t index, GGPOInput* input);
    bool synchronize_inputs(GGPOInput* inputs);
    bool idle(uint64_t timeoutMS);
    bool advance_frame();
    void close_session();

    SceneTreeLockstep* get_tree();
    uint8_t get_current_player_count() { return current_player_count; }
    bool is_player_local(uint8_t id);
    bool is_game_in_session();

    static bool cb_begin_game(const char *game);

    static bool cb_advance_frame(int);

    static bool cb_save_game_state(unsigned char **out_buffer, int *out_len,
            int *out_checksum, int frame);

    static bool cb_load_game_state(unsigned char *buffer, int len);

    static void cb_free_game_state(void *buffer);

    static bool cb_log_game_state(char* filename, unsigned char* buffer, int length);

    static bool cb_ggpo_event(GGPOEvent* info);

    static void _bind_methods();
};

VARIANT_ENUM_CAST(GGPONetwork::PlayerType);


class SceneTreeLockstep : public SceneTree {
    GDCLASS(SceneTreeLockstep, SceneTree)
private:
    float fixed_delta;

public:
    GGPOState game_state; // placeholder state for serialization

    bool iteration(float p_time) override;
    bool idle(float p_time) override;
    bool advance_frame(int);
    void request_inputs_for_local_player(GGPOInput &input, uint8_t player_id);

    static void _bind_methods();

    Object* get_ggpo() { return GGPONetwork::get_singleton(); }
};

#endif // SCENE_TREE_LOCKSTEP_H
