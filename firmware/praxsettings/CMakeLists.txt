add_library(praxsettings STATIC
  PraxSettingsScreen.cpp
  PraxSettingsScreen.h
  PraxSettingsStore.cpp
  PraxSettingsStore.h
)
target_include_directories(praxsettings PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(praxsettings PUBLIC displayapp)
