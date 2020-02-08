#include "scene_tree_lockstep.h"
#include "core/engine.h"
#include "viewport.h"
#include "core/os/os.h"

GGPONetwork* GGPONetwork::instance = nullptr;

GGPONetwork* GGPONetwork::get_singleton() {
    if (!instance) {
        instance = memnew(GGPONetwork);
    }

    return instance;
}

bool GGPONetwork::start_session(Object* tree_object, String name, uint16_t num_players, uint16_t udp_port) {
    SceneTreeLockstep* tree = Object::cast_to<SceneTreeLockstep>(tree_object);
    if (tree == nullptr) {
        return false;
    }

    set_tree(tree);

    GGPOErrorCode errcode;
    GGPOSessionCallbacks callbacks;

    callbacks.begin_game = &GGPONetwork::cb_begin_game;
    callbacks.advance_frame = &GGPONetwork::cb_advance_frame;
    callbacks.load_game_state = &GGPONetwork::cb_load_game_state;
    callbacks.save_game_state = &GGPONetwork::cb_save_game_state;
    callbacks.log_game_state = &GGPONetwork::cb_log_game_state;
    callbacks.free_buffer = &GGPONetwork::cb_free_game_state;
    callbacks.on_event = &GGPONetwork::cb_ggpo_event;

    Array string_args = Array();
    string_args.push_back(sizeof(GGPOInput));


    //sizeof Godot Object failing.
    errcode = ggpo_start_session(&session, &callbacks, name.utf8().get_data(), num_players, sizeof(GGPOInput), udp_port);
    string_args.push_back(errcode);
    string_args.push_back(num_players);
    print_line(String("SerializedGGPOInput {0}, errcode {1}, players {2}").format(string_args));

    total_player_count = num_players;
    current_player_count = 0;
    port = udp_port;

    ggpo_set_disconnect_timeout(session, 3000);
    ggpo_set_disconnect_notify_start(session, 1000);

    return GGPO_SUCCEEDED(errcode);
}

bool GGPONetwork::add_player(GGPONetwork::PlayerType type, String ip_address) {
    if (current_player_count >= total_player_count) {
        return false; // Can't have more players than we promised in the session initialization.
    }

    players[current_player_count].size = sizeof(GGPOPlayer);
    players[current_player_count].type = (type == LOCAL_PLAYER) ? GGPO_PLAYERTYPE_LOCAL : GGPO_PLAYERTYPE_REMOTE;
    players[current_player_count].player_num = current_player_count + 1;

    if( players[current_player_count].type == GGPO_PLAYERTYPE_REMOTE ) {
        strcpy( players[current_player_count].u.remote.ip_address, ip_address.utf8().get_data() );
        players[current_player_count].u.remote.port = port;
    }

    GGPOPlayerHandle handle;
    GGPOErrorCode err = ggpo_add_player(session, &players[current_player_count], &handle);
    handles[current_player_count] = handle;

    if (players[current_player_count].type == GGPO_PLAYERTYPE_LOCAL) {
        GGPOErrorCode delayErr = ggpo_set_frame_delay(session, handles[current_player_count], 2);

        if (!GGPO_SUCCEEDED(delayErr)) {
            Array string_args = Array();
            string_args.push_back(delayErr);
            print_line(String("ggpo_set_frame_delay, err {0}").format(string_args));
        }
    }

    if (GGPO_SUCCEEDED(err)) {
        current_player_count++; //Keeping track of current number of players initialized.
        return true;
    } else {
        Array string_args = Array();
        string_args.push_back(err);
        print_line(String("add_player, err {0}").format(string_args));
        return false;
    }
}

bool GGPONetwork::add_local_input(uint8_t index, GGPOInput* input) {
    if ( index >= current_player_count ) {
        return false;
    }

    GGPOErrorCode err;
    err = ggpo_add_local_input(session, handles[index], input, sizeof(GGPOInput));

    if (!GGPO_SUCCEEDED(err)) {
        Array string_args = Array();
        string_args.push_back(err);
        print_line(String("add_local_input, err {0}").format(string_args));
    }

    return GGPO_SUCCEEDED(err);
}

