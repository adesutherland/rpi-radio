/* */
#include <libspotify/api.h>
#include "spotifywrapper.h"
#include <sys/time.h>

using namespace std;

string myMacEthernetAddress(); // in getmacaddress.cpp

SessionWrapper* SessionWrapper::singleton = NULL;

SessionWrapper::SessionWrapper() {
  session = NULL;
  session_callbacks = (sp_session_callbacks){0};  
  session_config = (sp_session_config){0};
  audioDriver = NULL;
  
  singleton = this;
  
  // Session Callbacks  
  session_callbacks.logged_in = logged_in;
  session_callbacks.logged_out = logged_out;
  session_callbacks.metadata_updated = metadata_updated;
  session_callbacks.connection_error = connection_error;
  session_callbacks.message_to_user = message_to_user;
  session_callbacks.notify_main_thread = notify_main_thread;
  session_callbacks.play_token_lost = play_token_lost;
  session_callbacks.log_message = log_message;
  session_callbacks.end_of_track = end_of_track;
  session_callbacks.streaming_error = streaming_error;
  session_callbacks.userinfo_updated = userinfo_updated;
  session_callbacks.offline_status_updated = offline_status_updated;
  session_callbacks.offline_error = offline_error;
  session_callbacks.credentials_blob_updated = credentials_blob_updated;
  session_callbacks.connectionstate_updated = connectionstate_updated;
  session_callbacks.unaccepted_licenses_updated = unaccepted_licenses_updated;
  session_callbacks.scrobble_error = scrobble_error;
  session_callbacks.private_session_mode_changed = private_session_mode_changed;

  // Session Callbacks for the music playing system - No events
  session_callbacks.get_audio_buffer_stats = get_audio_buffer_stats; 
  session_callbacks.music_delivery = music_delivery;
  session_callbacks.start_playback = start_playback;
  session_callbacks.stop_playback = stop_playback;

  // Playlist Container Callbacks
  playlistcontainer_callbacks.playlist_added = playlist_added;
  playlistcontainer_callbacks.playlist_removed = playlist_removed;  
  playlistcontainer_callbacks.playlist_moved = playlist_moved;
  playlistcontainer_callbacks.container_loaded = container_loaded;

  // Playlist Callbacks
  playlist_callbacks.tracks_added = tracks_added; 
  playlist_callbacks.tracks_removed = tracks_removed; 
  playlist_callbacks.tracks_moved = tracks_moved;
  playlist_callbacks.playlist_renamed = playlist_renamed;    
  playlist_callbacks.playlist_state_changed = playlist_state_changed;
  playlist_callbacks.playlist_update_in_progress = playlist_update_in_progress;
  playlist_callbacks.playlist_metadata_updated = playlist_metadata_updated;
  playlist_callbacks.track_created_changed = track_created_changed;
  playlist_callbacks.track_seen_changed = track_seen_changed;
  playlist_callbacks.description_changed = description_changed;
  playlist_callbacks.image_changed = image_changed;
  playlist_callbacks.track_message_changed = track_message_changed;
  playlist_callbacks.subscribers_changed = subscribers_changed;  
}

BaseAudioDriver* SessionWrapper::getAudioDriver() {
  return audioDriver;
}

int SessionWrapper::login(std::string userid, std::string password) {
  class : public Workflow {
    void logged_in() {
      Workflow::logged_in();
      workflowDone();
    }
  } loginWorkflow;
  
  sp_session_login(getSession(), userid.c_str(), password.c_str(), true, NULL);

  loginWorkflow.run();
  if (loginWorkflow.getHadError()) {
    cerr << "Error: " << loginWorkflow.getErrorText() << endl;
    return -1;
  }
  return 0;
}

int SessionWrapper::login() {
  class : public Workflow {
    void logged_in() {
      Workflow::logged_in();
      workflowDone();
    }
  } loginWorkflow;
  
  sp_session_relogin(getSession());

  loginWorkflow.run();
  if (loginWorkflow.getHadError()) {
    cerr << "Error: " << loginWorkflow.getErrorText() << endl;
    return -1;
  }
  return 0;
}

int SessionWrapper::onlineLogin(std::string userid, std::string password) {
  class : public Workflow {
    public:
    sp_session *session;
    void logged_in() {
      Workflow::logged_in();
      if (sp_session_connectionstate(session) == SP_CONNECTION_STATE_LOGGED_IN) {
        workflowDone();
      }
    }
    void connectionstate_updated() {
      if (sp_session_connectionstate(session) == SP_CONNECTION_STATE_LOGGED_IN) {
        workflowDone();
      }
    }
  } loginWorkflow;
  loginWorkflow.session = getSession();
  
  
  sp_session_login(getSession(), userid.c_str(), password.c_str(), true, NULL);

  loginWorkflow.run();
  if (loginWorkflow.getHadError()) {
    cerr << "Error: " << loginWorkflow.getErrorText() << endl;
    return -1;
  }
  return 0;
}

