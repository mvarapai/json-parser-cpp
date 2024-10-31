#include <catch2/catch_test_macros.hpp>
#include "json_parser.h"

TEST_CASE("Correctly find initial symbol position from trimmed string", "[JSONSource]")
{
	JSONSource source("test1.json");
	REQUIRE(source.GetSymbolSourcePosition(0) == JSONSource::Pos(1, 1));
	REQUIRE(source.GetSymbolSourcePosition(1) == JSONSource::Pos(2, 3));
	REQUIRE(source.GetSymbolSourcePosition(2) == JSONSource::Pos(2, 4));
	REQUIRE(source.GetSymbolSourcePosition(8) == JSONSource::Pos(2, 11));
}

TEST_CASE("Handle out of bounds input", "[JSONSource]")
{
	JSONSource source("test1.json");

	// Get the position of last character from the editor
	REQUIRE(source.GetSymbolSourcePosition(1000) == JSONSource::Pos(22, 1));	
}

TEST_CASE("Basic substring functionality", "[JSONString]")
{
	JSONSource source("test1.json");
	JSONString string = source.GetString();

	REQUIRE(!string.substr(0, 7).ToString().compare("{\"menu\""));
	REQUIRE(!string.substr(1, 6).ToString().compare("\"menu\""));
	REQUIRE(!string.substr(2, 4).ToString().compare("menu"));
}

TEST_CASE("Handle out of bounds input", "[JSONString]")
{
	JSONSource source("test1.json");
	JSONString string = source.GetString();

	// Should give initial string
	REQUIRE(!string.substr(0, 1000).ToString().compare(string.ToString()));	

	// Should give a string from 100 up to its end
	REQUIRE(!string.substr(100, 1000).ToString().compare(string.substr(100, string.Size() - 100).ToString()));
}
