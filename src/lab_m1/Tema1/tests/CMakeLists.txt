cmake_minimum_required(VERSION 3.16)
project(Tema1RulesTests LANGUAGES CXX)

enable_testing()

add_executable(tema1_rules_tests
    rules_tests.cpp
    ../game_types.cpp
    ../game_rules.cpp
    ../game_effects.cpp
)
target_compile_features(tema1_rules_tests PRIVATE cxx_std_11)
target_compile_definitions(tema1_rules_tests PRIVATE TEMA1_RULE_TESTS)
target_include_directories(tema1_rules_tests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../..")

if(MSVC)
    target_compile_options(tema1_rules_tests PRIVATE /W4)
else()
    target_compile_options(tema1_rules_tests PRIVATE -Wall -Wextra -pedantic)
endif()

add_test(NAME tema1_rules COMMAND tema1_rules_tests)