int SessionWrapper::onlineLogin() {
  class : public Workflow {
    public:
    sp_session *session;
    void logged_in() {
      Workflow::logged_in();
      if (sp_session_connectionstate(session) == SP_CONNECTION_STATE_LOGGED_IN) {
        workflowDone();
      }
    }
    void connectionstate_updated() {
      if (sp_session_connectionstate(session) == SP_CONNECTION_STATE_LOGGED_IN) {
        workflowDone();
      }
    }
  } loginWorkflow;
  loginWorkflow.session = getSession();
  
  sp_session_relogin(getSession());

  loginWorkflow.run();
  if (loginWorkflow.getHadError()) {
    cerr << "Error: " << loginWorkflow.getErrorText() << endl;
    return -1;
  }
  return 0;
}

int SessionWrapper::logout() {
  class : public Workflow {
    void logged_out() {
      workflowDone(); 
    }
  } logoutWorkflow;
  
  sp_session_logout(getSession());
  logoutWorkflow.run();

  // Done with Audio
  audioDriver->done();

  if (logoutWorkflow.getHadError()) {
    cerr << "Error: " << logoutWorkflow.getErrorText() << endl;
    return -1;
  }
  
  // Wait for a couple of seconds to let libspotify to settle down
  wait(2000);
  return 0;
}

int SessionWrapper::wait(int milliseconds) {
  
  class : public Workflow {
    struct timeval t1, t2;
    int elapsedTime;
    int waitFor;

    public:
      void init(int milliseconds) {
        waitFor = milliseconds;
        gettimeofday(&t1, NULL);
      }
    
      void nothing() {
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;    // sec to ms
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms
        if (elapsedTime > waitFor) {
          workflowDone();
        } 
      }
  } workflow;
  
  workflow.init(milliseconds);
  workflow.run();

  if (workflow.getHadError()) {
    cerr << "Error: " << workflow.getErrorText() << endl;
    return -1;
  }
  return 0;
}

int SessionWrapper::listUnacceptedLicenses(list<string> &licenseIDs, list<string> &licenseUrls) {
  int num = sp_session_num_unaccepted_licenses(getSession());
  for (int i=0; i<num; i++) {
    const char *id = sp_session_unaccepted_license_id	(getSession(), i);
    const char *url = sp_session_url_for_license(getSession(), id);
    licenseIDs.push_back(id);
    licenseUrls.push_back(url);
  }	 
  return num;
}

int SessionWrapper::acceptLicenses(list<string> &licenseIDs) {
  int num = licenseIDs.size();
  const char** ids = new const char*[num];
  int i = 0;
  for (list<string>::const_iterator id = licenseIDs.begin(); id != licenseIDs.end(); ++id) {
    ids[i] = (*id).c_str();
    i++;
  }

  sp_error error = sp_session_accept_licenses	(getSession(), ids, num);
  delete ids;
  if (error != SP_ERROR_OK) {
    cerr << "Error accepting licenses: " << sp_error_message(error) << endl;
    return -1;
  }
  	 
  return 0;
}

int SessionWrapper::loadPlaylistContainer() {
  // Setup Container
  if (!playlistContainer) {
    playlistContainer = sp_session_playlistcontainer(session);
    if (!playlistContainer) {
      cerr << "setupPlaylistContainer() needs the user to be logged on first" << endl;
      return -1;
    }
   
    sp_error error = sp_playlistcontainer_add_callbacks(playlistContainer, &playlistcontainer_callbacks, NULL);
    if (error != SP_ERROR_OK) {
      cerr << "Error setting playlist container callbacks: " << sp_error_message(error) << endl;
      return -1;
    }
  }

  // Make sure it is loaded
  if (!sp_playlistcontainer_is_loaded(playlistContainer)) {
    // Wait until loaded
    class : public Workflow {
      void container_loaded() {
        workflowDone(); 
      }
    } workflow;
    workflow.run();
    if (workflow.getHadError()) {
      cerr << "Error loading playlist container: " << workflow.getErrorText() << endl;
      return -1;
    }
  }
  return 0;
}

