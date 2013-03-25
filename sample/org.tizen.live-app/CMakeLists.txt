CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(live-app CXX)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkg REQUIRED
	dlog
	glib-2.0
	gio-2.0
	appcore-efl
	bundle
	provider
	aul
	ecore-x
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

ADD_DEFINITIONS("-DNDEBUG")
ADD_DEFINITIONS("-DLOG_TAG=\"${PROJECT_NAME}\"")
ADD_DEFINITIONS("-DPKGNAME=\"org.tizen.${PROJECT_NAME}\"")
ADD_DEFINITIONS(${pkg_CFLAGS})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Winline -Werror -g")

ADD_EXECUTABLE(${PROJECT_NAME}
	src/CMain.cpp
	src/CWindow.cpp
	src/CController.cpp
	src/CView.cpp
	src/CModel.cpp
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkg_LDFLAGS} "-ldl")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.${PROJECT_NAME}.xml DESTINATION /opt/share/packages)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/org.tizen.${PROJECT_NAME}.conf DESTINATION /opt/usr/live/org.tizen.${PROJECT_NAME}/etc)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/image/unknown.png DESTINATION /opt/usr/live/org.tizen.${PROJECT_NAME}/res/image)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION
	/opt/usr/apps/org.tizen.${PROJECT_NAME}/bin
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	GROUP_READ GROUP_EXECUTE
	WORLD_READ WORLD_EXECUTE)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/image/org.tizen.${PROJECT_NAME}.png DESTINATION /opt/share/icons/default/small)
# End of a file
