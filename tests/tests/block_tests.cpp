/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <boost/test/unit_test.hpp>

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/account_object.hpp>
#include <eos/chain/key_value_object.hpp>
#include <eos/chain/block_summary_object.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

using namespace eos;
using namespace chain;


/*
 *  This code is saved because it provides an alternative implementation of string to int conversion.
   struct Tame {
      uint64_t value = 0;
      bool valid()const { return 0 == (value >> 60); }
      bool empty()const { return 0 == value; }
      bool good()const  { return !empty() && valid();  }

      Tame( const char* str )   { set(str);           } 
      Tame( const String& str ) { set( str.c_str() ); }

      void set( const char* str ) {
      try {
         idump((std::string(str)));
         const auto len = strnlen(str,14);
         value = 0;
         FC_ASSERT( len <= 13 );
         for( uint32_t i = 0; i < len && i < 12; ++i )
            value |= uint64_t(char_to_symbol( str[i] )) << (59-(i*5));
         if( len == 13 )
            value |= (0x0f & char_to_symbol( str[ 12 ] ));
      }FC_CAPTURE_AND_RETHROW( (str) ) }


      uint8_t char_to_symbol( char c ) const {
         static const char* charmap = ".abcdefghijklmnopqrstuvwxyz12345";
         if( c >= 'a' && c <= 'z' ) {
            idump((int(((c-'a') + 1)))(string()+c));
          //  FC_ASSERT( charmap[((c-'a') + 1)] == c );
            return uint8_t(c - 'a') + 1;
         }
         if( c >= '1' && c <= '5' ) {
            idump((int(((c-'1') + 26)))(string()+c));
           // FC_ASSERT( charmap[((c-'1') + 26)] == c );
            return uint8_t(c - '1') + 27;
         }
         FC_ASSERT( c == '.', "invalid character '${c}' (${i}) in Tame string", ("c", String(&c,1))("i",int(c)) );
         return 0;
      }

      Tame( uint64_t v = 0 ):value(v){
      //   FC_ASSERT( !(v>>(5*12)), "invalid name id" );
      }

      explicit operator String()const {
         static const char* charmap = ".abcdefghijklmnopqrstuvwxyz12345";
         String str;
         uint64_t tmp = value;
         for( uint32_t i = 0; i < 12; ++i ) {
            str += charmap[ 0x1f & (tmp >> (59-i*5)) ];
         }
         str += charmap[0x0f & tmp];
         boost::algorithm::trim_right_if( str, []( char c ){ return c == '.'; } );
         return str;
      }

      String toString() const { return String(*this); }

      Tame& operator=( uint64_t v ) {
         value = v;
         return *this;
      }

      Tame& operator=( const String& n ) {
         value = Tame(n).value;
         return *this;
      }
      Tame& operator=( const char* n ) {
         value = Tame(n).value;
         return *this;
      }

      template<typename Stream>
      friend Stream& operator << ( Stream& out, const Tame& n ) {
         return out << String(n);
      }

      friend bool operator < ( const Tame& a, const Tame& b ) { return a.value < b.value; }
      friend bool operator > ( const Tame& a, const Tame& b ) { return a.value > b.value; }
      friend bool operator == ( const Tame& a, const Tame& b ) { return a.value == b.value; }
      friend bool operator != ( const Tame& a, const Tame& b ) { return a.value != b.value; }

      operator bool()const            { return value; }
      operator uint64_t()const        { return value; }
   };
*/


BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_AUTO_TEST_CASE(name_test) {
   using eos::types::Name;
   Name temp;
   temp = "temp";
   BOOST_CHECK_EQUAL( String("temp"), String(temp) );
   BOOST_CHECK_EQUAL( String("temp.temp"), String(Name("temp.temp")) );
   BOOST_CHECK_EQUAL( String(""), String(Name()) );
   BOOST_REQUIRE_EQUAL( String("hello"), String(Name("hello")) );
   BOOST_REQUIRE_EQUAL( Name(-1), Name(String(Name(-1))) );
   BOOST_REQUIRE_EQUAL( String(Name(-1)), String(Name(String(Name(-1)))) );
}

