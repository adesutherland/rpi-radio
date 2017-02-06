/* */
#include <fstream>
#include <libspotify/api.h>
#include "spotifywrapper.h"

using namespace std;

Workflow::Workflow() {
  spotifyError = SP_ERROR_OK;
  hadError = false;
  done = false;
  currentEvent = NULL;
  
  eventHandlers[0] = &Workflow::nothing;
  // Session Events
  eventHandlers[1] = &Workflow::logged_in;
  eventHandlers[2] = &Workflow::logged_out;
  eventHandlers[3] = &Workflow::metadata_updated;
  eventHandlers[4] = &Workflow::connection_error;
  eventHandlers[5] = &Workflow::message_to_user;
  eventHandlers[6] = &Workflow::notify_main_thread;
  eventHandlers[7] = &Workflow::play_token_lost;
  eventHandlers[8] = &Workflow::log_message;
  eventHandlers[9] = &Workflow::end_of_track;
  eventHandlers[10] = &Workflow::streaming_error;
  eventHandlers[11] = &Workflow::userinfo_updated;
  eventHandlers[12] = &Workflow::offline_status_updated;
  eventHandlers[13] = &Workflow::offline_error;
  eventHandlers[14] = &Workflow::credentials_blob_updated;
  eventHandlers[15] = &Workflow::connectionstate_updated;
  eventHandlers[16] = &Workflow::unaccepted_licenses_updated;
  eventHandlers[17] = &Workflow::scrobble_error;
  eventHandlers[18] = &Workflow::private_session_mode_changed;
  
  // Playlist Container
  eventHandlers[19] = &Workflow::playlist_added;
  eventHandlers[20] = &Workflow::playlist_removed;  
  eventHandlers[21] = &Workflow::playlist_moved;  
  eventHandlers[22] = &Workflow::container_loaded;
  
  // Playlist
  eventHandlers[23] = &Workflow::tracks_added;
  eventHandlers[24] = &Workflow::tracks_removed;
  eventHandlers[25] = &Workflow::tracks_moved;
  eventHandlers[26] = &Workflow::playlist_renamed;
  eventHandlers[27] = &Workflow::playlist_state_changed;
  eventHandlers[28] = &Workflow::playlist_update_in_progress;
  eventHandlers[29] = &Workflow::playlist_metadata_updated;
  eventHandlers[30] = &Workflow::track_created_changed;
  eventHandlers[31] = &Workflow::track_seen_changed;
  eventHandlers[32] = &Workflow::description_changed;
  eventHandlers[33] = &Workflow::image_changed;
  eventHandlers[34] = &Workflow::track_message_changed;
  eventHandlers[35] = &Workflow::subscribers_changed;
}

Event* Workflow::getCurrentEvent() {
  return currentEvent;
}

void Workflow::setSpotifyError(sp_error err) {
  spotifyError = err;
}

sp_error Workflow::getSpotifyError() {
  return spotifyError;
}

void Workflow::setHadError(bool err){
  hadError = err;
}

bool Workflow::getHadError() {
  return hadError;
}

void Workflow::setErrorText(string text) {
  errorText = text;
}

string Workflow::getErrorText() {
  return errorText;
}

void Workflow::workflowDone() {
  done = true;
}

void Workflow::run() {
  
  setSpotifyError(SP_ERROR_OK);
  setHadError(false);
  setErrorText("");
  done = false;
  BaseAudioDriver* audioDriver = SessionWrapper::getSessionWrapper()->getAudioDriver();
  
  do {
    Event event = Event::getNextEvent();
    currentEvent = &event;
    Event::Action action = event.getAction();
// if (action != Event::nothing) cout << "Event: " << Event::ActionNames[action] << endl; 
    
    // Call event handler
    (this->*eventHandlers[action])();
    
    // Audio Driver Heartbeat
    if (action == Event::nothing && audioDriver) {
      audioDriver->heartBeat();
    }
    
  } while ( !done );
}

// Default Event Handlers
void Workflow::nothing() {
}

void Workflow::logged_in() {
  if (getCurrentEvent()->getError() != SP_ERROR_OK) {
    setSpotifyError(getCurrentEvent()->getError());
    setHadError(true);
    setErrorText(sp_error_message(getCurrentEvent()->getError()));
    workflowDone();
  }
}

void Workflow::logged_out() {
  // Assuming an unexpected logout
  setHadError(true);
  setErrorText("Logged Out");
  workflowDone(); 
}

void Workflow::metadata_updated() {
}

void Workflow::connection_error() {
  setSpotifyError(getCurrentEvent()->getError());
  setHadError(true);
  setErrorText(sp_error_message(getCurrentEvent()->getError()));
  workflowDone();
}

void Workflow::message_to_user() {
  cout << "Message: " << getCurrentEvent()->getMessage() << endl;
}

void Workflow::notify_main_thread() {
  int timeout = 0;
  do { 
    sp_session_process_events(SessionWrapper::getSessionWrapper()->getSession(), &timeout); 
  } while (timeout == 0);
}

void Workflow::play_token_lost() {
// TODO Implement Pause
}

void Workflow::log_message() {
  // TODO - Proper logging framework please!  
  std::ofstream logfile;
  logfile.open("spotifyplayer.log", std::ios_base::app);
  logfile << getCurrentEvent()->getMessage(); // << endl; // Seems to have a endl anyway
}

void Workflow::end_of_track() {
}

void Workflow::streaming_error() {
  setSpotifyError(getCurrentEvent()->getError());
  setHadError(true);
  setErrorText(sp_error_message(getCurrentEvent()->getError()));
  workflowDone();
}

void Workflow::userinfo_updated() {
}

void Workflow::offline_status_updated() {
}

void Workflow::offline_error() {
  sp_error error = getCurrentEvent()->getError();
  if (error != SP_ERROR_OK) {
    setSpotifyError(getCurrentEvent()->getError());
    setHadError(true);
    setErrorText(sp_error_message(getCurrentEvent()->getError()));
    workflowDone();
  }
  // Note error_ok means offline error has cleared 
}

void Workflow::credentials_blob_updated() {
}

void Workflow::connectionstate_updated() {
}

void Workflow::unaccepted_licenses_updated() {
  // TODO - Complex - lets hope we never have to do it!
}

void Workflow::scrobble_error() {
  setSpotifyError(getCurrentEvent()->getError());
  setHadError(true);
  setErrorText(sp_error_message(getCurrentEvent()->getError()));
  workflowDone();
}

void Workflow::private_session_mode_changed() {
}

// Playlist Container Workflow Callbacks
void Workflow::playlist_added() {
}

void Workflow::playlist_removed() {
}

void Workflow::playlist_moved() {
}

void Workflow::container_loaded() {
}

// Playlist
void Workflow::tracks_added() {
}

void Workflow::tracks_removed() {
}

void Workflow::tracks_moved() {
}

void Workflow::playlist_renamed() {
}

void Workflow::playlist_state_changed() {
}

void Workflow::playlist_update_in_progress() {
}

void Workflow::playlist_metadata_updated() {
}

void Workflow::track_created_changed() {
}

void Workflow::track_seen_changed() {
}

void Workflow::description_changed() {
}

void Workflow::image_changed() {
}

void Workflow::track_message_changed() {
}

void Workflow::subscribers_changed() {
}
