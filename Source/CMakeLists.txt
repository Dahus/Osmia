cmake_minimum_required(VERSION 3.20)
project(Osmia CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Shared (static lib) ────────────────────────────────────────────────
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS
    Shared/Common/*.cpp
    Shared/Library/*.cpp
    Shared/Server/*.cpp
)
add_library(Shared STATIC ${SHARED_SOURCES})
if(UNIX)
    set_target_properties(Shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(Shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# ── DatabaseODBC (static lib) ─────────────────────────────────────────
file(GLOB_RECURSE DBODBC_SOURCES CONFIGURE_DEPENDS
    Database/Implementation/DatabaseODBC/*.cpp
)
add_library(DatabaseODBC STATIC ${DBODBC_SOURCES})
if(UNIX)
    set_target_properties(DatabaseODBC PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(DatabaseODBC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(DatabaseODBC PRIVATE Shared)
if(UNIX)
    target_link_libraries(DatabaseODBC PRIVATE odbc)
endif()

# ── OsmiaLib (static lib) ─────────────────────────────────────────────
file(GLOB_RECURSE OSMIALIB_SOURCES CONFIGURE_DEPENDS
    OsmiaLib/*.cpp
)
add_library(OsmiaLib STATIC ${OSMIALIB_SOURCES})
if(UNIX)
    set_target_properties(OsmiaLib PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_include_directories(OsmiaLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(OsmiaLib PUBLIC Shared)

# ── Lua ───────────────────────────────────────────────────────────────
find_package(Lua REQUIRED)
target_include_directories(OsmiaLib PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(OsmiaLib PUBLIC ${LUA_LIBRARIES})

# ── Osmia (dynamic lib) ───────────────────────────────────────────────
file(GLOB_RECURSE OSMIA_SOURCES CONFIGURE_DEPENDS
    Osmia/*.cpp
)
add_library(Osmia SHARED ${OSMIA_SOURCES})
target_include_directories(Osmia PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Osmia PRIVATE OsmiaLib DatabaseODBC Shared)
if(UNIX)
    target_link_libraries(Osmia PRIVATE odbc dl)
    set_target_properties(Osmia PROPERTIES
        PREFIX ""
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH ON)
endif()

# ── Forwarder / HiveExt (dynamic lib) ─────────────────────────────────
file(GLOB_RECURSE FORWARDER_SOURCES CONFIGURE_DEPENDS
    Forwarder/*.cpp
)
add_library(HiveExt SHARED ${FORWARDER_SOURCES})
target_include_directories(HiveExt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(UNIX)
    target_link_libraries(HiveExt PRIVATE dl)
    set_target_properties(HiveExt PROPERTIES PREFIX "")
endif()

# ── Restarter (executable) ────────────────────────────────────────────
file(GLOB_RECURSE RESTARTER_SOURCES CONFIGURE_DEPENDS
    Restarter/*.cpp
)
if(UNIX)
    list(FILTER RESTARTER_SOURCES EXCLUDE REGEX "ServerMonitor_win\\.cpp")
else()
    list(FILTER RESTARTER_SOURCES EXCLUDE REGEX "ServerMonitor_lin\\.cpp")
endif()
add_executable(Restarter ${RESTARTER_SOURCES})
target_include_directories(Restarter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LUA_INCLUDE_DIR})
target_link_libraries(Restarter PRIVATE Shared ${LUA_LIBRARIES})
if(UNIX)
    target_link_libraries(Restarter PRIVATE dl pthread)
endif()