// Simple test of block production and head_block_num tracking
BOOST_FIXTURE_TEST_CASE(produce_blocks, testing_fixture)
{ try {
      Make_Blockchain(chain)

      BOOST_CHECK_EQUAL(chain.head_block_num(), 0);
      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);
      chain.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain.head_block_num(), 6);
      chain.produce_blocks(chain.get_global_properties().active_producers.size());
      BOOST_CHECK_EQUAL(chain.head_block_num(), chain.get_global_properties().active_producers.size() + 6);
} FC_LOG_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE(order_dependent_transactions, testing_fixture)
{ try {
      Make_Blockchain(chain);
      chain.produce_blocks(10);

      Make_Account(chain, newguy);
      Transfer_Asset(chain, inita, newguy, Asset(100));

      Transfer_Asset(chain, newguy, inita, Asset(1));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("newguy"), Asset(99));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000-199));
      chain.produce_blocks();

      BOOST_CHECK_EQUAL(chain.head_block_num(), 11);
      BOOST_CHECK(chain.fetch_block_by_number(11).valid());
      BOOST_CHECK(!chain.fetch_block_by_number(11)->cycles.empty());
      BOOST_CHECK(!chain.fetch_block_by_number(11)->cycles.front().empty());
      BOOST_CHECK_EQUAL(chain.fetch_block_by_number(11)->cycles.front().front().user_input.size(), 3);
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("newguy"), Asset(99));
      BOOST_CHECK_EQUAL(chain.get_liquid_balance("inita"), Asset(100000-199));
} FC_LOG_AND_RETHROW() }

// Simple test of block production when a block is missed
BOOST_FIXTURE_TEST_CASE(missed_blocks, testing_fixture)
{ try {
      Make_Blockchain(chain)

      chain.produce_blocks();
      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);

      AccountName skipped_producers[3] = {chain.get_scheduled_producer(1),
                                          chain.get_scheduled_producer(2),
                                          chain.get_scheduled_producer(3)};
      auto next_block_time = chain.get_slot_time(4);
      auto next_producer = chain.get_scheduled_producer(4);

      BOOST_CHECK_EQUAL(chain.head_block_num(), 1);
      chain.produce_blocks(1, 3);
      BOOST_CHECK_EQUAL(chain.head_block_num(), 2);
      BOOST_CHECK_EQUAL(chain.head_block_time().to_iso_string(), next_block_time.to_iso_string());
      BOOST_CHECK_EQUAL(chain.head_block_producer(), next_producer);
      BOOST_CHECK_EQUAL(chain.get_producer(next_producer).total_missed, 0);

      for (auto producer : skipped_producers) {
         BOOST_CHECK_EQUAL(chain.get_producer(producer).total_missed, 1);
      }
} FC_LOG_AND_RETHROW() }

// Simple sanity test of test network: if databases aren't connected to the network, they don't sync to eachother
BOOST_FIXTURE_TEST_CASE(no_network, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2));

      BOOST_CHECK_EQUAL(chain1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain1.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain2.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 5);
} FC_LOG_AND_RETHROW() }

