/* */
#ifndef SPOTIFY_WRAPPER_H
#define SPOTIFY_WRAPPER_H

#include <queue> 
#include <list>
#include <iostream>
#include <pthread.h>

#include <libspotify/api.h>

/**
 * Handle callbacks / Events (back into the Spotify main thread)
 */

class BaseAudioDriver;

class Event {
  
public:
  enum Action { 
    nothing,
    // Session Events
    logged_in,
    logged_out,
    metadata_updated,
    connection_error,
    message_to_user,
    notify_main_thread,
    play_token_lost,
    log_message,
    end_of_track,
    streaming_error,
    userinfo_updated,
    offline_status_updated,
    offline_error,
    credentials_blob_updated,
    connectionstate_updated,
    unaccepted_licenses_updated,
    scrobble_error,
    private_session_mode_changed,
     
    // Playlist Container
    playlist_added,
    playlist_removed,
    playlist_moved,
    container_loaded,
     
    // Playlist
    tracks_added,
    tracks_removed,
    tracks_moved,
    playlist_renamed,
    playlist_state_changed,
    playlist_update_in_progress,
    playlist_metadata_updated,
    track_created_changed,
    track_seen_changed,
    description_changed,
    image_changed,
    track_message_changed,
    subscribers_changed,

    // Always keep at end!! (this is so I can size the member pointer array)
    ACTION_AT_THE_END
   };
   
   static const char *ActionNames[];

   static void queueEvent(Event event);
   static void queueEvent(Action action);
   static void queueEvent(Action action, sp_error error);
   static void queueEvent(Action action, std::string message);
   static Event getNextEvent();
 
   Action getAction();
   sp_error getError();
   std::string getMessage();
   
private:
   static std::queue<Event> actionQueue;
   static pthread_mutex_t spotifyConditionMutex;
   static pthread_cond_t spotifyThreadCondition;
 
   Event();
   Event(Action act);
 
   void setError(sp_error err);
   void setMessage(std::string mess);
   
   Action action;
   sp_error error;
   std::string message;
};

/* Session setup */
class SessionWrapper {
public:

  SessionWrapper();
  sp_error create(BaseAudioDriver *driver);
  sp_session* getSession();
  static SessionWrapper* getSessionWrapper();
  BaseAudioDriver* getAudioDriver();
  
  // Convinient Functions
  int login(std::string userid, std::string password);
  int login();
  int onlineLogin(std::string userid, std::string password);
  int onlineLogin();
  int listUnacceptedLicenses(std::list<std::string> &licenseIDs, std::list<std::string> &licenseUrls);
  int acceptLicenses(std::list<std::string> &licenseIDs);
  int wait(int milliseconds);
  
  int logout();
  int loadPlaylistContainer();
  int loadUsersPlaylists(std::list<std::string> &playlists);
  sp_error loadPlaylist(sp_playlist* playlist);
  sp_playlist* getPlaylistByName(std::string playlistName);
  int loadPlaylistTracks(std::string playlistName, std::list<std::string> &tracks);
  int loadPlaylistTracks(sp_playlist* playlist, std::list<std::string> &tracks);
  sp_error loadTrack(sp_track* track);
  int playTrack(sp_session *session, sp_track* track);

  // Session Callbacks 
  static void logged_in(sp_session *session, sp_error error);
  static void logged_out(sp_session *session);
  static void metadata_updated(sp_session *session);
  static void connection_error(sp_session *session, sp_error error);
  static void message_to_user(sp_session *session, const char *message);
  static void notify_main_thread(sp_session *session);
  static void play_token_lost(sp_session *session);
  static void log_message(sp_session *session, const char *data);
  static void end_of_track(sp_session *session);
  static void streaming_error(sp_session *session, sp_error error);
  static void userinfo_updated(sp_session *session);
  static void offline_status_updated(sp_session *session);
  static void offline_error(sp_session *session, sp_error error);
  static void credentials_blob_updated(sp_session *session, const char *blob);
  static void connectionstate_updated(sp_session *session);
  static void unaccepted_licenses_updated(sp_session *session);
  static void scrobble_error(sp_session *session, sp_error error);
  static void private_session_mode_changed(sp_session *session, bool is_private);

