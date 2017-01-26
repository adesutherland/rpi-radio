/* */
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#include <libspotify/api.h>
#include "spotifywrapper.h"

using namespace std;

const char *Event::ActionNames[] = {
  "nothing",
  // Session Events
  "logged_in",
  "logged_out",
  "metadata_updated",
  "connection_error",
  "message_to_user",
  "notify_main_thread",
  "play_token_lost",
  "log_message",
  "end_of_track",
  "streaming_error",
  "userinfo_updated",
  "offline_status_updated",
  "offline_error",
  "credentials_blob_updated",
  "connectionstate_updated",
  "unaccepted_licenses_updated",
  "scrobble_error",
  "private_session_mode_changed",
  
  // Playlist Container
  "playlist_added",
  "playlist_removed",
  "playlist_moved",
  "container_loaded"
  
  // Playlist
  "tracks_added",
  "tracks_removed",
  "tracks_moved",
  "playlist_renamed",
  "playlist_state_changed",
  "playlist_update_in_progress",
  "playlist_metadata_updated",
  "track_created_changed",
  "track_seen_changed",
  "description_changed",
  "image_changed",
  "track_message_changed",
  "subscribers_changed"
};

void Event::queueEvent(Event event) {
  // Queue the event and Kick the spotify main thread
  pthread_mutex_lock(&spotifyConditionMutex);
  actionQueue.push(event);
  pthread_cond_signal(&spotifyThreadCondition);
  pthread_mutex_unlock(&spotifyConditionMutex);
}

void Event::queueEvent(Action action) {
  Event event(action);
  queueEvent(event);
}

void Event::queueEvent(Action action, sp_error error) {
  Event event(action);
  event.setError(error);
  queueEvent(event);
}

void Event::queueEvent(Action action, std::string message) {
  Event event(action);
  event.setMessage(message);
  queueEvent(event);
}



Event Event::getNextEvent() {
  // Get the next event (maybe after a wait)
  pthread_mutex_lock(&spotifyConditionMutex);
  if (actionQueue.empty()) {
    struct timespec timeToWait;
    struct timeval now;
    gettimeofday(&now,NULL);
    // Because waiting for tracks has not callback I need to loop quite fast
    // I went for a 10th of a second - on the basis that
    // 1. fast enough for humans
    // 2. In a playlist we are not waiting for each one (they will have mostly all been loaded)
    timeToWait.tv_sec = now.tv_sec; 
    timeToWait.tv_nsec = (now.tv_usec + 100000UL) * 1000UL; // tv_nsec is nano, tv_usec is micro {sigh}!
          
    // Carry Seconds ...
    #define BILLION 1000000000
    if (timeToWait.tv_nsec >= BILLION) {
        timeToWait.tv_nsec -= BILLION;
        timeToWait.tv_sec++;
    }
          
    int rc = pthread_cond_timedwait(&spotifyThreadCondition, &spotifyConditionMutex, &timeToWait);
    if (rc == ETIMEDOUT) {
      // Timed out - push a nothing event
      Event event(nothing);
      actionQueue.push(event);
    }		
  }
  Event result = actionQueue.front();
  actionQueue.pop();
  pthread_mutex_unlock(&spotifyConditionMutex);
  return result;
}
    
Event::Event() {
  action = nothing;
  error = SP_ERROR_OK;
}
   
Event::Event(Action act) {
  action = act;
  error = SP_ERROR_OK;
}
          
Event::Action Event::getAction() {
  return action;
}

sp_error Event::getError() {
  return error;
}

string Event::getMessage() {
  return message;
}
  
void Event::setError(sp_error err) {
  error = err;
}

void Event::setMessage(std::string mess) {
  message = mess;
}

pthread_mutex_t Event::spotifyConditionMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t Event::spotifyThreadCondition = PTHREAD_COND_INITIALIZER;

queue<Event> Event::actionQueue;
