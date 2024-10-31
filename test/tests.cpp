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

	// Check the escape sequences and whitespaces. For now they are simple text.
	REQUIRE(!string.substr(15, 43).ToString().compare("some text and escape sequences \\\\\\n\\\"\\t}{]["));

	// Whitespaces must be preserved only whithin string literals. Outside they are trimmed.
	REQUIRE(!string.substr(1, 8).ToString().compare("\"menu\":{"));
}

TEST_CASE("Handle out of bounds input", "[JSONString]")
{
	JSONSource source("test1.json");
	JSONString string = source.GetString();

	// Should give initial string
	REQUIRE(!string.substr(0, 1000).ToString().compare(string.ToString()));	

	// Should give a string from 100 up to its end
	REQUIRE(!string.substr(100, 1000).ToString().compare(string.substr(100, string.Size() - 100).ToString()));

	// Should give the same string
	REQUIRE(!string.substr(1000, 1000).ToString().compare(string.ToString()));
}

TEST_CASE("Scanning string literals", "[JSONString]")
{
	JSONSource source("test1.json");
	JSONString string = source.GetString();

	JSONString text = string.substr(14, 50);
	size_t pos = 0;
	REQUIRE(!text.ScanString(pos).compare("some text and escape sequences \\\n\"\t}{]["));
	REQUIRE(pos == 45);
}

TEST_CASE("Scanning objects and lists", "[JSONString]")
{
	JSONSource source("test1.json");
	JSONString string = source.GetString();

	size_t pos = 0;
	JSONString body = string.substr(96, 1000);
	body = body.ScanListObjectBody(pos);
	REQUIRE(!body.ToString().compare("{\"value\":\"New\",\"onclick\":\"CreateNewDoc()\"}"));
	REQUIRE(pos == 42);
}
