#include "ggpo_network.h"
#include "core/io/marshalls.h"
#include "scene/main/scene_tree_lockstep.h"

GGPONetwork* GGPONetwork::instance = nullptr;

GGPONetwork* GGPONetwork::get_singleton() {
    if (!instance) {
        instance = memnew(GGPONetwork);
    }

    return instance;
}

bool GGPONetwork::add_player(GGPONetwork::PlayerType type, String ip_address) {
    const uint8_t current_player_count = details->current_player_count;

    if (details->current_player_count >= details->max_player_count) {
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
        GGPOErrorCode delayErr = ggpo_set_frame_delay(session, handles[current_player_count], 1);

        if (!GGPO_SUCCEEDED(delayErr)) {
            Array string_args = Array();
            string_args.push_back(delayErr);
            print_line(String("ggpo_set_frame_delay, err {0}").format(string_args));
        }
    }

    if (GGPO_SUCCEEDED(err)) {
        //Keeping track of current number of players initialized.
        details->remote_player_count = (players[current_player_count].type == GGPO_PLAYERTYPE_REMOTE) ? details->remote_player_count + 1 : details->remote_player_count;
        details->current_player_count++;
        return true;
    } else {
        Array string_args = Array();
        string_args.push_back(err);
        print_line(String("add_player, err {0}").format(string_args));
        return false;
    }
}

bool GGPONetwork::is_player_local(uint8_t id){
    if ( id < get_current_player_count()) {
        return (players[id].type == GGPO_PLAYERTYPE_LOCAL);
    } else {
        return false;
    }
}

bool GGPONetwork::is_game_in_session() {
    return details ? (details->max_player_count > 0) : false;
}

bool GGPONetwork::is_sync_testing() {
    return details ? details->sync_testing : false;
}


bool GGPONetwork::cb_begin_game(const char *game) {
    /* deprecated, but still necessary for some reason? */
    return true;
}

bool GGPONetwork::cb_advance_frame(int flags) {
    //Would be nice if we could use godot's signal system to handle these callbacks!
    //print_line("cb_advance_frame");
    GGPONetwork::get_singleton()->get_tree()->advance_frame(flags);
    return true;
}

bool GGPONetwork::cb_save_game_state(unsigned char **out_buffer, int *out_len, int *out_checksum, int frame) {
    //print_line("cb_save_game_state");
    if (!GGPONetwork::get_singleton()->get_tree()) {
        print_line("No tree?");
        return false;
    }

    int length = 0;
    Error err = encode_variant(GGPONetwork::get_singleton()->get_tree()->get_game_state(), NULL, length, true);
    *out_len = length;
    *out_buffer = (unsigned char*)memalloc(*out_len);

    if (!*out_buffer || err != OK) {
       return false;
    }

    encode_variant(GGPONetwork::get_singleton()->get_tree()->get_game_state(), *out_buffer, *out_len, true);
    return true;
}

bool GGPONetwork::cb_load_game_state(unsigned char *buffer, int len) {
    //print_line("cb_load_game_state");
    Variant var;
    Error err = decode_variant(var, buffer, len, NULL, true);

    if (err != OK) {
        return false;
    }

    GGPONetwork::get_singleton()->get_tree()->load_game_state((Dictionary)var);
    return true;
}

bool GGPONetwork::cb_log_game_state(char *filename, unsigned char *buffer, int length) {
    //print_line("cb_log_game_state");
    return true;
}

void GGPONetwork::cb_free_game_state(void *buffer) {
    //print_line("cb_free_game_state");
    memfree(buffer);
}

