include(FetchContent)

set(OPENEXR_REPO "https://github.com/AcademySoftwareFoundation/openexr.git" CACHE STRING "OpenEXR repository for auto-build of OpenEXR")
set(OPENEXR_TAG "v3.2.4" CACHE STRING "Tag for auto-build of OpenEXR")

if (NOT TARGET OpenEXR::OpenEXR)
	message(STATUS "Downloading OpenEXR from GitHub: ${OPENEXR_REPO} (${OPENEXR_TAG})")
	FetchContent_Declare(
		OpenEXR
		GIT_REPOSITORY "${OPENEXR_REPO}"
		GIT_TAG "${OPENEXR_TAG}"
		GIT_SHALLOW ON
	)
	FetchContent_GetProperties(OpenEXR)
	if (NOT OpenEXR_POPULATED)
		message(STATUS "Populating OpenEXR dependency...")
		FetchContent_Populate(OpenEXR)
	endif()

	message(STATUS "Configuring OpenEXR for building...")
	execute_process(
		COMMAND
			${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" ../openexr-src -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/openexr
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/openexr-build
	)
	message(STATUS "Building OpenEXR and installing to ${CMAKE_BINARY_DIR}/openexr")
	execute_process(
		COMMAND
			${CMAKE_COMMAND} --build . --target install --config Release
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/_deps/openexr-build
	)
endif()
