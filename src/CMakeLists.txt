project(minestars)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)

# Add custom SemiDebug build mode
set(CMAKE_CXX_FLAGS_SEMIDEBUG "-O1 -g -Wall" CACHE STRING
	"Flags used by the C++ compiler during semidebug builds."
	FORCE
)
set(CMAKE_C_FLAGS_SEMIDEBUG "-O1 -g -Wall -pedantic" CACHE STRING
	"Flags used by the C compiler during semidebug builds."
	FORCE
)
mark_as_advanced(
	CMAKE_CXX_FLAGS_SEMIDEBUG
	CMAKE_C_FLAGS_SEMIDEBUG
)
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
	"Choose the type of build. Options are: None Debug SemiDebug RelWithDebInfo MinSizeRel."
	FORCE
)


# Set some random things default to not being visible in the GUI
mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)

option(USE_SDL "Use SDL2 for window management" FALSE)
option(USE_STATIC_BUILD "Link additional libraries needed for static build" FALSE)

option(ENABLE_CURL "Enable cURL support for fetching media" TRUE)
set(USE_CURL TRUE)

if(ENABLE_CURL)
	find_package(CURL)
	if (CURL_FOUND)
		message(STATUS "cURL support enabled.")
		set(USE_CURL TRUE)
	endif()
endif()


option(ENABLE_GETTEXT "Use GetText for internationalization" TRUE)
set(USE_GETTEXT FALSE)

if(ENABLE_GETTEXT)
	find_package(GettextLib)
	if(GETTEXTLIB_FOUND)
		if(WIN32)
			message(STATUS "GetText library: ${GETTEXT_LIBRARY}")
			message(STATUS "GetText DLL: ${GETTEXT_DLL}")
			message(STATUS "GetText iconv DLL: ${GETTEXT_ICONV_DLL}")
		endif()
		set(USE_GETTEXT TRUE)
		message(STATUS "GetText enabled; locales found: ${GETTEXT_AVAILABLE_LOCALES}")
	endif(GETTEXTLIB_FOUND)
else()
	mark_as_advanced(GETTEXT_ICONV_DLL GETTEXT_INCLUDE_DIR GETTEXT_LIBRARY GETTEXT_MSGFMT)
	message(STATUS "GetText disabled.")
endif()

option(ENABLE_FREETYPE "Enable FreeType2 (TrueType fonts and basic unicode support)" TRUE)
set(USE_FREETYPE FALSE)

if(ENABLE_FREETYPE)
	find_package(Freetype)
	if(FREETYPE_FOUND)
		message(STATUS "Freetype enabled.")
		set(USE_FREETYPE TRUE)
	endif()
endif(ENABLE_FREETYPE)

option(ENABLE_CURSES "Enable ncurses console" TRUE)
set(USE_CURSES FALSE)

if(ENABLE_CURSES)
	find_package(Ncursesw)
	if(CURSES_FOUND)
		set(USE_CURSES TRUE)
		message(STATUS "ncurses console enabled.")
		include_directories(${CURSES_INCLUDE_DIRS})
	else()
		message(STATUS "ncurses not found!")
	endif()
endif(ENABLE_CURSES)

option(ENABLE_POSTGRESQL "Enable PostgreSQL backend" TRUE)
set(USE_POSTGRESQL FALSE)

if(ENABLE_POSTGRESQL)
	find_package("PostgreSQL")

	if(PostgreSQL_FOUND)
		set(USE_POSTGRESQL TRUE)
		message(STATUS "PostgreSQL backend enabled")
		# This variable is case sensitive, don't try to change it to POSTGRESQL_INCLUDE_DIR
		message(STATUS "PostgreSQL includes: ${PostgreSQL_INCLUDE_DIRS}")
		include_directories(${PostgreSQL_INCLUDE_DIRS})
	else()
		message(STATUS "PostgreSQL not found!")
	endif()
endif(ENABLE_POSTGRESQL)

option(ENABLE_LEVELDB "Enable LevelDB backend" TRUE)
set(USE_LEVELDB FALSE)

if(ENABLE_LEVELDB)
	find_library(LEVELDB_LIBRARY NAMES leveldb libleveldb)
	find_path(LEVELDB_INCLUDE_DIR db.h PATH_SUFFIXES leveldb)
	if(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB TRUE)
		message(STATUS "LevelDB backend enabled.")
		include_directories(${LEVELDB_INCLUDE_DIR})
	else()
		message(STATUS "LevelDB not found!")
	endif()
endif(ENABLE_LEVELDB)

option(ENABLE_SQLITE "Enable SQLite backend" TRUE)
set(USE_SQLITE TRUE)

OPTION(ENABLE_REDIS "Enable Redis backend" TRUE)
set(USE_REDIS FALSE)

