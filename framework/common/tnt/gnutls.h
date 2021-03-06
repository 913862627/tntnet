/*
 * Copyright (C) 2006 Tommi Maekitalo
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * As a special exception, you may use this file as part of a free
 * software library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU Library
 * General Public License.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef TNT_GNUTLS_H
#define TNT_GNUTLS_H

#include <cxxtools/net/tcpstream.h>
#include <gnutls/gnutls.h>

/// @cond internal
namespace tnt
{
  class GnuTlsException : public std::runtime_error
  {
      unsigned long code;
      static std::string formatMessage(const char* function, int code_);

    public:
      explicit GnuTlsException(const std::string& message)
        : std::runtime_error(message),
          code(0)
      { }

      GnuTlsException(const char* function, int code_)
        : std::runtime_error(formatMessage(function, code_)),
          code(code_)
      { }

      unsigned long getCode() const
      { return code; }
  };

  class GnuTlsInit
  {
      static unsigned initCount;
      static gnutls_dh_params dhParams;

    public:
      GnuTlsInit();
      ~GnuTlsInit();

      gnutls_dh_params getDhParams() const  { return dhParams; }
  };

  class GnuTlsX509Cred
  {
      gnutls_certificate_credentials x509_cred;
      GnuTlsInit init;

    public:
      GnuTlsX509Cred(const char* certificateFile, const char* privateKeyFile);
      ~GnuTlsX509Cred();

      operator gnutls_certificate_credentials() const { return x509_cred; }
  };

  class GnuTlsServer : public cxxtools::net::TcpServer
  {
      GnuTlsX509Cred cred;

    public:
      GnuTlsServer(const char* certificateFile, const char* privateKeyFile);

      gnutls_certificate_credentials getCred() const  { return cred; }
  };

  class GnuTlsStream : public cxxtools::net::TcpSocket
  {
    public:
      struct FdInfo
      {
        int fd;
        int timeout;
      };

    private:
      gnutls_session session;
      bool connected;
      mutable FdInfo fdInfo;

    public:
      GnuTlsStream()
        : session(0),
          connected(false)
        { }

      explicit GnuTlsStream(const GnuTlsServer& server, bool inherit = false)
        : session(0),
          connected(false)
        { accept(server, inherit); }

      ~GnuTlsStream();

      void accept(const GnuTlsServer& server, bool inherit = false);
      void handshake(const GnuTlsServer& server);
      int sslRead(char* buffer, int bufsize) const;
      int sslWrite(const char* buffer, int bufsize) const;
      void shutdown();
  };

  class GnuTls_streambuf : public std::streambuf
  {
      GnuTlsStream& m_stream;
      char_type* m_buffer;
      unsigned m_bufsize;

    public:
      explicit GnuTls_streambuf(GnuTlsStream& stream, unsigned bufsize = 256, int timeout = -1);
      ~GnuTls_streambuf()
      { delete[] m_buffer; }

      void setTimeout(int t)   { m_stream.setTimeout(t); }
      int getTimeout() const   { return m_stream.getTimeout(); }

      /// overload std::streambuf
      int_type overflow(int_type c);
      /// overload std::streambuf
      int_type underflow();
      /// overload std::streambuf
      int sync();
  };

  class GnuTls_iostream : public GnuTlsStream, public std::iostream
  {
      GnuTls_streambuf m_buffer;

    public:
      explicit GnuTls_iostream(unsigned bufsize = 8192, int timeout = -1)
        : std::iostream(0),
          m_buffer(*this, bufsize, timeout)
        { init(&m_buffer); }

      explicit GnuTls_iostream(const GnuTlsServer& server, unsigned bufsize = 8192, int timeout = -1)
        : GnuTlsStream(server),
          std::iostream(0),
          m_buffer(*this, bufsize, timeout)
        { init(&m_buffer); }

      void setTimeout(int timeout)  { m_buffer.setTimeout(timeout); }
      int getTimeout() const        { return m_buffer.getTimeout(); }
  };
}
/// @endcond internal

#endif // TNT_GNUTLS_H

