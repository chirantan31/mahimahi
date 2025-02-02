/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_PROXY_HH
#define HTTP_PROXY_HH

#include <string>
#include <unordered_map>
#include <chrono>
#include <ctime>
#include <ratio>

#include "socket.hh"
#include "secure_socket.hh"
#include "http_response.hh"

using namespace std;
using namespace std::chrono;

class HTTPBackingStore;
class EventLoop;
class Poller;
class HTTPRequestParser;
class HTTPResponseParser;

class HTTPProxy
{
private:
    TCPSocket listener_socket_;

    template <class SocketType>
    void loop( SocketType & server, SocketType & client, HTTPBackingStore & backing_store );
    template <class SocketType>
    void loop( SocketType & server, SocketType & client );

    SSLContext server_context_, client_context_;

public:
    HTTPProxy( const Address & listener_addr );

    TCPSocket & tcp_listener( void ) { return listener_socket_; }

    /* backing_store is set to NULL if you don't want to save the request-reponse */
    void handle_tcp( HTTPBackingStore & backing_store );
    void handle_tcp( );

    /* register this HTTPProxy's TCP listener socket to handle events with
       the given event_loop, saving request-response pairs to the given
       backing_store (which is captured and must continue to persist).
       If you don't want to save the request-record, set backing_store to NULL */
    void register_handlers( EventLoop & event_loop, HTTPBackingStore & backing_store );
    void register_handlers( EventLoop & event_loop );

};

#endif /* HTTP_PROXY_HH */
