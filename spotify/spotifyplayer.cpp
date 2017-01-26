/* */
#include <iostream>
#include <queue> 
#include <pthread.h>

#include <libspotify/api.h>

#include "spotifywrapper.h"
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
  
  sp_error error = session.create();
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

  // Read Playlists
  cout << "Reading Playlists ..." << endl;
  list<string> playlists;
  rc = session.loadUsersPlaylists(playlists);
  if (rc) exit(rc);
  for (list<string>::const_iterator p = playlists.begin(); p != playlists.end(); ++p) {
    cout << "> " << *p << endl;
  }
  cout << "Playlists Read" << endl;
  
  // Logout  
  cout << "Logging out ..." << endl;
  rc = session.logout();
  if (rc) exit(rc);
  cout << "Logged out" << endl;
  
  cout << "Exiting" << endl;
  return rc;
}