int SessionWrapper::loadUsersPlaylists(list<string> &playlists) {
  string name;
  list<string> dirPath;
  char buffer[100];
  sp_playlist* playlist;
  sp_error error;
  
  // Load Container
  if (loadPlaylistContainer()) {
    return -1;
  }
  
  int num = sp_playlistcontainer_num_playlists(playlistContainer); 	
  for (int n=0; n<num; n++) {
    sp_playlist_type type = sp_playlistcontainer_playlist_type(playlistContainer, n);
    switch (type) {
      case SP_PLAYLIST_TYPE_PLAYLIST: // Normal Playlist
        // Directory
        name.clear();
        for (list<string>::const_iterator i = dirPath.begin(); i != dirPath.end(); ++i) {
          name.append(*i);
          name.append(" / ");
        }
        
        playlist = sp_playlistcontainer_playlist(playlistContainer, n);
        error = loadPlaylist(playlist);
        if (error != SP_ERROR_OK) {
          cerr << "Error loading playlist: " << sp_error_message(error) << endl;
          return -1;
        }
        name.append(sp_playlist_name(playlist));
        playlists.push_back(name);
        break;

      case SP_PLAYLIST_TYPE_START_FOLDER: // Marks a folder starting point
        sp_playlistcontainer_playlist_folder_name(playlistContainer, n, buffer, sizeof(buffer));
        dirPath.push_back(buffer);
        break;

      case SP_PLAYLIST_TYPE_END_FOLDER: // Folder ending point
        dirPath.pop_back();
        break;
      
      case SP_PLAYLIST_TYPE_PLACEHOLDER: // Unknown entry
      
      default:
        ;
    }
  }
  
  return 0;
}

sp_error SessionWrapper::loadPlaylist(sp_playlist* playlist) {
  if (sp_playlist_is_loaded(playlist)) {
    return SP_ERROR_OK;
  }
  sp_error error = sp_playlist_add_callbacks(playlist, &playlist_callbacks, NULL);
  if (error != SP_ERROR_OK) {
    cerr << "Error setting playlist callbacks: " << sp_error_message(error) << endl;
    return error;
  }
  
  // Wait until loaded
  class : public Workflow {
    public:
    sp_playlist* workflowPlaylist;
    void playlist_state_changed() {
      if (sp_playlist_is_loaded(workflowPlaylist)) {
        workflowDone();
      } 
    }
  } workflow;
  workflow.workflowPlaylist = playlist;
  workflow.run();

  sp_playlist_remove_callbacks(playlist, &playlist_callbacks, NULL);
  if (error != SP_ERROR_OK) {
    cerr << "Error removing playlist callbacks: " << sp_error_message(error) << endl;
    return error;
  }  
  
  if (workflow.getHadError()) {
    cerr << "Error waiting for playlist to load: " << workflow.getErrorText() << endl;
    return workflow.getSpotifyError();
  }
    
  return SP_ERROR_OK;
}


sp_playlist* SessionWrapper::getPlaylistByName(string playlistName) {  
  string name;
  list<string> dirPath;
  char buffer[100];
  sp_playlist* playlist;
  sp_error error;
  
  // Load Container
  if (loadPlaylistContainer()) {
    return NULL;
  }
  
  int num = sp_playlistcontainer_num_playlists(playlistContainer); 	
  for (int n=0; n<num; n++) {
    sp_playlist_type type = sp_playlistcontainer_playlist_type(playlistContainer, n);
    switch (type) {
      case SP_PLAYLIST_TYPE_PLAYLIST: // Normal Playlist
        // Directory
        name.clear();
        for (list<string>::const_iterator i = dirPath.begin(); i != dirPath.end(); ++i) {
          name.append(*i);
          name.append(" / ");
        }
        
        playlist = sp_playlistcontainer_playlist(playlistContainer, n);
        error = loadPlaylist(playlist);
        if (error != SP_ERROR_OK) {
          cerr << "Error loading playlist: " << sp_error_message(error) << endl;
          return NULL;
        }
        name.append(sp_playlist_name(playlist));
        if (playlistName.compare(name) == 0) {
          return playlist;
        }
        break;

      case SP_PLAYLIST_TYPE_START_FOLDER: // Marks a folder starting point
        sp_playlistcontainer_playlist_folder_name(playlistContainer, n, buffer, sizeof(buffer));
        dirPath.push_back(buffer);
        break;

      case SP_PLAYLIST_TYPE_END_FOLDER: // Folder ending point
        dirPath.pop_back();
        break;
      
      case SP_PLAYLIST_TYPE_PLACEHOLDER: // Unknown entry
      
      default:
        ;
    }
  }
  
  return NULL;
}

