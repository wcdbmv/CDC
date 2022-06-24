cmake_minimum_required(VERSION 3.20)
project(bsq)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(
	SOURCES
	src/ast.cpp
	src/bad_ast_visitor.cpp
	src/semantic_checker.cpp
	src/compiler.cpp
	src/ir_generator.cpp
	src/lexeme.cpp
	src/syntax_parser.cpp
	src/lexeme_reader.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} src/main.cpp)

llvm_map_components_to_libnames(llvm_libs support core irreader linker)

target_link_libraries(${PROJECT_NAME} ${llvm_libs})

add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/bsq_lib.ll
	COMMAND clang -S -emit-llvm ${CMAKE_SOURCE_DIR}/src/bsq_lib.c
)

add_custom_target(${PROJECT_NAME}-lib ALL DEPENDS bsq_lib.ll)

add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}-lib)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(${PROJECT_NAME} INTERFACE
	"$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Werror;-Wall;-Wextra;-Wpedantic-Wcast-align;-Wcast-qual;-Wconversion;-Wctor-dtor-privacy;-Wenum-compare;-Wextra-semi;-Wfloat-equal;-Wnon-virtual-dtor;-Wold-style-cast;-Woverloaded-virtual;-Wredundant-decls;-Wshadow;-Wsign-conversion;-Wsign-promo;-Wzero-as-null-pointer-constant>>"
	"$<${msvc_cxx}:$<BUILD_INTERFACE:-W4>>"
)