// Test that two databases on the same network do sync to eachother
BOOST_FIXTURE_TEST_CASE(simple_network, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2))
      Make_Network(net, (chain1)(chain2))

      BOOST_CHECK_EQUAL(chain1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain1.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
      chain2.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Test that two databases joining and leaving a network sync correctly after a fork
BOOST_FIXTURE_TEST_CASE(forked_network, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2))
      Make_Network(net)

      BOOST_CHECK_EQUAL(chain1.head_block_num(), 0);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      chain1.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 0);
      BOOST_CHECK_NE(chain1.head_block_id().str(), chain2.head_block_id().str());

      net.connect_blockchain(chain1);
      net.connect_blockchain(chain2);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 1);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());

      chain2.produce_blocks(5);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 6);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());

      net.disconnect_database(chain1);
      chain1.produce_blocks(1, 1);
      chain2.produce_blocks();
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 7);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 7);
      BOOST_CHECK_NE(chain1.head_block_id().str(), chain2.head_block_id().str());

      chain2.produce_blocks(1, 1);
      net.connect_blockchain(chain1);
      BOOST_CHECK_EQUAL(chain1.head_block_num(), 8);
      BOOST_CHECK_EQUAL(chain2.head_block_num(), 8);
      BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Check that the recent_slots_filled bitmap is being updated correctly
