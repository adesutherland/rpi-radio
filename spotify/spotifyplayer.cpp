/* */
#include <iostream>
#include <queue> 
#include <pthread.h>

#include <libspotify/api.h>

#include "spotifywrapper.h"
#include "soundiodriver.h"
#include "dummydriver.h"
#include "appkey.c"

using namespace std;

int main(int argc, char* argv[]) {
  char *user = NULL;
  char *password = NULL;
  
  if (argc == 3) {
    user = argv[1];
    password = argv[2];
  }
  else if (argc != 1) {
    cout << "Usage: 'spotifyplayer user password', or just 'spotifyplayer' to log on as last user" << endl; 
    return -1;
  }
  
  int rc = 0;
  cout << "Spotify Player" << endl;
  
  class : public SessionWrapper {
    void configureSession(sp_session_config &session_config) {
      session_config.application_key      = g_appkey;
      session_config.application_key_size = g_appkey_size;
      session_config.user_agent           = "RPIRadio";
    }
  } session;
  
//  SoundIODriver driver;
  DummyDriver driver;
  
  sp_error error = session.create(&driver);
  if (error != SP_ERROR_OK) {
    cerr << "Error: " << sp_error_message(error) << endl;
    exit(-1);
  } 
  cout << "Created session successfully" << endl;  


  // Login
  cout << "Logging in ..." << endl;
  if (user) {
    rc = session.onlineLogin(user, password);
  }
  else {
    rc = session.onlineLogin();
  }
  if (rc) exit(rc);
  cout << "Logged in" << endl;

  {
    // Read Playlists
    cout << "Reading Playlists ..." << endl;
    list<string> playlists;
    rc = session.loadUsersPlaylists(playlists);
    if (rc) exit(rc);
    for (list<string>::const_iterator p = playlists.begin(); p != playlists.end(); ++p) {
      cout << "> " << *p << endl;
    }
    cout << "Playlists Read" << endl;
  
    cout << "Reading Tracks ..." << endl;
    list<string> tracks;
    for (list<string>::const_iterator p = playlists.begin(); p != playlists.end(); ++p) {
      tracks.clear();
      rc = session.loadPlaylistTracks(*p, tracks);
      if (rc) exit(rc);
      cout << "> " << *p << endl;
      for (list<string>::const_iterator t = tracks.begin(); t != tracks.end(); ++t) {
        cout << "--> " << *t << endl;
      }
    }
  cout << "Tracks Read" << endl;
  }
  
  {
    sp_error error;
    cout << "Play a track ..." << endl;
    list<string> playlists;
    rc = session.loadUsersPlaylists(playlists);
    if (rc) exit(rc);
    cout << "> Playlist:  " << playlists.front() << endl;
  
    list<string> tracks;
    rc = session.loadPlaylistTracks(playlists.front(), tracks);
    if (rc) exit(rc);
    cout << "> Track:  " << tracks.front() << endl;
    
    sp_playlist* pl = session.getPlaylistByName(playlists.front());
    error = session.loadPlaylist(pl);
    if (error != SP_ERROR_OK) {
      cerr << "Error load playlist: " << sp_error_message(error) << endl;
      exit(-1);
    }
    
    sp_track *tk = sp_playlist_track(pl, 0);
    error = session.loadTrack(tk);
    if (error != SP_ERROR_OK) {
      cerr << "Error load track: " << sp_error_message(error) << endl;
      exit(-1);
    }
    
    cout << "> Track:  " << sp_track_name(tk) << endl;
    
    if (session.playTrack(session.getSession(),tk)) {
      cerr << "Error playing track" << endl;
      exit(-1);
    }

    cout << "Track Played" << endl;
  }
  
  
  // Logout  
  cout << "Logging out ..." << endl;
  rc = session.logout();
  if (rc) exit(rc);
  cout << "Logged out" << endl;
  
  cout << "Exiting" << endl;
  return rc;
}
