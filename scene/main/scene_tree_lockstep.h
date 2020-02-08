#ifndef SCENE_TREE_LOCKSTEP_H
#define SCENE_TREE_LOCKSTEP_H

#include "core/array.h"
#include "core/object.h"
#include "scene_tree.h"
#include "thirdparty/ggpo/src/include/ggponet.h"

/* As a replacement for proper reusable code, just make some basic classes for testing. */
class PFGInput : public Object {
    GDCLASS(PFGInput, Object)

public:
    struct SerializedGGPOInput {
        uint16_t buttons;
    };

private:
    SerializedGGPOInput ggpo_inputs;

public:
    enum Buttons {
        UP = 1,
        DOWN = 1 << 2,
        LEFT = 1 << 3,
        RIGHT = 1 << 4,
        GRAB = 1 << 5
    };

    static void _bind_methods();

    void press_button(Buttons btn_id) {
        ggpo_inputs.buttons |= btn_id;
    }

    void flush_input_state() {
        ggpo_inputs = SerializedGGPOInput{0};
    }

    SerializedGGPOInput get_ggpo_input() {
        return ggpo_inputs;
    }

    void set_ggpo_inputs(SerializedGGPOInput p_ggpo_inputs) {
        ggpo_inputs = p_ggpo_inputs;
    }
};

VARIANT_ENUM_CAST(PFGInput::Buttons);

class PFGState : public Object {
    GDCLASS(PFGState, Object);
public:
    struct SerializedGGPOPlayerState {
        int x;
        int y;
        bool is_grabbing;
    };

    struct SerializedGGPOState {
        SerializedGGPOPlayerState players[2];
    };

    SerializedGGPOState gameState;

    static void _bind_methods();

    void serialize_gamestate(Dictionary dictGameState) {
        for (int i = 0; i < 2; i++ ){
            gameState.players[i].x = dictGameState[0]["x"];
            gameState.players[i].y = dictGameState[0]["y"];
            gameState.players[i].is_grabbing = dictGameState[0]["is_grabbing"];
        }
    }

    Dictionary deserialize_gamestate() {
        Dictionary dict = Dictionary();

        for (int i = 0; i < 2; i++){
            dict[i] = Dictionary();
            dict[i].set("x", gameState.players[i].x);
            dict[i].set("y", gameState.players[i].y);
            dict[i].set("is_grabbing", gameState.players[i].is_grabbing);
        }

        return dict;
    }

    SerializedGGPOState get_current_gamestate() {
        return gameState;
    }

    SerializedGGPOState* get_current_gamestate_ptr() {
        return &gameState;
    }
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
    bool add_local_input(uint8_t index, PFGInput::SerializedGGPOInput* input);
    bool synchronize_inputs(PFGInput::SerializedGGPOInput* inputs);
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
    PFGState game_state; // placeholder state for serialization

    bool iteration(float p_time) override;
    bool advance_frame(int);
    void request_inputs_for_local_player(PFGInput* ob, uint8_t player_id);

    static void _bind_methods();

    Dictionary get_game_state() { return game_state.deserialize_gamestate(); }
    void set_game_state(Dictionary new_state_object) {
        game_state.serialize_gamestate(new_state_object);
    }

    Object* get_ggpo() { return GGPONetwork::get_singleton(); }
};

#endif // SCENE_TREE_LOCKSTEP_H
