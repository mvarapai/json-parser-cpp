add_library(json_parser_lib json_parser.cpp utilstr.cpp "query.cpp")
target_include_directories(json_parser_lib PUBLIC include)

add_executable(parser main.cpp command.cpp )
target_link_libraries(parser json_parser_lib)