int SessionWrapper::loadPlaylistTracks(string playlistName, std::list<std::string> &tracks){
  sp_playlist* playlist = getPlaylistByName(playlistName);
  if (!playlist) return -1;
  
  return loadPlaylistTracks(playlist, tracks);
}

int SessionWrapper::loadPlaylistTracks(sp_playlist* playlist, std::list<std::string> &tracks) {  
  int num = sp_playlist_num_tracks(playlist); 	
  for (int n = 0; n < num; n++) {
    sp_track *track = sp_playlist_track(playlist, n);
    sp_error error = loadTrack(track);
    if (error != SP_ERROR_OK) {
      cerr << "Error loading track: " << sp_error_message(error) << endl;
      return -1;
    }
    tracks.push_back(sp_track_name(track));
  }
  return 0;
}

sp_error SessionWrapper::loadTrack(sp_track* track){

  if (sp_track_is_loaded(track)) {
    return SP_ERROR_OK;
  }
  
  // Wait until loaded
  class : public Workflow {
    public:
    sp_track* workflowTrack;
    void nothing() { // It seems there is no specific callback to listen for!
      if (sp_track_is_loaded(workflowTrack)) {
        workflowDone();
      } 
    }
  } workflow;
  workflow.workflowTrack = track;
  workflow.run();
  if (workflow.getHadError()) {
    cerr << "Error waiting for track to load: " << workflow.getErrorText() << endl;
    return workflow.getSpotifyError();
  }
  
  return SP_ERROR_OK;
}

int SessionWrapper::playTrack(sp_session *session, sp_track *track) {
  sp_error error;
 
  error = sp_session_player_load(session, track);	 
  if (error != SP_ERROR_OK) {
    cerr << "Error player_loading: " << sp_error_message(error) << endl;
    return -1;
  }

  error = sp_session_player_play(session, true);
  if (error != SP_ERROR_OK) {
    cerr << "Error playing: " << sp_error_message(error) << endl;
    return -1;
  }

  // Wait until played
  class : public Workflow {
    void end_of_track() { 
      workflowDone();
    }
  } workflow;
  workflow.run();
  if (workflow.getHadError()) {
    cerr << "Error waiting for track to play: " << workflow.getErrorText() << endl;
    return -1;
  }
  return 0;
}

sp_session* SessionWrapper::getSession() {
  return session;
}
  
SessionWrapper* SessionWrapper::getSessionWrapper() {
  return singleton;
}

sp_error SessionWrapper::create(BaseAudioDriver *driver) {
  sp_error error;
  session = NULL;
  audioDriver = driver;
    
  // Init Audio
  audioDriver->init();
  
  // Default Session Config
  session_config.api_version          = SPOTIFY_API_VERSION;
  session_config.cache_location       = "/tmp/spotify";
  session_config.settings_location    = "/tmp/spotify";
  session_config.user_agent           = "Unnamed";
  session_config.device_id            = myMacEthernetAddress().c_str();
  session_config.userdata             = NULL;
  session_config.compress_playlists   = true;
  session_config.initially_unload_playlists = false;
  session_config.dont_save_metadata_for_playlists = false;

  configureSession(session_config);
  session_config.callbacks            = &session_callbacks;
  
  // Create Session
  error = sp_session_create(&session_config, &session);
  if (error != SP_ERROR_OK) {
	  return error;
  }
  
  // High Quality Bitrate
  error = sp_session_preferred_bitrate(session, SP_BITRATE_320k);
  if (error != SP_ERROR_OK) {
    cerr << "Error setting bitrate: " << sp_error_message(error) << endl;
  }
  error = sp_session_preferred_offline_bitrate(session, SP_BITRATE_320k, false);	
  if (error != SP_ERROR_OK) {
    cerr << "Error setting offline bitrate: " << sp_error_message(error) << endl;
  }

  return error;
}

void SessionWrapper::configureSession(sp_session_config &session_config) {
}

void SessionWrapper::logged_in(sp_session *session, sp_error error) {
  Event::queueEvent(Event::logged_in, error);
}

void SessionWrapper::logged_out(sp_session *session) {
  Event::queueEvent(Event::logged_out);
}

void SessionWrapper::metadata_updated(sp_session *session) {
  Event::queueEvent(Event::metadata_updated);
}

void SessionWrapper::connection_error(sp_session *session, sp_error error) {
  Event::queueEvent(Event::connection_error, error);
}

void SessionWrapper::message_to_user(sp_session *session, const char *message) {
  Event::queueEvent(Event::message_to_user, (string)message);
}

