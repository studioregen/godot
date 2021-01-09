#ifndef SCENE_TREE_LOCKSTEP_H
#define SCENE_TREE_LOCKSTEP_H

#include "core/array.h"
#include "core/object.h"
#include "core/engine.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/main/ggpo_network.h"
#include "scene/main/viewport.h"

struct GGPOInput {
   uint32_t buttons = 0;

   static GGPOInput random(){
       GGPOInput to_return = GGPOInput{};
       to_return.buttons = rand();
       return to_return;
   }
};

class SceneTreeLockstep : public SceneTree {
    GDCLASS(SceneTreeLockstep, SceneTree);

private:
    float fixed_delta;
	Dictionary game_state;

public:

    /* This function wraps the SceneTree version of iteration so that,
     * when the system needs to rollback and resimulate n-frames,
     * it can call this function as a callback within GGPO.
     *
     * Iteration must be fixed & deterministic.
     */
    bool iteration(float p_time) override {
        fixed_delta = p_time;

        if (Engine::get_singleton()->is_editor_hint()) {
            return SceneTree::iteration(p_time);
        }

        GGPONetwork* ggpo = GGPONetwork::get_singleton();
        if (!ggpo->is_game_in_session()) {
            return SceneTree::iteration(p_time);
        }

        //We need to poll for local inputs
        //Sends this frame's local input to ggpo (or any other rollback network)
        bool inputs_accepted = true;
        {
            GGPOInput localInput = GGPOInput();
            request_inputs_for_local_player(localInput, 0);

            if (ggpo->is_sync_testing()){
                localInput = GGPOInput::random();
            }

            for (uint8_t i = 0; i < ggpo->get_current_player_count(); i++){
                if (ggpo->is_player_local(i)) {
                    Array args = Array();
                    args.append(i);
                    if( !ggpo->add_local_input(i, &localInput) ) {
                        print_line(String("Ignoring player input for {0} to catch up.").format(args));
                        inputs_accepted = false;
                    }
                }
            }
        }

        GGPOInput inputs[2] = {GGPOInput(), GGPOInput()};

        if (!ggpo->has_remote_players() || (ggpo->synchronize_inputs(inputs) && inputs_accepted)) { //Gets both remote and local (+frameDelay) inputs into array.
            bool shouldQuit = SceneTree::iteration(fixed_delta);
            lockstep_update(inputs, ggpo->get_current_player_count(), fixed_delta);
            ggpo->advance_frame();
            return shouldQuit;
        } else {
            print_line("Stalled...");
            return false;
        }
    }


    bool idle(float p_time) override {
        const uint64_t before = OS::get_singleton()->get_ticks_msec();
        bool result = SceneTree::idle(p_time);
        const uint64_t after = OS::get_singleton()->get_ticks_msec();
        const uint64_t timeLeft = (p_time * 1000) - (after - before);

        /* use remainder of "idle" time by handling GGPO networking tasks. */
        GGPONetwork* ggpo = GGPONetwork::get_singleton();
        if (ggpo->is_game_in_session()) {
            ggpo->idle(timeLeft);
        }

        return result;
    }

    bool advance_frame(int flags) {
        GGPONetwork* network = GGPONetwork::get_singleton();
        const int players = network->get_current_player_count();
        GGPOInput inputs[players];

        network->synchronize_inputs(inputs);
        iteration(fixed_delta);
        lockstep_update(inputs, network->get_current_player_count(), fixed_delta);

        return true;
    }

    void request_inputs_for_local_player(GGPOInput &input, uint8_t player_id) {

        Dictionary input_dict = serialize_ggpo_input(input);
        Array args;
        args.append(input_dict);
        args.append(player_id);

        if( get_root() )
            get_root()->propagate_call("fill_local_player_input", args );

        input = deserialize_ggpo_input(input_dict);
    }

    void lockstep_update(GGPOInput* inputs, uint8_t num_players, float p_time) {
        Dictionary all_inputs = Dictionary();

        for (int player_id = 0; player_id < num_players; player_id++) {
            all_inputs[player_id] = serialize_ggpo_input(inputs[player_id]);
        }

        Array args;
        args.append(all_inputs);
        args.append(p_time);

        if( get_root() )
            get_root()->propagate_call("_lockstep_update", args);

        for (int player_id = 0; player_id < num_players; player_id++) {
            inputs[player_id] = deserialize_ggpo_input(all_inputs[player_id]);
        }

        write_gamestate();
    }

    void write_gamestate() {
        Dictionary dict_game_state = game_state;

        Array args;
        args.append(dict_game_state);

        if( get_root() )
            get_root()->propagate_call("_serialize_game_state", args);

        //print_line( String("write_gamestate {0}").format(args) );

        game_state = dict_game_state;
    }

    Object* get_ggpo() { return (Object*)GGPONetwork::get_singleton(); }

    void load_game_state(Dictionary dict) {
        game_state = dict;

        print_line(String("{}").format(dict));

        Array args;
        args.append(game_state);

        if( get_root() )
            get_root()->propagate_call("_deserialize_game_state", args);
    }

    Dictionary get_game_state() { return game_state; }

    Dictionary serialize_ggpo_input(GGPOInput& input) {
        Dictionary input_dict = Dictionary();

        {
            input_dict["up"] = ((input.buttons & 1) > 0);
            input_dict["down"] = ((input.buttons & 1 << 2) > 0);
            input_dict["left"] = ((input.buttons & 1 << 3) > 0);
            input_dict["right"] = ((input.buttons & 1 << 4) > 0);
        }

        return input_dict;
    }

    GGPOInput deserialize_ggpo_input(Dictionary dictionary) {
        GGPOInput input;

        Array fmt;
        fmt.append(dictionary);

        {
            input.buttons |= dictionary["up"] ? 1 : 0;
            input.buttons |= dictionary["down"] ? 1 << 2 : 0;
            input.buttons |= dictionary["left"] ? 1 << 3 : 0;
            input.buttons |= dictionary["right"] ? 1 << 4 : 0;
        }

        return input;
    }

    static void _bind_methods(){
        ClassDB::bind_method(D_METHOD("get_ggpo"), &SceneTreeLockstep::get_ggpo);
        ClassDB::bind_method(D_METHOD("set_gamestate"), &SceneTreeLockstep::load_game_state);
    }
};

#endif // SCENE_TREE_LOCKSTEP_H