BOOST_FIXTURE_TEST_CASE( rsf_missed_blocks, testing_fixture )
{ try {
      Make_Blockchain(chain)
      chain.produce_blocks();

      auto rsf = [&]() -> string
      {
         auto rsf = chain.get_dynamic_global_properties().recent_slots_filled;
         string result = "";
         result.reserve(64);
         for( int i=0; i<64; i++ )
         {
            result += ((rsf & 1) == 0) ? '0' : '1';
            rsf >>= 1;
         }
         return result;
      };

      auto pct = []( uint32_t x ) -> uint32_t
      {
         return uint64_t( config::Percent100 ) * x / 64;
      };

      BOOST_CHECK_EQUAL( rsf(),
         "1111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), config::Percent100 );

      chain.produce_blocks(1, 1);
      BOOST_CHECK_EQUAL( rsf(),
         "0111111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(127-64) );

      chain.produce_blocks(1, 1);
      BOOST_CHECK_EQUAL( rsf(),
         "0101111111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(126-64) );

      chain.produce_blocks(1, 2);
      BOOST_CHECK_EQUAL( rsf(),
         "0010101111111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(124-64) );

      chain.produce_blocks(1, 3);
      BOOST_CHECK_EQUAL( rsf(),
         "0001001010111111111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(121-64) );

      chain.produce_blocks(1, 5);
      BOOST_CHECK_EQUAL( rsf(),
         "0000010001001010111111111111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(116-64) );

      chain.produce_blocks(1, 8);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000010000010001001010111111111111111111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(108-64) );

      chain.produce_blocks(1, 13);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000100000000100000100010010101111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1000000000000010000000010000010001001010111111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1100000000000001000000001000001000100101011111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1110000000000000100000000100000100010010101111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks();
      BOOST_CHECK_EQUAL( rsf(),
         "1111000000000000010000000010000010001001010111111111111111111111"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(95-64) );

      chain.produce_blocks(1, 64);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000000"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(0) );

      chain.produce_blocks(1, 63);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000000000000000000000000000000000001"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(1) );

      chain.produce_blocks(1, 32);
      BOOST_CHECK_EQUAL( rsf(),
         "0000000000000000000000000000000010000000000000000000000000000000"
      );
      BOOST_CHECK_EQUAL( chain.producer_participation_rate(), pct(1) );
} FC_LOG_AND_RETHROW() }

// Check that a db rewinds to the LIB after being closed and reopened
BOOST_FIXTURE_TEST_CASE(restart_db, testing_fixture)
{ try {
      auto lag = EOS_PERCENT(config::BlocksPerRound, config::IrreversibleThresholdPercent);
      {
         Make_Blockchain(chain, x);

         chain.produce_blocks(20);

         BOOST_CHECK_EQUAL(chain.head_block_num(), 20);
         BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 20 - lag);
      }

      {
         Make_Blockchain(chain, x);

         // After restarting, we should have rewound to the last irreversible block.
         BOOST_CHECK_EQUAL(chain.head_block_num(), 20 - lag);
         chain.produce_blocks(5);
         BOOST_CHECK_EQUAL(chain.head_block_num(), 25 - lag);
      }
} FC_LOG_AND_RETHROW() }

// Check that a db which is closed and reopened successfully syncs back with the network, including retrieving blocks
// that it missed while it was down
BOOST_FIXTURE_TEST_CASE(sleepy_db, testing_fixture)
{ try {
      Make_Blockchain(producer)
      Make_Network(net, (producer))

      auto lag = EOS_PERCENT(config::BlocksPerRound, config::IrreversibleThresholdPercent);
      producer.produce_blocks(20);

      {
         // The new node, sleepy, joins, syncs, disconnects
         Make_Blockchain(sleepy, sleepy)
         net.connect_blockchain(sleepy);
         BOOST_CHECK_EQUAL(producer.head_block_num(), 20);
         BOOST_CHECK_EQUAL(sleepy.head_block_num(), 20);

         net.disconnect_database(sleepy);
      }

      // 5 new blocks are produced
      producer.produce_blocks(5);
      BOOST_CHECK_EQUAL(producer.head_block_num(), 25);

      // Sleepy is reborn! Check that it is now rewound to the LIB...
      Make_Blockchain(sleepy, sleepy)
      BOOST_CHECK_EQUAL(sleepy.head_block_num(), 20 - lag);

      // Reconnect sleepy to the network and check that it syncs up to the present
      net.connect_blockchain(sleepy);
      BOOST_CHECK_EQUAL(sleepy.head_block_num(), 25);
      BOOST_CHECK_EQUAL(sleepy.head_block_id().str(), producer.head_block_id().str());
} FC_LOG_AND_RETHROW() }

// Test reindexing the blockchain
BOOST_FIXTURE_TEST_CASE(reindex, testing_fixture)
{ try {
      auto lag = EOS_PERCENT(config::BlocksPerRound, config::IrreversibleThresholdPercent);
      {
         chainbase::database db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE);
         block_log log(get_temp_dir("log"));
         fork_database fdb;
         native_contract::native_contract_chain_initializer initr(genesis_state());
         testing_blockchain chain(db, fdb, log, initr, *this);

         chain.produce_blocks(100);

         BOOST_CHECK_EQUAL(chain.last_irreversible_block_num(), 100 - lag);
      }

      {
         chainbase::database db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE);
         block_log log(get_temp_dir("log"));
         fork_database fdb;
         native_contract::native_contract_chain_initializer initr(genesis_state());
         testing_blockchain chain(db, fdb, log, initr, *this);

         BOOST_CHECK_EQUAL(chain.head_block_num(), 100 - lag);
         chain.produce_blocks(20);
         BOOST_CHECK_EQUAL(chain.head_block_num(), 120 - lag);
      }
} FC_LOG_AND_RETHROW() }

// Test wiping a database and resyncing with an ongoing network
BOOST_FIXTURE_TEST_CASE(wipe, testing_fixture)
{ try {
      Make_Blockchains((chain1)(chain2))
      Make_Network(net, (chain1)(chain2))
      {
         // Create db3 with a temporary data dir
         Make_Blockchain(chain3)
         net.connect_blockchain(chain3);

         chain1.produce_blocks(3);
         chain2.produce_blocks(3);
         BOOST_CHECK_EQUAL(chain1.head_block_num(), 6);
         BOOST_CHECK_EQUAL(chain2.head_block_num(), 6);
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 6);
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain3.head_block_id().str());

         net.disconnect_database(chain3);
      }

      {
         // Create new chain3 with a new temporary data dir
         Make_Blockchain(chain3)
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 0);

         net.connect_blockchain(chain3);
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 6);

         chain1.produce_blocks(3);
         chain2.produce_blocks(3);
         BOOST_CHECK_EQUAL(chain1.head_block_num(), 12);
         BOOST_CHECK_EQUAL(chain2.head_block_num(), 12);
         BOOST_CHECK_EQUAL(chain3.head_block_num(), 12);
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain2.head_block_id().str());
         BOOST_CHECK_EQUAL(chain1.head_block_id().str(), chain3.head_block_id().str());
      }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