bool GGPONetwork::synchronize_inputs(GGPOInput* inputs) {
    GGPOErrorCode err;
    int disconnection_flags;

    err = ggpo_synchronize_input(session, inputs, sizeof(GGPOInput) * current_player_count, &disconnection_flags);

    if (!GGPO_SUCCEEDED(err)) {
        Array string_args = Array();
        string_args.push_back(err);
        print_line(String("synchronize_input, err {0}").format(string_args));
    }

    return GGPO_SUCCEEDED(err);
}

bool GGPONetwork::idle(uint64_t timeoutMS) {
    GGPOErrorCode err = ggpo_idle(session, timeoutMS);

    if (!GGPO_SUCCEEDED(err)) {
        Array string_args = Array();
        string_args.push_back(err);
        print_line(String("idle, err {0}").format(string_args));
    }

    return GGPO_SUCCEEDED(err);
}

bool GGPONetwork::advance_frame() {
    int err = ggpo_advance_frame(session);

    if (!GGPO_SUCCEEDED(err)) {
        Array string_args = Array();
        string_args.push_back(err);
        print_line(String("advance_frame, err {0}").format(string_args));
    }

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
    print_line("cb_advance_frame");
    GGPONetwork::get_singleton()->get_tree()->advance_frame(someInt);
    return true;
}

bool GGPONetwork::cb_save_game_state(unsigned char **out_buffer, int *out_len, int *out_checksum, int frame) {
    print_line("cb_save_game_state");
    GGPOState *state = &GGPONetwork::get_singleton()->get_tree()->game_state;
    *out_len = sizeof(*state);
    memcpy(*out_buffer, state, *out_len);
    return true;
}

bool GGPONetwork::cb_load_game_state(unsigned char *buffer, int len) {
    print_line("cb_load_game_state");
    GGPOState *state = &GGPONetwork::get_singleton()->get_tree()->game_state;
    memcpy(state, buffer, len);
    return true;
}

bool GGPONetwork::cb_log_game_state(char *filename, unsigned char *buffer, int length) {
    print_line("cb_log_game_state");
    return true;
}

void GGPONetwork::cb_free_game_state(void *buffer) {
    print_line("cb_free_game_state");
    free(buffer);
}

bool GGPONetwork::cb_ggpo_event(GGPOEvent* info) {
    print_line("cb_ggpo_event");
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

    if (Engine::get_singleton()->is_editor_hint()) {
        return SceneTree::iteration(p_time);
    }

    GGPONetwork* ggpo = GGPONetwork::get_singleton();
    if (!ggpo->is_game_in_session()) {
        return SceneTree::iteration(p_time);
    }

    GGPOInput inputs[ggpo->get_current_player_count()];

    //We need to poll for local inputs
    //Sends this frame's local input to ggpo (or any other rollback network)
    for (uint8_t i = 0; i < ggpo->get_current_player_count(); i++) {
        inputs[i] = GGPOInput();
        if(ggpo->is_player_local(i)) {
            request_inputs_for_local_player(inputs[i], i);
            ggpo->add_local_input(i, &inputs[i]);
        }
    }

    if (ggpo->synchronize_inputs(inputs)) { //Gets both remote and local (+frameDelay) inputs into array.
        bool shouldQuit = SceneTree::iteration(fixed_delta);
        ggpo->advance_frame();
		return shouldQuit;
	} else {
        print_line("Not working...");
        return false;
    }
}

bool SceneTreeLockstep::idle(float p_time) {
    const uint64_t before = OS::get_singleton()->get_ticks_msec();
    bool result = SceneTree::idle(p_time);
    const uint64_t after = OS::get_singleton()->get_ticks_msec();
    const uint64_t timeLeft = (p_time * 1000) - (after - before);


    GGPONetwork* ggpo = GGPONetwork::get_singleton();
    if (ggpo->is_game_in_session()) {
        ggpo->idle(timeLeft);
    }

    return result;
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
    GGPOInput inputs[players];

    network->synchronize_inputs(inputs);
    SceneTree::iteration(fixed_delta);
    return true;
}

void SceneTreeLockstep::request_inputs_for_local_player(GGPOInput &input, uint8_t player_id) {
    /*Array args;
    args.append(ob);
    args.append(player_id);

    if( get_root() )
        get_root()->propagate_call("fill_local_player_input", args );*/

}

void SceneTreeLockstep::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_ggpo"), &SceneTreeLockstep::get_ggpo);
}

