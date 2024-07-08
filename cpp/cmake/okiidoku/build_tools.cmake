# SPDX-FileCopyrightText: 2020 David Fong
# SPDX-License-Identifier: GPL-3.0-or-later
include_guard(DIRECTORY)
if(NOT okiidoku_IS_TOP_LEVEL)
	return()
endif()

# do not wrap these with `block()`

if(NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
	find_program(CCACHE_EXE NAMES ccache DOC "Path to ccache executable")
	if(NOT "${CCACHE_EXE}" STREQUAL "CCACHE_EXE-NOTFOUND")
		set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_EXE}"
			# "--config-path" "${okiidoku_SOURCE_DIR}/cmake/okiidoku/ccache.conf"
			# "base_dir=${okiidoku_SOURCE_DIR}/../.."
			# "run_second_cpp=false"
		)
		# add_compile_options( # see run_second_cpp docs
		# 	"$<$<CXX_COMPILER_ID:GNU>:-fdirectives-only>"
		# 	"$<$<CXX_COMPILER_ID:Clang>:-frewrite-includes>"
		# )
	endif()
endif()

if(NOT DEFINED CMAKE_LINKER_TYPE)
	if(LINUX AND (CMAKE_CXX_COMPILER_ID STREQUAL GNU))
		find_program(GOLD_EXE NAMES gold DOC "Path to gold executable")
		if(NOT "${GOLD_EXE}" STREQUAL "GOLD_EXE-NOTFOUND")
			set(CMAKE_LINKER_TYPE GOLD)
		endif()
	endif()
	# disappointingly, Mold is slower for release builds and only slightly faster than Gold for debug.
	# if(LINUX)
	# 	find_program(MOLD_EXE NAMES mold DOC "Path to mold executable")
	# 	if(NOT "${MOLD_EXE}" STREQUAL "MOLD_EXE-NOTFOUND" AND LINUX)
	# 		set(CMAKE_LINKER_TYPE MOLD)
	# 		set(CMAKE_JOB_POOLS "compile=7" "link=1")
	# 		set(CMAKE_JOB_POOL_COMPILE "compile")
	# 		set(CMAKE_JOB_POOL_LINK "link")
	# 		return()
	# 	endif()
	# endif()
endif()