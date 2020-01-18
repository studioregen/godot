#include "scene_tree_lockstep.h"
#include "core/engine.h"
#include "viewport.h"

GGPONetwork* GGPONetwork::instance = nullptr;


void PFGInput::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_left_right", "dir"), &PFGInput::set_left_right);
    ClassDB::bind_method(D_METHOD("get_left_right"), &PFGInput::get_left_right);
    ClassDB::bind_method(D_METHOD("set_up_down", "dir"), &PFGInput::set_up_down);
    ClassDB::bind_method(D_METHOD("get_up_down"), &PFGInput::get_up_down);
    ClassDB::bind_method(D_METHOD("set_swap", "grab"), &PFGInput::set_swap);
    ClassDB::bind_method(D_METHOD("get_swap"), &PFGInput::get_swap);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "left_right"), "set_left_right", "get_left_right");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "up_down"), "set_up_down", "get_up_down");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "swap"), "set_swap", "get_swap");
}

void PFGState::_bind_methods() {

}

GGPONetwork* GGPONetwork::get_singleton() {
    if (!instance) {
        instance = memnew(GGPONetwork);
    }

    return instance;
}

bool GGPONetwork::start_session(Object* tree_object, String name, uint16_t num_players, uint16_t udp_port) {
    SceneTreeLockstep* tree = Object::cast_to<SceneTreeLockstep>(tree_object);
    if (tree != nullptr) { //bail if tree is invalid.
        set_tree(tree);
    } else {
        return false;
    }

    GGPOErrorCode errcode;
    GGPOSessionCallbacks callbacks;

    callbacks.begin_game = GGPONetwork::cb_begin_game;
    callbacks.advance_frame = GGPONetwork::cb_advance_frame;
    callbacks.load_game_state = GGPONetwork::cb_load_game_state;
    callbacks.save_game_state = GGPONetwork::cb_save_game_state;
    callbacks.free_buffer = GGPONetwork::cb_free_game_state;
    callbacks.on_event = GGPONetwork::cb_ggpo_event;

    errcode = ggpo_start_session(&session, &callbacks, name.utf8().get_data(), num_players, sizeof(PFGInput), udp_port);

    total_player_count = num_players;
    current_player_count = 0;
    port = udp_port;

    return true;
}

bool GGPONetwork::add_player(GGPONetwork::PlayerType type, String ip_address) {
    if (current_player_count >= total_player_count) {
        return false; // Can't have more players than we promised in the session initialization.
    }

    players[current_player_count].type = (type == LOCAL_PLAYER) ? GGPO_PLAYERTYPE_LOCAL : GGPO_PLAYERTYPE_REMOTE;
    players[current_player_count].size = sizeof(GGPOPlayer);

    if( players[current_player_count].type == GGPO_PLAYERTYPE_REMOTE ) {
        strcpy( players[current_player_count].u.remote.ip_address, ip_address.utf8().get_data() );
        players[current_player_count].u.remote.port = port;
    }

    ggpo_add_player(session, &players[current_player_count], &handles[current_player_count]);
    current_player_count++; //Keeping track of current number of players initialized.
    return true;
}

bool GGPONetwork::add_local_input(uint8_t index, PFGInput* input) {
    if ( index >= current_player_count ) {
        return false;
    }

    GGPOErrorCode err;
    err = ggpo_add_local_input(session, handles[index], input, sizeof(PFGInput));

    return GGPO_SUCCEEDED(err);
}

bool GGPONetwork::synchronize_inputs(PFGInput* inputs) {
    GGPOErrorCode err;
    int disconnection_flags;
    err = ggpo_synchronize_input(session, inputs, sizeof(PFGInput) * current_player_count, &disconnection_flags);
    return GGPO_SUCCEEDED(err);
}

bool GGPONetwork::advance_frame() {
    int err = ggpo_advance_frame(session);
    return GGPO_SUCCEEDED(err);
}

void GGPONetwork::close_session() {
    ggpo_close_session(session);
    current_player_count = 0;
    total_player_count = 0;
    tree = nullptr;
}

SceneTreeLockstep *GGPONetwork::get_tree() {
    return tree;
}

bool GGPONetwork::is_player_local(uint8_t id){
    if ( id < get_current_player_count()) {
        return (players[id].type == GGPO_PLAYERTYPE_LOCAL);
    } else {
        return false;
    }
}

bool GGPONetwork::is_game_in_session() {
    return (total_player_count > 0);
}