void SessionWrapper::notify_main_thread(sp_session *session) {
  Event::queueEvent(Event::notify_main_thread);
}

void SessionWrapper::play_token_lost(sp_session *session) {
  Event::queueEvent(Event::play_token_lost);
}

void SessionWrapper::log_message(sp_session *session, const char *data) {
  Event::queueEvent(Event::log_message, (string)data);
}

void SessionWrapper::end_of_track(sp_session *session) {
  Event::queueEvent(Event::end_of_track);
}

void SessionWrapper::streaming_error(sp_session *session, sp_error error) {
  Event::queueEvent(Event::streaming_error);
}

void SessionWrapper::userinfo_updated(sp_session *session) {
  Event::queueEvent(Event::userinfo_updated);
}

void SessionWrapper::offline_status_updated(sp_session *session) {
  Event::queueEvent(Event::offline_status_updated);
}

void SessionWrapper::offline_error(sp_session *session, sp_error error) {
  Event::queueEvent(Event::offline_error);
}

void SessionWrapper::credentials_blob_updated(sp_session *session, const char *blob) {
  Event::queueEvent(Event::credentials_blob_updated);
}

void SessionWrapper::connectionstate_updated(sp_session *session) {
  Event::queueEvent(Event::connectionstate_updated);
}

void SessionWrapper::unaccepted_licenses_updated(sp_session *session) {
  Event::queueEvent(Event::unaccepted_licenses_updated);
}

void SessionWrapper::scrobble_error(sp_session *session, sp_error error) {
  Event::queueEvent(Event::scrobble_error);
}

void SessionWrapper::private_session_mode_changed(sp_session *session, bool is_private) {
  Event::queueEvent(Event::private_session_mode_changed);
}

void SessionWrapper::get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats) {
  if (singleton->audioDriver) {
    singleton->audioDriver->get_audio_buffer_stats(session, stats);
  }
}

int SessionWrapper::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {
  if (singleton->audioDriver) {
    return singleton->audioDriver->music_delivery(session, format, frames, num_frames);
  }
  return 0;
}

void SessionWrapper::start_playback(sp_session *session) {
  if (singleton->audioDriver) {
    singleton->audioDriver->start_playback(session);
  }
}

void SessionWrapper::stop_playback(sp_session *session) {
  if (singleton->audioDriver) {
    singleton->audioDriver->stop_playback(session);
  }
}

// Playlist Container
void SessionWrapper::playlist_added(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata) {
  Event::queueEvent(Event::playlist_added);
  // TODO playlist, pos
}

void SessionWrapper::playlist_removed(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata) {
  Event::queueEvent(Event::playlist_removed);
  // TODO playlist, pos
}

void SessionWrapper::playlist_moved(sp_playlistcontainer *pc, sp_playlist *playlist, int position, int new_position, void *userdata) {
  Event::queueEvent(Event::playlist_moved);
  // TODO playlist, pos
}

void SessionWrapper::container_loaded(sp_playlistcontainer *pc, void *userdata) {
  Event::queueEvent(Event::container_loaded);
}


// Playlist Callbacks
void SessionWrapper::tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::tracks_added);
}

void SessionWrapper::tracks_removed(sp_playlist *pl, const int *tracks, int num_tracks, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::tracks_removed);
}

void SessionWrapper::tracks_moved(sp_playlist *pl, const int *tracks, int num_tracks, int new_position, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::tracks_moved);
}

void SessionWrapper::playlist_renamed(sp_playlist *pl, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::playlist_renamed);
}

void SessionWrapper::playlist_state_changed(sp_playlist *pl, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::playlist_state_changed);
}

void SessionWrapper::playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::playlist_update_in_progress);
}

void SessionWrapper::playlist_metadata_updated(sp_playlist *pl, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::playlist_metadata_updated);
}

void SessionWrapper::track_created_changed(sp_playlist *pl, int position, sp_user *user, int when, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::track_created_changed);
}

void SessionWrapper::track_seen_changed(sp_playlist *pl, int position, bool seen, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::track_seen_changed);
}

void SessionWrapper::description_changed(sp_playlist *pl, const char *desc, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::description_changed);
}

void SessionWrapper::image_changed(sp_playlist *pl, const byte *image, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::image_changed);
}

void SessionWrapper::track_message_changed(sp_playlist *pl, int position, const char *message, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::track_message_changed);
}

void SessionWrapper::subscribers_changed(sp_playlist *pl, void *userdata) {
  // TODO playlist etc
  Event::queueEvent(Event::subscribers_changed);
}
