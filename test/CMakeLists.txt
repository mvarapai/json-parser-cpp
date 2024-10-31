include(FetchContent)
FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG v3.0.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE json_parser_lib Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(tests)