bool GGPONetwork::cb_ggpo_event(GGPOEvent* info) {
    //print_line("cb_ggpo_event");
    /*int progress;
    switch (info->code) {
    case GGPO_EVENTCODE_CONNECTED_TO_PEER:
       ngs.SetConnectState(info->u.connected.player, Synchronizing);
       break;
    case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
       progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
       ngs.UpdateConnectProgress(info->u.synchronizing.player, progress);
       break;
    case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
       ngs.UpdateConnectProgress(info->u.synchronized.player, 100);
       break;
    case GGPO_EVENTCODE_RUNNING:
       ngs.SetConnectState(Running);
       renderer->SetStatusText("");
       break;
    case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
       ngs.SetDisconnectTimeout(info->u.connection_interrupted.player,
                                GetCurrentTimeMS(),
                                info->u.connection_interrupted.disconnect_timeout);
       break;
    case GGPO_EVENTCODE_CONNECTION_RESUMED:
       ngs.SetConnectState(info->u.connection_resumed.player, Running);
       break;
    case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
       ngs.SetConnectState(info->u.disconnected.player, Disconnected);
       break;
    case GGPO_EVENTCODE_TIMESYNC:
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 * info->u.timesync.frames_ahead / 60));
       break;
    }*/
    return true;
}

void GGPONetwork::_bind_methods() {
    ClassDB::bind_method(D_METHOD("start_session", "tree", "name", "player_count", "port"), &GGPONetwork::start_session);
    ClassDB::bind_method(D_METHOD("start_sync_test", "tree", "name", "player_count"), &GGPONetwork::start_sync_test);
    ClassDB::bind_method(D_METHOD("add_player", "player_locality", "ip_address"), &GGPONetwork::add_player);
    ClassDB::bind_method(D_METHOD("close_session"), &GGPONetwork::close_session);

    ClassDB::bind_method(D_METHOD("is_player_local", "id"), &GGPONetwork::is_player_local);
    ClassDB::bind_method(D_METHOD("is_game_in_session"), &GGPONetwork::is_game_in_session);

    BIND_ENUM_CONSTANT(LOCAL_PLAYER);
    BIND_ENUM_CONSTANT(REMOTE_PLAYER);
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

        //sizeof Godot Object failing.
        errcode = ggpo_start_session(&session, &callbacks, name.utf8().get_data(), num_players, sizeof(GGPOInput), udp_port);

        if (!GGPO_SUCCEEDED(errcode) ) {
            Array string_args = Array();
            string_args.push_back(sizeof(GGPOInput));
            string_args.push_back(errcode);
            string_args.push_back(num_players);
            print_line(String("SerializedGGPOInput {0}, errcode {1}, players {2}").format(string_args));
        }

        details = new GGPOSessionDetails(num_players);
        details->current_player_count = 0;
        port = udp_port;

        ggpo_set_disconnect_timeout(session, 3000);
        ggpo_set_disconnect_notify_start(session, 1000);

        return GGPO_SUCCEEDED(errcode);
    }

bool GGPONetwork::start_sync_test(Object* tree_object, String name, uint16_t num_players) {
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

        errcode = ggpo_start_synctest(&session, &callbacks, "sync_test", num_players, sizeof(GGPOInput), 1);

        if (!GGPO_SUCCEEDED(errcode)) {
            Array string_args = Array();
            string_args.push_back(sizeof(GGPOInput));
            string_args.push_back(errcode);
            string_args.push_back(num_players);
            print_line(String("SyncTest GGPOInputSize {0}, errcode {1}, players {2}").format(string_args));
        }

        details = new GGPOSessionDetails(num_players, true);
        details->current_player_count = 0;

        ggpo_set_disconnect_timeout(session, 3000);
        ggpo_set_disconnect_notify_start(session, 1000);

        return GGPO_SUCCEEDED(errcode);
    }

bool GGPONetwork::add_local_input(uint8_t index, GGPOInput* input) {
        if ( index >= get_current_player_count() ) {
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

        err = ggpo_synchronize_input(session, (void *) inputs, sizeof(GGPOInput) * details->max_player_count, &disconnection_flags);

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

void GGPONetwork::close_session(){
       ggpo_close_session(session);
       delete details;
       tree = nullptr;
   }
