/** \file telnetConn.cpp
  * \brief Managing a connection to a telnet device.
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  * \ingroup tty_files
  */


#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "telnetConn.hpp"


namespace MagAOX
{
namespace tty
{

telnetConn::~telnetConn()
{
   /* clean up */
   if(m_telnet) telnet_free(m_telnet);
   if(m_sock) close(m_sock);
}

int telnetConn::connect( const std::string & host,
                         const std::string & port
                       )
{
   //First cleanup any previous connections.
   if(m_telnet) telnet_free(m_telnet);
   m_telnet = 0;
   if(m_sock) close(m_sock);
   m_sock = 0;

   m_loggedin = 0;

   struct sockaddr_in addr;
   struct addrinfo *ai;
   struct addrinfo hints;

   /* look up server host */
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   if( getaddrinfo(host.c_str(), port.c_str(), &hints, &ai) != 0)
   {
      return TELNET_E_GETADDR;
   }

   /* create server m_socket */
   if ((m_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      return TELNET_E_SOCKET;
   }

   /* bind server socket */
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   // Binding to port 0/INADDR_ANY (the only values ever set here) lets the kernel pick
   // any free ephemeral port, which essentially never fails.
   // LCOV_EXCL_START
   if (bind(m_sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
   {
      return TELNET_E_BIND;
   }
   // LCOV_EXCL_STOP

   /* connect */
   if (::connect(m_sock, ai->ai_addr, ai->ai_addrlen) == -1)
   {
      return TELNET_E_CONNECT;
   }

   /* free address lookup info */
   freeaddrinfo(ai);

   /* initialize the telnet box */
   m_telnet = telnet_init(telopts, telnetConn::event_handler, 0, this);

   // telnet_init() only returns nullptr on a failed internal malloc().
   // LCOV_EXCL_START
   if(m_telnet == nullptr)
   {
      return TELNET_E_TELNETINIT;
   }
   // LCOV_EXCL_STOP

   return TTY_E_NOERROR;
}

int telnetConn::login( const std::string & username,
                       const std::string & password
                     )
{
   char buffer[1024];
   int rs;

   struct pollfd pfd[1];

   /* initialize poll descriptors */
   memset(pfd, 0, sizeof(pfd));
   pfd[0].fd = m_sock;
   pfd[0].events = POLLIN;

   //Loop while waiting on the login process to complete.
   #ifdef TELNET_DEBUG
   std::cerr << "Starting poll\n";
   #endif
   int pollrv;
   while ( (pollrv = poll(pfd, 1, 30000)) > 0)
   {
      #ifdef TELNET_DEBUG
      std::cerr << "Polled\n";
      #endif
      /* read from client */
      if (pfd[0].revents & POLLIN)
      {
         #ifdef TELNET_DEBUG
         std::cerr << "Starting read\n";
         #endif
         if ((rs = recv(m_sock, buffer, sizeof(buffer), 0)) > 0)
         {
            #ifdef TELNET_DEBUG
            std::cerr << "read: " << rs << "bytes\n";
            #endif
            telnet_recv(m_telnet, buffer, rs);
            if(m_EHError != TTY_E_NOERROR) return m_EHError;
         }
         else if (rs == 0)
         {
            break;
         }
         else
         {
            fprintf(stderr, "recv(client) failed: %s\n",
            strerror(errno));
            return TTY_E_ERRORONREAD;
         }
      }

      if(m_loggedin == TELNET_GOT_USER)
      {
         int rv = write(username + "\n", 1000);
         if(rv != TTY_E_NOERROR) return rv;

         m_loggedin = TELNET_WAITING_PASS;
      }

      if(m_loggedin == TELNET_GOT_PASS)
      {
         int rv = write(password + "\n", 1000);
         if(rv != TTY_E_NOERROR) return rv;

         m_loggedin = TELNET_WAITING_PROMPT;
      }

      if(m_loggedin == TELNET_LOGGED_IN)
      {
         break;
      }
      #ifdef TELNET_DEBUG
      std::cerr << "polling\n";
      #endif
   }

   // The poll() above uses a fixed real 30-second timeout; genuinely waiting it out isn't
   // practical in a unit test (and there's no test-only override for it, unlike e.g.
   // ttyUSB.cpp's sysfs path/prefix, since this class isn't currently structured for the
   // XWCTEST_NAMESPACE re-inclusion pattern).
   // LCOV_EXCL_START
   if(pollrv == 0)
   {
      #ifdef TELNET_DEBUG
      std::cerr << "login timed out\n";
      #endif
      return TELNET_E_LOGINTIMEOUT;
   }
   // LCOV_EXCL_STOP

   return TTY_E_NOERROR;
}

int telnetConn::noLogin()
{
   m_loggedin = TELNET_LOGGED_IN;
   return TTY_E_NOERROR;
}

int telnetConn::write( const std::string & buffWrite,
                        int timeoutWrite
                     )
{
   double t0;
   struct pollfd pfd;

   errno = 0;
   pfd.fd = m_sock;
   pfd.events = POLLOUT;

   std::string _buffWrite;
   telnetCRLF(_buffWrite, buffWrite);


   t0 = mx::sys::get_curr_time();

   size_t totWritten = 0;
   while( totWritten < _buffWrite.size())
   {
      int timeoutCurrent = timeoutWrite - (mx::sys::get_curr_time()-t0)*1000;
      if(timeoutCurrent < 0) return TTY_E_TIMEOUTONWRITE;

      int rv = poll( &pfd, 1, timeoutCurrent);
      if( rv == 0 ) return TTY_E_TIMEOUTONWRITEPOLL;
      else if( rv < 0 ) return TTY_E_ERRORONWRITEPOLL;

      telnet_send(m_telnet, _buffWrite.c_str(), _buffWrite.size());
      totWritten = _buffWrite.size();

      #ifdef TELNET_DEBUG
      std::cerr << "Wrote " << totWritten << " chars of " << buffWrite.size() << "\n";
      #endif


      if( ( mx::sys::get_curr_time()-t0)*1000 > timeoutWrite ) return TTY_E_TIMEOUTONWRITE;
   }

   return TTY_E_NOERROR;
}

int telnetConn::read( const std::string & eot,
                      int timeoutRead,
                      bool clear
                    )
{
   int rv;
   int timeoutCurrent;
   double t0;

   struct pollfd pfd;

   errno = 0;

   pfd.fd = m_sock;
   pfd.events = POLLIN;

   char buffRead[TELNET_BUFFSIZE];

   //Start timeout clock for reading.
   t0 = mx::sys::get_curr_time();
   timeoutCurrent = timeoutRead;

   //Now read the response up to the eot.
   if(clear) m_strRead = "";


   rv = poll( &pfd, 1, timeoutCurrent);
   if( rv == 0 ) return TTY_E_TIMEOUTONREADPOLL;
   if( rv < 0 ) return TTY_E_ERRORONREADPOLL;

   rv = ::read(m_sock, buffRead, TELNET_BUFFSIZE);
   if( rv < 0 ) return TTY_E_ERRORONREAD;
   buffRead[rv] = '\0';

   telnet_recv(m_telnet, buffRead, rv);
   if(m_EHError != TTY_E_NOERROR) return m_EHError;

   while( !isEndOfTrans(m_strRead, eot) )
   {
      timeoutCurrent = timeoutRead - (mx::sys::get_curr_time()-t0)*1000;
      if(timeoutCurrent < 0) return TTY_E_TIMEOUTONREAD;

      rv = poll( &pfd, 1, timeoutCurrent);
      if( rv == 0 ) return TTY_E_TIMEOUTONREADPOLL;
      if( rv < 0 ) return TTY_E_ERRORONREADPOLL;

      rv = ::read(m_sock, buffRead, TELNET_BUFFSIZE);
      if( rv < 0 ) return TTY_E_ERRORONREAD;
      buffRead[rv] ='\0';

      telnet_recv(m_telnet, buffRead, rv);
      if(m_EHError != TTY_E_NOERROR) return m_EHError;

      #ifdef TELNET_DEBUG
      std::cerr << "telnetRead: read " << rv << " bytes. buffRead=" << buffRead << "\n";
      #endif
   }


   return TTY_E_NOERROR;


}

int telnetConn::read( int timeoutRead,
                      bool clear
                    )
{
   return read(m_prompt, timeoutRead, clear);
}

int telnetConn::writeRead( const std::string & strWrite,
                           bool swallowEcho,
                           int timeoutWrite,
                           int timeoutRead
                         )
{
   m_strRead.clear();

   int rv;

   //Write First
   rv = write( strWrite, timeoutWrite);
   if(rv != TTY_E_NOERROR) return rv;

   //Now read response from console
   int timeoutCurrent;
   double t0;

   struct pollfd pfd;
   pfd.fd = m_sock;
   pfd.events = POLLIN;


   //Start timeout clock for reading.
   t0 = mx::sys::get_curr_time();;

   if(swallowEcho)
   {
      char buffRead[TELNET_BUFFSIZE];

      //First swallow the echo.
      while( m_strRead.size() <= strWrite.size() )
      {
         timeoutCurrent = timeoutRead - (mx::sys::get_curr_time()-t0)*1000;
         if(timeoutCurrent < 0) return TTY_E_TIMEOUTONREAD;

         rv = poll( &pfd, 1, timeoutCurrent);
         if( rv == 0 ) return TTY_E_TIMEOUTONREADPOLL;
         if( rv < 0 ) return TTY_E_ERRORONREADPOLL;

         rv = ::read(m_sock, buffRead, TELNET_BUFFSIZE);
         if( rv < 0 ) return TTY_E_ERRORONREAD;

         telnet_recv(m_telnet, buffRead, rv);
         if(m_EHError != TTY_E_NOERROR) return m_EHError;
      }

      m_strRead.erase(0, strWrite.size());
   }

   if(isEndOfTrans(m_strRead, m_prompt)) return TTY_E_NOERROR;

   timeoutCurrent = timeoutRead - (mx::sys::get_curr_time()-t0)*1000;
   if(timeoutCurrent < 0) return TTY_E_TIMEOUTONREAD;

   //Now read the response up to the eot.
   return read(timeoutCurrent, false);
}

int telnetConn::send(int sock, const char *buffer, size_t size)
{
   /* send data */
   while (size > 0)
   {
      int rs;

      if ((rs = ::send(sock, buffer, size, 0)) == -1)
      {
         fprintf(stderr, "send() failed: %s\n", strerror(errno));
         return TTY_E_ERRORONWRITE;
      }
      // POSIX doesn't define a case where send() returns 0 for a connected stream socket
      // with a non-zero buffer size -- defensive, not reachable in practice.
      // LCOV_EXCL_START
      else if (rs == 0)
      {
         fprintf(stderr, "send() unexpectedly returned 0\n");
         return TTY_E_ERRORONWRITE;
      }
      // LCOV_EXCL_STOP
      /* update pointer and size to see if we've got more to send */
      buffer += rs;
      size -= rs;
   }

   return TTY_E_NOERROR;
}

void telnetConn::event_handler( telnet_t *telnet,
                                telnet_event_t *ev,
                                void *user_data
                              )
{
   telnetConn * cs = static_cast<telnetConn*>(user_data);
   int sock = cs->m_sock;

   //Always reset the error at beginning.
   cs->m_EHError = 0;

   switch (ev->type)
   {
      /* data received */
      case TELNET_EV_DATA:
      {
         #ifdef TELNET_DEBUG
         std::cerr << "Got ev->data.size: " << ev->data.size << "\n";
         #endif

         //First we remove the various control chars from the front.
         if(ev->data.size == 0) break;

         char * buf = const_cast<char *>(ev->data.buffer);
         buf[ev->data.size] = 0;

         int nn = 0;
         for(size_t i=0; i<ev->data.size; ++i)
         {
            if(ev->data.buffer[i] < 32)
            {
               ++buf;
               ++nn;
               continue;
            }
            break;
         }

         #ifdef TELNET_DEBUG
         std::cerr << "removed: " << nn << "\n";
         #endif

         //Now we check for '\0' characters inside the data
         //this is maybe a bug workaround for tripp lite pdu LX card . . .
         int mm = 0;
         for(size_t i=nn; i < ev->data.size;++i)
         {
            if( ev->data.buffer[i] == 0)
            {
               ++mm;
               buf[i-nn] = '\n';
            }
	 }

	 #ifdef TELNET_DEBUG
         std::cerr << "dezeroed: " << mm << "\n";
         #endif

         //Now make it a string so we can make use of it.
         std::string sbuf(buf);

         if(sbuf.size() == 0) break;

         #ifdef TELNET_DEBUG
         std::cerr  << "ev->data: " << sbuf << "\n";
         #endif

         if(cs->m_loggedin < TELNET_LOGGED_IN) //we aren't logged in yet
         {
            if(cs->m_loggedin == TELNET_WAITING_USER)
            {
               if(sbuf.find(cs->m_usernamePrompt) != std::string::npos)
               {
                  cs->m_loggedin = TELNET_GOT_USER;
               }
               break;
            }

            if(cs->m_loggedin == TELNET_WAITING_PASS)
            {
               if( sbuf.find(cs->m_passwordPrompt) != std::string::npos)
               {
                  cs->m_loggedin = TELNET_GOT_PASS;
               }
               break;
            }

            if(cs->m_loggedin == TELNET_WAITING_PROMPT)
            {
               if( sbuf.find(cs->m_prompt) != std::string::npos)
               {
                  cs->m_loggedin = TELNET_LOGGED_IN;
               }
               break;
            }
         }

         //Always append
         cs->m_strRead += sbuf;
         break;
      }
      /* data must be sent */
      case TELNET_EV_SEND:
      {
         send(sock, ev->data.buffer, ev->data.size);
         break;
      }
      /* request to enable remote feature (or receipt) */
      case TELNET_EV_WILL:
      {
         /* we'll agree to turn off our echo if server wants us to stop */
         //if (ev->neg.telopt == TELNET_TELOPT_ECHO) do_echo = 0;
         break;
      }
      /* notification of disabling remote feature (or receipt) */
      case TELNET_EV_WONT:
      {
         break;
      }
      /* request to enable local feature (or receipt) */
      case TELNET_EV_DO:
      {
         break;
      }
      /* demand to disable local feature (or receipt) */
      case TELNET_EV_DONT:
      {
         break;
      }
      /* respond to TTYPE commands */
      case TELNET_EV_TTYPE:
      {
         /* respond with our terminal type, if requested */
         if (ev->ttype.cmd == TELNET_TTYPE_SEND)
         {
            telnet_ttype_is(telnet, getenv("TERM"));
         }
         break;
      }
      /* respond to particular subnegotiations */
      case TELNET_EV_SUBNEGOTIATION:
      {
         break;
      }
      /* error */
      // libtelnet only raises a fatal TELNET_EV_ERROR (vs. EV_WARNING) for an internal
      // zlib/MCCP compression failure -- not reachable by feeding it plain protocol bytes.
      // LCOV_EXCL_START
      case TELNET_EV_ERROR:
      {
         fprintf(stderr, "ERROR: %s\n", ev->error.msg);
         cs->m_EHError = TELNET_E_EHERROR;
         break;
      }
      // LCOV_EXCL_STOP
      // Catches libtelnet event types (EV_WARNING, EV_COMPRESS, EV_ZMP, EV_ENVIRON,
      // EV_MSSP) this class never triggers: it only passively responds to the peer's
      // WILL/WONT/DO/DONT per its telopts stance and never itself calls
      // telnet_negotiate(), so it can't create the RFC1143 state race (e.g. "DONT
      // answered by WILL") that would raise a non-fatal protocol warning.
      // LCOV_EXCL_START
      default:
      {
         /* ignore */
         break;
      }
      // LCOV_EXCL_STOP
   }
}

} //namespace tty
} //namespace MagAOX