bool GGPONetwork::cb_begin_game(const char *game) {
    /* deprecated, but still necessary for some reason? */
    return true;
}

bool GGPONetwork::cb_advance_frame(int someInt) {
    //Would be nice if we could use godot's signal system to handle these callbacks!
    GGPONetwork::get_singleton()->get_tree()->advance_frame(someInt);
    return true;
}

bool GGPONetwork::cb_save_game_state(unsigned char **out_buffer, int *out_len, int *out_checksum, int frame) {
    PFGState *state = &GGPONetwork::get_singleton()->get_tree()->game_state;
    *out_len = sizeof(*state);
    copymem(*out_buffer, state, *out_len);
    return true;
}

bool GGPONetwork::cb_load_game_state(unsigned char *buffer, int len) {
    PFGState *state = &GGPONetwork::get_singleton()->get_tree()->game_state;
    copymem(state, buffer, len);
    return true;
}

void GGPONetwork::cb_free_game_state(void *buffer) {
    memfree(buffer);
}

bool GGPONetwork::cb_ggpo_event(GGPOEvent* info) {
    return true;
}

void GGPONetwork::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_session", "tree", "name", "player_count", "port"), &GGPONetwork::start_session);
    ClassDB::bind_method(D_METHOD("add_player", "player_locality", "ip_address"), &GGPONetwork::add_player);
    ClassDB::bind_method(D_METHOD("close_session"), &GGPONetwork::close_session);

    ClassDB::bind_method(D_METHOD("is_player_local", "id"), &GGPONetwork::is_player_local);
    ClassDB::bind_method(D_METHOD("is_game_in_session"), &GGPONetwork::is_game_in_session);

    BIND_ENUM_CONSTANT(LOCAL_PLAYER);
    BIND_ENUM_CONSTANT(REMOTE_PLAYER);
}

bool SceneTreeLockstep::iteration(float p_time) {
    fixed_delta = p_time;

    //Dictionary format_args;
    //format_args["isEditor"] = Engine::get_singleton()->is_editor_hint();

    //print_line(String("Testing. IsEditor={isEditor}").format(format_args));
    if (Engine::get_singleton()->is_editor_hint()) {
        return SceneTree::iteration(p_time);
    }

    GGPONetwork* ggpo = GGPONetwork::get_singleton();
    if (!ggpo->is_game_in_session()) {
        return SceneTree::iteration(p_time);
    }

    PFGInput inputs[ggpo->get_current_player_count()];

    //We need to poll for local inputs
    //Sends this frame's local input to ggpo (or any other rollback network)
    uint8_t local_player_id = 0;
    for (uint8_t i = 0; i < ggpo->get_current_player_count(); i++) {
        if(ggpo->is_player_local(i)) {
            //FILL IN INPUT HERE;
            request_inputs_for_local_player(&inputs[i], local_player_id);
            ggpo->add_local_input(i, &inputs[i]);
            local_player_id++;
        }
    }

    if (ggpo->synchronize_inputs(inputs)) { //Gets both remote and local (+frameDelay) inputs into array.
        bool shouldQuit = SceneTree::iteration(fixed_delta);
        ggpo->advance_frame();
		return shouldQuit;
	} else {
        return false;
	}
}

/* This function wraps the SceneTree version of iteration so that,
 * when the system needs to rollback and resimulate n-frames,
 * it can call this function as a callback within GGPO.
 *
 * Iteration must be fixed & deterministic.
 */
bool SceneTreeLockstep::advance_frame(int) {
    GGPONetwork* network = GGPONetwork::get_singleton();
    const int players = network->get_current_player_count();
    PFGInput inputs[players];

    network->synchronize_inputs(inputs);
    SceneTree::iteration(fixed_delta);
    return true;
}

void SceneTreeLockstep::request_inputs_for_local_player(PFGInput *ob, uint8_t player_id) {
    Array args;
    args.append(ob);
    args.append(player_id);

    if( get_root() )
        get_root()->propagate_call("fill_local_player_input", args );

}

void SceneTreeLockstep::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_game_state"), &SceneTreeLockstep::get_game_state);
    ClassDB::bind_method(D_METHOD("set_game_state"), &SceneTreeLockstep::set_game_state);
    ClassDB::bind_method(D_METHOD("get_ggpo"), &SceneTreeLockstep::get_ggpo);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "game_state"), "set_game_state", "get_game_state");
}
