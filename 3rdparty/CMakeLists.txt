add_subdirectory(raylib)

set(CGRAPH_ONLY_CORE ON)
add_subdirectory(cgraph)

set(3rdparty_DIRS 3rdparty/raylib/src 3rdparty/cgraph/include PARENT_SCOPE)
set(3rdparty_LIBS raylib cgraph_core PARENT_SCOPE)