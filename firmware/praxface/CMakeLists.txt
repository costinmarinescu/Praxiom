add_library(praxface STATIC
  PraxWatchFace.cpp
  PraxWatchFace.h
  dna_bg_light.c
  dna_bg_dark.c
)
target_include_directories(praxface PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(praxface PUBLIC displayapp)
