#ifndef _TORNET_UDT_CHANNEL_HPP_
#define _TORNET_UDT_CHANNEL_HPP_
#include <tornet/net/channel.hpp>
#include <boost/asio.hpp>

namespace tornet {

  /**
   *  Provides in-order, gauranteed delivery of streams of
   *  data using a protocol similar to UDT.  All processing is
   *  done in the node thread.  Calls to write/read are posted
   *  to the node thread to complete. 
   */
  class udt_channel {
    public:
      udt_channel( const channel& c, uint16_t max_window_packets = 256 );
      ~udt_channel();

      void close();

      /**
       *  Blocks until all of @param b has been sent
       *
       *  Throws on error.
       *
       *  @return bytes read
       */
      size_t write( const boost::asio::const_buffer& b );
      
      /**
       *  Blocks until all of @param b has been filled.
       *
       *  Throws on error.
       *
       *  @return bytes read
       */
      size_t read( const boost::asio::mutable_buffer& b );

    private:
      class udt_channel_private* my;
  };

}

#endif
