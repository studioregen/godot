#ifndef SCENE_TREE_LOCKSTEP_H
#define SCENE_TREE_LOCKSTEP_H

#include "core/array.h"
#include "core/object.h"
#include "core/engine.h"
#include "core/os/os.h"
#include "core/map.h"
#include "core/os/input.h"
#include "scene/main/scene_tree.h"
#include "scene/main/ggpo_network.h"
#include "scene/main/viewport.h"

struct GGPOController {
	uint32_t buttons = 0;

	GGPOController()
		: buttons(0) {}

	GGPOController( const GGPOController& rhs )
		: buttons(rhs.buttons) {}

	static GGPOController random(){
		GGPOController to_return = GGPOController{};
		to_return.buttons = rand();
		return to_return;
	}
};

class GGPOInput : public Reference {
	GDCLASS(GGPOInput, Reference)

public:
	GGPOInput()
		: Reference() {
		control.buttons = 0;
	}

	GGPOInput( const GGPOController &state )
		: Reference()
		, control(state) {}

	GGPOInput( const GGPOInput &rhs )
		: Reference()
		, control(rhs.control)
		, idToAction(rhs.idToAction)
		, actionToId(rhs.actionToId) {}

	bool register_action(int offset, StringName action_name) {
		if (offset >= 32 || offset < 0 ) {
			return false;
		}

		idToAction.insert(offset, action_name);
		actionToId.insert(action_name, offset);
		return true;
	}

	void free_action(int offset) {
		if (!idToAction.has(offset)) {
			return;
		}

		StringName action = idToAction.find(offset)->value();
		idToAction.erase(offset);
		actionToId.erase(action);
	}

	GGPOController get_state() {
		return control;
	}

	void set_state( GGPOController state ) {
		control = state;
	}

	void fill_inputs() {
		control.buttons = 0;
		for ( Map<int, StringName>::Element* E = idToAction.front();
			  E;
			  E = E->next() ) {
			bool is_pressed = Input::get_singleton()->is_action_pressed(E->value());
			control.buttons = is_pressed ? (control.buttons | (1 << E->key())) : control.buttons;
		}
	}

	void random() {
		control = GGPOController::random();
	}

	bool is_pressed( StringName action ) {
		if (!actionToId.has(action))
			return false;
		int offset = actionToId[action];
		print_line(String(action) + vformat("%d", control.buttons & (1 << offset)));
		return ((1 << offset) & control.buttons) > 0;
	}

protected:
	static void _bind_methods() {
		ClassDB::bind_method(D_METHOD("is_pressed", "p_action_name"), &GGPOInput::is_pressed);
		ClassDB::bind_method(D_METHOD("register_action", "p_bit_offset", "p_action_name"), &GGPOInput::register_action);
		ClassDB::bind_method(D_METHOD("free_action", "p_bit_offset"), &GGPOInput::register_action);
	}

private:
	GGPOController control;
	Map<int, StringName> idToAction;
	Map<StringName, int> actionToId;
};

const int MAX_PLAYERS = 4;

class SceneTreeLockstep : public SceneTree {
    GDCLASS(SceneTreeLockstep, SceneTree);
public:
	SceneTreeLockstep()
		: SceneTree()
		, player_input{memnew(GGPOInput), memnew(GGPOInput), memnew(GGPOInput), memnew(GGPOInput)} {

	}

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
            if (ggpo->is_sync_testing()){
                for (int i = 0; i < MAX_PLAYERS; i++) {
					player_input[i]->random();
				}
            }

            uint8_t local_player_index = 0;
            for (uint8_t i = 0; i < ggpo->get_current_player_count(); i++){
                if (ggpo->is_player_local(i)) {
					player_input[local_player_index]->fill_inputs();
					GGPOController ctrl_state = player_input[local_player_index]->get_state();
                    if( !ggpo->add_local_input(i, &ctrl_state) ) {
                        print_line(vformat("Ignoring player input for %d to catch up.", i));
                        inputs_accepted = false;
                    }

                    local_player_index++;
                }
            }
        }

        GGPOController inputs[MAX_PLAYERS];

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
        GGPOController inputs[players];

        network->synchronize_inputs(inputs);
        iteration(fixed_delta);
        lockstep_update(inputs, network->get_current_player_count(), fixed_delta);

        return true;
    }

    void lockstep_update(GGPOController* inputs, uint8_t num_players, float p_time) {
        Array all_inputs;

        for (int player_id = 0; player_id < num_players; player_id++) {
			player_input[player_id]->set_state(inputs[player_id]);
			print_line(vformat( "%d", player_input[player_id]->get_state().buttons));
			all_inputs.push_back(player_input[player_id]);
        }

        Array args;
        args.append(all_inputs);
        args.append(p_time);

        if( get_root() )
            get_root()->propagate_call("_lockstep_update", args);

        write_gamestate();
    }

    void write_gamestate() {
        Dictionary dict_game_state = game_state;

        Array args;
        args.append(dict_game_state);

        if( get_root() )
            get_root()->propagate_call("_serialize_game_state", args);

        game_state = dict_game_state;
    }

    Object* get_ggpo() { return (Object*)GGPONetwork::get_singleton(); }

    Ref<GGPOInput> get_player_input(int p_player_index) {
		if ( p_player_index >= MAX_PLAYERS || p_player_index < 0 )
			return nullptr;

		return player_input[p_player_index];
	}

    void load_game_state(Dictionary dict) {
        game_state = dict;

        print_line(String("{}").format(dict));

        Array args;
        args.append(game_state);

        if( get_root() )
            get_root()->propagate_call("_deserialize_game_state", args);
    }

    Dictionary get_game_state() { return game_state; }

    static void _bind_methods(){
        ClassDB::bind_method(D_METHOD("get_ggpo"), &SceneTreeLockstep::get_ggpo);
		ClassDB::bind_method(D_METHOD("get_player_input", "p_player_index"), &SceneTreeLockstep::get_player_input);
        ClassDB::bind_method(D_METHOD("set_gamestate"), &SceneTreeLockstep::load_game_state);
    }

	private:
		float fixed_delta;
		Dictionary game_state;
		Ref<GGPOInput> player_input[MAX_PLAYERS];

};

#endif // SCENE_TREE_LOCKSTEP_H
