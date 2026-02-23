include(CTest)

# Point both test binaries at the fixtures directory
set(FIXTURES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

# ---------- test_pickers ----------
add_executable(test_pickers test_pickers.cpp)
target_link_libraries(test_pickers PRIVATE triplefill)
target_include_directories(test_pickers PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/catch2")
add_test(NAME pickers COMMAND test_pickers)

# ---------- test_fill_golden ----------
add_executable(test_fill_golden test_fill_golden.cpp)
target_link_libraries(test_fill_golden PRIVATE triplefill)
target_include_directories(test_fill_golden PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/catch2")
target_compile_definitions(test_fill_golden PRIVATE
    TRIPLEFILL_FIXTURES_DIR="${FIXTURES_DIR}")
add_test(NAME fill_golden COMMAND test_fill_golden)
