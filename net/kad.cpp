/** 
 *  @file tornet/net/kad.cpp
 *
 *  This file manages the KAD lookup algorithm.   
 *
 *  This algorithm depends upon a node being able to return a list of N 
 *  closest active node IDs.  
 *
 *
 */
#include "node.hpp"
#include "kad.hpp"

namespace tornet { 

  kad_search::kad_search( const node::ptr& local_node, const node::id_type& target, uint32_t n, uint32_t p ) 
  :m_node(local_node),m_target(target),m_target_dist( local_node->get_id()^target ), m_n(n), m_p(p) 
  {
     m_cur_status = idle;
  }

  void kad_search::start() {
     m_current_results.clear();
     m_cur_status   = searching;
     m_search_queue = m_node->find_nodes_near( m_target, m_n );
     m_pending.reserve(m_p);
     for( uint32_t i = 0; i < m_p ; ++i ) {
        m_pending.push_back( boost::cmt::async<void>( boost::bind( &kad_search::search_thread, shared_from_this() ) ) );
     }
  }

  void kad_search::wait() {
    for( uint32_t i = 0; i < m_pending.size(); ++i ) 
      m_pending[i].wait();
  }

  /**
   *  This method is multi-plexed among multiple coroutines, and exits when the
   *  search queue is empty, the desired ID is found, or the search is
   *  canceled.  The search queue is empty once all nodes in the
   *  search path are included in the result set.
   *
   *
   */
  void kad_search::search_thread() {
    while( m_search_queue.size() && m_cur_status == kad_search::searching ) {
        node::endpoint ep     = m_search_queue.begin()->second;
        node::id_type  nid    = m_search_queue.begin()->first ^ m_target;
        m_search_queue.erase(m_search_queue.begin());
        
        try {
          node::id_type  rtn    = m_node->connect_to(ep);
          if( filter( rtn ) ) {
              m_current_results[m_target^rtn] = rtn;
              if( m_current_results.size() > m_n )  {
                m_current_results.erase( --m_current_results.end() );
              }
          }

          if( rtn == m_target ) {
            m_cur_status = kad_search::done;
            return;
          }

          
          /** Only place the node in the search queue if it is closer than
             the furthest result.   If we are searching for 20 nodes and 
             already have 20 valid results, we only want the closest 20 and
             thus there is no need to consider a result further away.

             There is no need for the remote node to return nodes further away than our
             current 'worst result'.  Otherwise, we are consuming unecesary/redunant 
             bandwidth and ultimately searching almost every node on the network.
          */
          node::id_type limit;
          if( m_current_results.size() >= m_n && m_n ) {
            limit = (--m_current_results.end())->first; 
          }

          std::map<node::id_type,node::endpoint> rr = m_node->remote_nodes_near( rtn, m_target, m_n, limit );
          std::map<node::id_type,node::endpoint>::const_iterator rri = rr.begin();
          while( rri != rr.end() ) {
            if( m_current_results.find( rri->first ) == m_current_results.end() ) {

                m_search_queue[rri->first] = rri->second;
            }
            ++rri;
          }

        } catch ( const boost::exception& e ) {
          wlog( "%1%", boost::diagnostic_information(e) );
        }
    }
  }

  /**
   *  This method can be overloaded by derived classes to perform 
   *  operations on the node.   By default, it just returns true.
   *  If results are returned then it returns true, otherwise false.  Only nodes that
   *  return true are included in the results.
   */
  bool kad_search::filter( const node::id_type& id ) {
    return true;
  }

  const scrypt::sha1& kad_search::target()const { return m_target; }

} // namespace tornet


/*
  void node_private::kad_try_connection( const kad_search_state::ptr& kss, const connection::ptr& c ) {
    try {
      std::map<node_id, node::endpoint> peers         = c->find_peers( kss->target, 20 );
      std::map<node_id, node::endpoint>::iterator itr = peers.begin();
      while( itr != peers.end() ) {
        kss->search_queue[itr->first ^ target] = itr->second;
        ++itr;
      }
      while( peers.size() ) {
        node::endpoint ep = peers.front().second;
        peers.erase(peers.begin());
        node_id r = connect_to( ep );
        if( r == kss->target ) {
          kss->result.set_value( get_connection( r ) ); 
          return;
        } else {

        }
      }
    } catch ( const boost::exception& e ) {
      wlog( "Unexpected exception %1%", boost::diagnostic_information(e) );
    }
  }

  connection::ptr node_private::kad_find( const node_id& nid ) {
    node_id dist = nid ^ m_id;
    std::map<node_id,connection*>::iterator itr = m_dist_to_con.lower_bound( dist );
    if( itr->first == dist ) { return itr->second->shared_from_this(); }

    if( itr != m_dist_to_con.end() ) {
      // query K closest nodes to nid from itr, itr + 1, and itr + 2
      // when the result from any of those parallel queries returns, add the
      //  K closest to our list and then query the next closest.
      //  Stop when we have no closer nodes or when we find the node
      //  we are looking for.

      boost::shared_ptr<kad_search_state> kss( new kad_search_state() );
      kss->target      = nid;
      kss->target_dist = dist;
      return kss->wait();
    }

    return connection::ptr();
  }
  */