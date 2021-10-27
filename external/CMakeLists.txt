cmake_minimum_required(VERSION 3.14)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Dynawo
if (DYNAFLOW_LAUNCHER_USE_LOCAL_DYNAWO)
    set(DYNAWO_URL ${DYNAWO_HOME})
else()
    set(DYNAWO_URL https://github.com/dynawo/dynawo/releases/download/nightly/dynawo-cxx11-nightly.zip)
endif()
FetchContent_Declare(dynawo URL ${DYNAWO_URL})
FetchContent_GetProperties(dynawo)
if(NOT dynawo_POPULATED)
    FetchContent_Populate(dynawo)
    # We do not add sub directories here as no build is required
endif()
set(DYNAWO_HOME ${dynawo_SOURCE_DIR} PARENT_SCOPE)

