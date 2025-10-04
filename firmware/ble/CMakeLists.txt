add_library(praxble STATIC PraxService.cpp PraxService.h)
target_include_directories(praxble PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(praxble PUBLIC nimble)
