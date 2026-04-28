if(NOT WIN32)
    return()
endif()

set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/app_icon.rc")
set(APP_VERSION_RESOURCE "${GENERATED_DIR}/app_version.rc")

configure_file(
    "${CMAKE_SOURCE_DIR}/assets/app_version.rc.in"
    "${APP_VERSION_RESOURCE}"
    @ONLY
)

if(EXISTS "${APP_ICON_RESOURCE}")
    source_group("Resource Files" FILES "${APP_ICON_RESOURCE}")
    target_sources(${APP_NAME} PRIVATE "${APP_ICON_RESOURCE}")
else()
    message(WARNING "Resource file not found: ${APP_ICON_RESOURCE}")
endif()

if(EXISTS "${APP_VERSION_RESOURCE}")
    source_group("Resource Files" FILES "${APP_VERSION_RESOURCE}")
    target_sources(${APP_NAME} PRIVATE "${APP_VERSION_RESOURCE}")
endif()