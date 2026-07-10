/** \file IndiMessage_test.cpp
  * \brief Catch2 tests for pcf::IndiMessage (INDI/libcommon/IndiMessage.cpp).
  */
#include "../../tests/catch2/catch.hpp"

#include "../libcommon/IndiMessage.hpp"
#include "../libcommon/IndiProperty.hpp"

using pcf::IndiMessage;
using pcf::IndiProperty;

namespace IndiMessage_test
{

SCENARIO( "IndiMessage construction, assignment, and accessors", "[IndiMessage]" )
{
   GIVEN( "messages built each way" )
   {
      WHEN( "constructing, copying, and assigning" )
      {
         IndiMessage m0;
         REQUIRE( m0.getType() == IndiMessage::Unknown );

         IndiProperty ip( IndiProperty::Text );
         ip.setDevice( "dev" );
         ip.setName( "prop" );

         IndiMessage m1( IndiMessage::Define, ip );
         REQUIRE( m1.getType() == IndiMessage::Define );
         REQUIRE( m1.getProperty().getName() == "prop" );

         IndiMessage m2( m1 );
         REQUIRE( m2.getType() == IndiMessage::Define );

         IndiMessage m3;
         m3 = m1;
         REQUIRE( m3.getProperty().getDevice() == "dev" );
         m3 = m3; // self-assignment branch
         REQUIRE( m3.getType() == IndiMessage::Define );

         // non-const getProperty and setProperty
         m3.getProperty().setName( "renamed" );
         REQUIRE( m3.getProperty().getName() == "renamed" );

         IndiProperty ip2( IndiProperty::Number );
         ip2.setName( "num" );
         m0.setProperty( ip2 );
         REQUIRE( m0.getProperty().getName() == "num" );
      }
   }

   GIVEN( "every message type" )
   {
      WHEN( "converting types to strings and back" )
      {
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::Unknown ) == "Unknown" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::Define ) == "Define" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::Delete ) == "Delete" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::EnableBLOB ) == "EnableBLOB" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::GetProperties ) == "GetProperties" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::Message ) == "Message" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::NewProperty ) == "NewProperty" );
         REQUIRE( IndiMessage::convertTypeToString( IndiMessage::SetProperty ) == "SetProperty" );

         REQUIRE( IndiMessage::convertStringToType( "Define" ) == IndiMessage::Define );
         REQUIRE( IndiMessage::convertStringToType( "Delete" ) == IndiMessage::Delete );
         REQUIRE( IndiMessage::convertStringToType( "EnableBLOB" ) == IndiMessage::EnableBLOB );
         REQUIRE( IndiMessage::convertStringToType( "GetProperties" ) == IndiMessage::GetProperties );
         REQUIRE( IndiMessage::convertStringToType( "Message" ) == IndiMessage::Message );
         REQUIRE( IndiMessage::convertStringToType( "NewProperty" ) == IndiMessage::NewProperty );
         REQUIRE( IndiMessage::convertStringToType( "SetProperty" ) == IndiMessage::SetProperty );
         REQUIRE( IndiMessage::convertStringToType( "nonsense" ) == IndiMessage::Unknown );
      }
   }
}

} //namespace IndiMessage_test
