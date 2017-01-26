/* */
#include <iostream>
#include <queue> 
#include <pthread.h>

#include <libspotify/api.h>

#include "spotifywrapper.h"
#include "appkey.c"

using namespace std;

int main() {
  int rc = 0;
  cout << "Spotify Test" << endl;
  
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
  rc = session.onlineLogin("robertsrpiradio", "rpirocks123");
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