  // Session Callbacks for the music playing system - No events
  static void get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats);
  static int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  static void start_playback(sp_session *session);
  static void stop_playback(sp_session *session);

  // Playlist Containter callbacks
  static void playlist_added(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata);
  static void playlist_removed(sp_playlistcontainer *pc, sp_playlist *playlist, int position, void *userdata);
  static void playlist_moved(sp_playlistcontainer *pc, sp_playlist *playlist, int position, int new_position, void *userdata);
  static void container_loaded(sp_playlistcontainer *pc, void *userdata);
  
  static void tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position, void *userdata);
  static void tracks_removed(sp_playlist *pl, const int *tracks, int num_tracks, void *userdata);
  static void tracks_moved(sp_playlist *pl, const int *tracks, int num_tracks, int new_position, void *userdata);
  static void playlist_renamed(sp_playlist *pl, void *userdata);
  static void playlist_state_changed(sp_playlist *pl, void *userdata);
  static void playlist_update_in_progress(sp_playlist *pl, bool done, void *userdata);
  static void playlist_metadata_updated(sp_playlist *pl, void *userdata);
  static void track_created_changed(sp_playlist *pl, int position, sp_user *user, int when, void *userdata);
  static void track_seen_changed(sp_playlist *pl, int position, bool seen, void *userdata);
  static void description_changed(sp_playlist *pl, const char *desc, void *userdata);
  static void image_changed(sp_playlist *pl, const byte *image, void *userdata);
  static void track_message_changed(sp_playlist *pl, int position, const char *message, void *userdata);
  static void subscribers_changed(sp_playlist *pl, void *userdata);

protected:
  virtual void configureSession(sp_session_config &session_config);

private:
  BaseAudioDriver *audioDriver = NULL;
  sp_session_callbacks session_callbacks;
  sp_playlistcontainer_callbacks playlistcontainer_callbacks;
  sp_playlist_callbacks playlist_callbacks;
  sp_session_config session_config;
  static SessionWrapper* singleton; // libspotify can only handle one session ...
  sp_session* session;
  sp_playlistcontainer* playlistContainer = NULL;
};

/* Workflow - controller */
class Workflow;

// Pointer to an event handler 
typedef  void (Workflow::*EventHandler)();

class Workflow {
public:  
  Workflow();
  void run();
  sp_error getSpotifyError();
  bool getHadError();
  std::string getErrorText();

protected:
  // Called within a event handler to get the event object
  Event* getCurrentEvent();
  
  // Called within a event handler to set the spotify error code
  void setSpotifyError(sp_error err);

  // Called within a event handler to set and error condition
  void setHadError(bool err);

  // Called within a event handler to set the error text
  void setErrorText(std::string text);
  
  // Called within a Workflow event handler to end the workdflow (once the handler returns)
  void workflowDone();

  // Default Session Event Handlers
  virtual void nothing();
  virtual void logged_in();
  virtual void logged_out();
  virtual void metadata_updated();
  virtual void connection_error();
  virtual void message_to_user();
  virtual void notify_main_thread();
  virtual void play_token_lost();
  virtual void log_message();
  virtual void end_of_track();
  virtual void streaming_error();
  virtual void userinfo_updated();
  virtual void offline_status_updated();
  virtual void offline_error();
  virtual void credentials_blob_updated();
  virtual void connectionstate_updated();
  virtual void unaccepted_licenses_updated();
  virtual void scrobble_error();
  virtual void private_session_mode_changed();
  
  // Playlist Container
  virtual void playlist_added();
  virtual void playlist_removed();  
  virtual void playlist_moved();  
  virtual void container_loaded();
  
  // Playlist
  virtual void tracks_added();
  virtual void tracks_removed();
  virtual void tracks_moved();
  virtual void playlist_renamed();
  virtual void playlist_state_changed();
  virtual void playlist_update_in_progress();
  virtual void playlist_metadata_updated();
  virtual void track_created_changed();
  virtual void track_seen_changed();
  virtual void description_changed();
  virtual void image_changed();
  virtual void track_message_changed();
  virtual void subscribers_changed();

private:
  Event* currentEvent;
  sp_error spotifyError;
  bool hadError;
  std::string errorText;
  bool done;
  EventHandler eventHandlers[Event::ACTION_AT_THE_END];
};

class BaseAudioDriver {
public:
  virtual ~BaseAudioDriver();

  virtual int init() = 0;
  virtual int done() = 0;

  // Session Callbacks for the music playing system
  virtual void get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats) = 0;
  virtual int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) = 0;
  virtual void start_playback(sp_session *session) = 0;
  virtual void stop_playback(sp_session *session) = 0;
  
  virtual void heartBeat() = 0; // Called in the event loop several times a second

protected:
  BaseAudioDriver();

};

#endif
