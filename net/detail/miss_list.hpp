#ifndef _TORNET_UDT_MISS_LIST_HPP_
#define _TORNET_UDT_MISS_LIST_HPP_
#include <list>
#include <stdint.h>

namespace tornet {
  class miss_list {
    public:
      void add( uint16_t start, uint16_t end );
      void remove( uint16_t seq );
      bool contains( uint16_t seq )const;
      void print()const;
      void clear();

      template<typename Stream>
      friend Stream& operator << ( Stream& s, const miss_list& n ) {
        uint8_t len = (std::min)(uint16_t(128),uint16_t(n.m_ml.size())); 
        s.write( (char*)&len, sizeof(len) );
        mlist::const_iterator itr = n.m_ml.begin();
        while( itr != n.m_ml.end() && len ) {
          s.write( (const char*)&itr->first, sizeof(itr->first) );
          s.write( (const char*)&itr->second, sizeof(itr->second) );
          --len;
          ++itr;
        }
        return s;
      }

      template<typename Stream>
      friend Stream& operator >> ( Stream& s, miss_list& n ) {
        uint8_t len = 0;
        s.read( (char*)&len, sizeof(len) );
        n.m_ml.resize(len);
        mlist::const_iterator itr = n.m_ml.begin();
        while( itr != n.m_ml.end() && len ) {
          s.read( (char*)&itr->first, sizeof(itr->first) );
          s.read( (char*)&itr->second, sizeof(itr->second) );
          --len;
          ++itr;
        }
        return s;
      }

    private:
      typedef std::list<std::pair<uint16_t,uint16_t> > mlist;
      mlist m_ml;
  };

} // tornet 

#endif