if(ENABLE_REDIS)
	find_library(REDIS_LIBRARY hiredis)
	find_path(REDIS_INCLUDE_DIR hiredis.h PATH_SUFFIXES hiredis)
	if(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		set(USE_REDIS TRUE)
		message(STATUS "Redis backend enabled.")
		include_directories(${REDIS_INCLUDE_DIR})
	else(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		message(STATUS "Redis not found!")
	endif(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
endif(ENABLE_REDIS)


find_package(SQLite3 REQUIRED)

OPTION(ENABLE_PROMETHEUS "Enable prometheus client support" FALSE)
set(USE_PROMETHEUS FALSE)

if(ENABLE_PROMETHEUS)
	find_path(PROMETHEUS_CPP_INCLUDE_DIR NAMES prometheus/counter.h)
	find_library(PROMETHEUS_PULL_LIBRARY NAMES prometheus-cpp-pull)
	find_library(PROMETHEUS_CORE_LIBRARY NAMES prometheus-cpp-core)
	if(PROMETHEUS_CPP_INCLUDE_DIR AND PROMETHEUS_PULL_LIBRARY AND PROMETHEUS_CORE_LIBRARY)
		set(PROMETHEUS_LIBRARIES ${PROMETHEUS_PULL_LIBRARY} ${PROMETHEUS_CORE_LIBRARY})
		set(USE_PROMETHEUS TRUE)
		include_directories(${PROMETHEUS_CPP_INCLUDE_DIR})
	endif(PROMETHEUS_CPP_INCLUDE_DIR AND PROMETHEUS_PULL_LIBRARY AND PROMETHEUS_CORE_LIBRARY)
endif(ENABLE_PROMETHEUS)

if(USE_PROMETHEUS)
	message(STATUS "Prometheus client enabled.")
else(USE_PROMETHEUS)
	message(STATUS "Prometheus client disabled.")
endif(USE_PROMETHEUS)

OPTION(ENABLE_SPATIAL "Enable SpatialIndex AreaStore backend" TRUE)
set(USE_SPATIAL FALSE)

if(ENABLE_SPATIAL)
	find_library(SPATIAL_LIBRARY spatialindex)
	find_path(SPATIAL_INCLUDE_DIR spatialindex/SpatialIndex.h)
	if(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
		set(USE_SPATIAL TRUE)
		message(STATUS "SpatialIndex AreaStore backend enabled.")
		include_directories(${SPATIAL_INCLUDE_DIR})
	else(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
		message(STATUS "SpatialIndex not found!")
	endif(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
endif(ENABLE_SPATIAL)


if(NOT MSVC)
	set(USE_GPROF FALSE CACHE BOOL "Use -pg flag for g++")
endif()

# Haiku endian support
if(HAIKU)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE")
endif()

# Use cmake_config.h
add_definitions(-DUSE_CMAKE_CONFIG_H)

check_include_files(endian.h HAVE_ENDIAN_H)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

find_package(ZLIB REQUIRED)
set(PLATFORM_LIBS -lpthread ${CMAKE_DL_LIBS})
if(APPLE)
	set(PLATFORM_LIBS "-framework CoreFoundation" ${PLATFORM_LIBS})
else()
	check_library_exists(rt clock_gettime "" HAVE_LIBRT)
	if (HAVE_LIBRT)
		set(PLATFORM_LIBS -lrt ${PLATFORM_LIBS})
	endif(HAVE_LIBRT)
endif(APPLE)

# Prefer local iconv if installed
find_library(ICONV_LIBRARY iconv)
mark_as_advanced(ICONV_LIBRARY)
if (ICONV_LIBRARY)
	set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ICONV_LIBRARY})
endif()
if (HAIKU)
	set(PLATFORM_LIBS ${PLATFORM_LIBS} intl network)
endif()

# Add a target that always rebuilds cmake_config_githash.h
add_custom_target(GenerateVersion
	COMMAND ${CMAKE_COMMAND}
	-D "GENERATE_VERSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	-D "GENERATE_VERSION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-D "VERSION_STRING=${VERSION_STRING}"
	-D "DEVELOPMENT_BUILD=${DEVELOPMENT_BUILD}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")


add_subdirectory(threading)
add_subdirectory(content)
add_subdirectory(database)
add_subdirectory(gui)
add_subdirectory(mapgen)
add_subdirectory(network)
#add_subdirectory(script)
add_subdirectory(unittest)
add_subdirectory(core)
add_subdirectory(addons)
add_subdirectory(util)
add_subdirectory(irrlicht_changes)
add_subdirectory(server)
add_subdirectory(slave_proxy_net)

set(common_SRCS
	${core}
	${addons}
	${database_SRCS}
	${mapgen_SRCS}
	${server_SRCS}
	${proxy_resources}
	${content_SRCS}
	ban.cpp
	chat.cpp
	clientiface.cpp
	collision.cpp
	content_mapnode.cpp
	content_nodemeta.cpp
	convert_json.cpp
	craftdef.cpp
	debug.cpp
	defaultsettings.cpp
	emerge.cpp
	environment.cpp
	face_position_cache.cpp
	filesys.cpp
	gettext.cpp
	httpfetch.cpp
	hud.cpp
	inventory.cpp
	inventorymanager.cpp
	itemdef.cpp
	itemstackmetadata.cpp
	light.cpp
	log.cpp
	main.cpp
	map.cpp
	map_settings_manager.cpp
	mapblock.cpp
	mapnode.cpp
	mapsector.cpp
	metadata.cpp
	nameidmapping.cpp
	nodedef.cpp
	nodemetadata.cpp
	nodetimer.cpp
	noise.cpp
	objdef.cpp
	object_properties.cpp
	particles.cpp
	pathfinder.cpp
	player.cpp
	porting.cpp
	profiler.cpp
	raycast.cpp
	reflowscan.cpp
	remoteplayer.cpp
	rollback.cpp
	rollback_interface.cpp
	serialization.cpp
	slave_helpers.cpp
	server.cpp
	serverenvironment.cpp
	ServerNetworkEngine.cpp
	abm_threading.cpp
	serverlist.cpp
	settings.cpp
	staticobject.cpp
	terminal_chat_console.cpp
	texture_override.cpp
	tileanimation.cpp
	tool.cpp
	translation.cpp
	version.cpp
	voxel.cpp
	voxelalgorithms.cpp
	hud.cpp
	${common_network_SRCS}
	${JTHREAD_SRCS}
	#${common_SCRIPT_SRCS}
	${UTIL_SRCS}
)

if(BUILD_UNITTESTS)
	set(common_SRCS ${common_SRCS} ${UNITTEST_SRCS})
endif()


# This gives us the icon and file version information
if(WIN32)
	set(WINRESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/winresource.rc")
	set(MINETEST_EXE_MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/minetest.exe.manifest")
	if(MINGW)
		if(NOT CMAKE_RC_COMPILER)
			set(CMAKE_RC_COMPILER "windres.exe")
		endif()
		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o
			COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR}
			-i${WINRESOURCE_FILE}
			-o ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${WINRESOURCE_FILE} ${MINETEST_EXE_MANIFEST_FILE})
		SET(extra_windows_SRCS ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o)
	else(MINGW) # Probably MSVC
		set(extra_windows_SRCS ${WINRESOURCE_FILE} ${MINETEST_EXE_MANIFEST_FILE})
	endif(MINGW)
endif()

set(client_SRCS
	${client_SRCS}
	${common_SRCS}
	${gui_SRCS}
	${client_network_SRCS}
	${client_irrlicht_changes_SRCS}
	#${client_SCRIPT_SRCS}
)

if(BUILD_UNITTESTS)
	set(client_SRCS ${client_SRCS} ${UNITTEST_CLIENT_SRCS})
endif()

# Server sources
set(server_SRCS
	${common_SRCS}
)
list(SORT server_SRCS)

# Avoid source_group on broken CMake version.
# see issue #7074 #7075
if (CMAKE_VERSION VERSION_GREATER 3.8.1)
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${client_SRCS})
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${server_SRCS})
endif()

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}
	${IRRLICHT_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${SOUND_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIR}
	${CHACHA_INCLUDE_DIR}
	${GMP_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${X11_INCLUDE_DIR}
	${ASMJIT_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/script
)

if(USE_GETTEXT)
	include_directories(${GETTEXT_INCLUDE_DIR})
endif()

if(USE_FREETYPE)
	include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

if(USE_CURL)
	include_directories(${CURL_INCLUDE_DIR})
endif()


set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")

add_executable(${PROJECT_NAME}server ${server_SRCS} ${extra_windows_SRCS} ${ASMJIT_SRC})
target_link_options(${PROJECT_NAME}server PRIVATE -rdynamic)
if (CMAKE_BUILD_TYPE STREQUAL Debug)
	target_compile_options(${PROJECT_NAME}server PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${PROJECT_NAME}server PRIVATE -fsanitize=address)
endif()
add_dependencies(${PROJECT_NAME}server GenerateVersion)
target_link_libraries(
	${PROJECT_NAME}server
	${ZLIB_LIBRARIES}
	${SQLITE3_LIBRARY}
	${JSON_LIBRARY}
	${CHACHA_INCLUDE_DIR}
	${GMP_LIBRARY}
	${PLATFORM_LIBS}
	asmjit
	chacha
)
set_target_properties(${PROJECT_NAME}server PROPERTIES COMPILE_DEFINITIONS "SERVER")

if (USE_GETTEXT)
	target_link_libraries(${PROJECT_NAME}server ${GETTEXT_LIBRARY})
endif()
if (USE_CURSES)
	target_link_libraries(${PROJECT_NAME}server ${CURSES_LIBRARIES})
endif()
if (USE_POSTGRESQL)
	target_link_libraries(${PROJECT_NAME}server ${PostgreSQL_LIBRARIES})
endif()
if (USE_LEVELDB)
	target_link_libraries(${PROJECT_NAME}server ${LEVELDB_LIBRARY})
endif()
if (USE_REDIS)
	target_link_libraries(${PROJECT_NAME}server ${REDIS_LIBRARY})
endif()
if (USE_PROMETHEUS)
	target_link_libraries(${PROJECT_NAME}server ${PROMETHEUS_LIBRARIES})
endif()
if (USE_SPATIAL)
	target_link_libraries(${PROJECT_NAME}server ${SPATIAL_LIBRARY})
endif()
if(USE_CURL)
	target_link_libraries(
		${PROJECT_NAME}server
		${CURL_LIBRARY}
	)
endif()

# Blacklisted locales that don't work.
# see issue #4638
set(GETTEXT_BLACKLISTED_LOCALES
	ar
	he
	ky
	ms_Arab
	th
)

option(APPLY_LOCALE_BLACKLIST "Use a blacklist to avoid broken locales" TRUE)

if (GETTEXTLIB_FOUND AND APPLY_LOCALE_BLACKLIST)
	set(GETTEXT_USED_LOCALES "")
	foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
		if (NOT "${LOCALE}" IN_LIST GETTEXT_BLACKLISTED_LOCALES)
			list(APPEND GETTEXT_USED_LOCALES ${LOCALE})
		endif()
	endforeach()
	message(STATUS "Locale blacklist applied; Locales used: ${GETTEXT_USED_LOCALES}")
endif()

# Set some optimizations and tweaks

include(CheckCSourceCompiles)

# GCC or compatible compilers such as Clang
if(WARN_ALL)
	set(RELEASE_WARNING_FLAGS "-Wall")
else()
	set(RELEASE_WARNING_FLAGS "")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(WARNING_FLAGS "${WARNING_FLAGS} -Wsign-compare")
endif()

# Use a safe subset of flags to speed up math calculations:
# - we don't need errno or math exceptions
# - we don't deal with Inf/NaN or signed zero
set(MATH_FLAGS "-fno-math-errno -fno-trapping-math -ffinite-math-only -fno-signed-zeros")

set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${RELEASE_WARNING_FLAGS} ${WARNING_FLAGS} ${OTHER_FLAGS} -Wall -pipe -funroll-loops")
if(CMAKE_SYSTEM_NAME MATCHES "(Darwin|BSD|DragonFly)")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
else()
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
			AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
			AND CMAKE_CXX_COMPILER_VERSION MATCHES "^9\\.")
		# Clang 9 has broken -ffast-math on glibc
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MATH_FLAGS}")
	endif()
endif(CMAKE_SYSTEM_NAME MATCHES "(Darwin|BSD|DragonFly)")
set(CMAKE_CXX_FLAGS_SEMIDEBUG "-g -O1 -Wall ${WARNING_FLAGS} ${OTHER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall ${WARNING_FLAGS} ${OTHER_FLAGS}")

if(USE_GPROF)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
endif()

if(BUILD_SERVER)
	install(TARGETS ${PROJECT_NAME}server DESTINATION ${BINDIR})
endif()

if (USE_GETTEXT)
	set(MO_FILES)

	foreach(LOCALE ${GETTEXT_USED_LOCALES})
		set(PO_FILE_PATH "${GETTEXT_PO_PATH}/${LOCALE}/minetest.po")
		set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
		set(MO_FILE_PATH "${MO_BUILD_PATH}/minetest.mo")

		add_custom_command(OUTPUT ${MO_BUILD_PATH}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${MO_BUILD_PATH}
			COMMENT "mo-update [${LOCALE}]: Creating locale directory.")

		add_custom_command(
			OUTPUT ${MO_FILE_PATH}
			COMMAND ${GETTEXT_MSGFMT} -o ${MO_FILE_PATH} ${PO_FILE_PATH}
			DEPENDS ${MO_BUILD_PATH} ${PO_FILE_PATH}
			WORKING_DIRECTORY "${GETTEXT_PO_PATH}/${LOCALE}"
			COMMENT "mo-update [${LOCALE}]: Creating mo file."
			)

		set(MO_FILES ${MO_FILES} ${MO_FILE_PATH})
	endforeach()

	add_custom_target(translations ALL COMMENT "mo update" DEPENDS ${MO_FILES})
endif()
