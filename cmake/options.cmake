#
# Options for optional components.
#

option(WITH_COMPLETION
    "Install bash completions scripts for command line?" FALSE)
add_feature_info("Bash completion" WITH_COMPLETION
    "completion for PDAL command line")

option(BUILD_PLUGIN_CPD
    "Choose if the cpd filter should be built" FALSE)
add_feature_info("CPD plugin" BUILD_PLUGIN_CPD
    "Coherent Point Drift (CPD) computes rigid or nonrigid transformations between point sets")

option(BUILD_PLUGIN_DRACO
    "Choose if the draco filter should be built" FALSE)
add_feature_info("Draco plugin" BUILD_PLUGIN_DRACO
    "Read data in the draco format")

option(BUILD_PLUGIN_ICEBRIDGE
    "Choose if Icebridge support should be built" FALSE)
add_feature_info("Icebridge plugin" BUILD_PLUGIN_ICEBRIDGE
    "read data in the Icebridge format")

option(BUILD_PLUGIN_HDF
    "Choose if HDF support should be built" FALSE)
add_feature_info("HDF plugin" BUILD_PLUGIN_HDF
    "read data in the HDF format")

option(BUILD_PLUGIN_MATLAB
    "Choose if Matlab support should be built" FALSE)
add_feature_info("Matlab plugin" BUILD_PLUGIN_MATLAB
    "write data to a .mat file")

option(BUILD_PLUGIN_NITF
    "Choose if NITF support should be built (only install supported is from http://github.com/hobu/nitro)" FALSE)
add_feature_info("NITF plugin" BUILD_PLUGIN_NITF
    "read/write LAS data wrapped in NITF")

option(BUILD_PLUGIN_OPENSCENEGRAPH
    "Choose if OpenSceneGraph support should be built" FALSE)
add_feature_info("OpenSceneGraph plugin" BUILD_PLUGIN_OPENSCENEGRAPH
    "read/write OpenSceneGraph objects")

find_package(PostgreSQL QUIET)
option(BUILD_PLUGIN_PGPOINTCLOUD
    "Choose if PostgreSQL PointCloud support should be built"
    ${POSTGRESQL_FOUND})
add_feature_info("PostgreSQL PointCloud plugin" BUILD_PLUGIN_PGPOINTCLOUD
    "read/write PostgreSQL PointCloud objects")

option(BUILD_PLUGIN_RIVLIB
    "Choose if RiVLib support should be built" FALSE)
add_feature_info("RiVLib plugin" BUILD_PLUGIN_RIVLIB
    "read data in the RXP format")

option(BUILD_PLUGIN_RDBLIB
    "Choose if rdblib support should be built" FALSE)
add_feature_info("rdblib plugin" BUILD_PLUGIN_RDBLIB
    "read data in the RDB format")

option(BUILD_PLUGIN_MBIO
    "Choose if MBIO support should be built" FALSE)
add_feature_info("MBIO plugin" BUILD_PLUGIN_MBIO
    "add features that depend on MBIO")

option(BUILD_PLUGIN_FBX
    "Choose if FBX support should be built" FALSE)
add_feature_info("FBX plugin" BUILD_PLUGIN_FBX
    "add features that depend on FBX")

option(BUILD_PLUGIN_TEASER
    "Choose if TEASER++ support should be built" FALSE)
add_feature_info("TEASER++ plugin" BUILD_PLUGIN_TEASER
    "TEASER++ computes transformations between point sets")

option(BUILD_PLUGIN_TILEDB
    "Choose if TileDB support should be built" FALSE)
add_feature_info("TileDB plugin" BUILD_PLUGIN_TILEDB
    "read/write data from TileDB")

option(BUILD_PLUGIN_TRAJECTORY
    "Choose if LAS trajectory computation support should be built. Requires Ceres Solver." FALSE)
add_feature_info("Trajectory plugin" BUILD_PLUGIN_TRAJECTORY
    "Compute trajectory from LAS input")

option(BUILD_PLUGIN_E57
    "Choose if e57 support should be built" FALSE)
add_feature_info("E57 plugin" BUILD_PLUGIN_E57
    "read/write data to and from e57 format")

option(BUILD_PLUGIN_ARROW
    "Choose if Arrow support should be built" FALSE)
add_feature_info("Arrow plugin" BUILD_PLUGIN_ARROW
    "read/write data to and from Arrow format")

option(BUILD_TOOLS_NITFWRAP "Choose if nitfwrap tool should be built" FALSE)

option(WITH_TESTS
    "Choose if PDAL unit tests should be built" TRUE)
add_feature_info("Unit tests" WITH_TESTS "PDAL unit tests")

option(BUILD_DOCS
    "Choose if PDAL creates targets for building documentation" FALSE)

# Enable CTest and submissions to PDAL dashboard at CDash
# http://my.cdash.org/index.php?project=PDAL
option(ENABLE_CTEST
    "Enable CTest to support submissions of results to CDash at http://cdash.org" FALSE)

option(WITH_ABSEIL
    "Choose if PDAL should be built with Abseil support for testing" FALSE)
add_feature_info("Abseil debugging support " WITH_ABSEIL "unit tests")

option(WITH_BACKTRACE
    "Build with backtrace" ON)
add_feature_info("Backtrace" WITH_BACKTRACE
    "build with backtrace (Libunwind/Libexecinfo) support")

option(WITH_GCS
    "Build with Google storage IO support" TRUE)
add_feature_info("Google Cloud Storage" WITH_GCS
    "build with OpenSSL and others for Google storage IO support")

#
# Choose dependent options
#

include(CMakeDependentOption)

cmake_dependent_option(BUILD_PGPOINTCLOUD_TESTS
    "Choose if PostgreSQL PointCloud tests should be built"
    ON "BUILD_PLUGIN_PGPOINTCLOUD; WITH_TESTS" OFF)
cmake_dependent_option(BUILD_RIVLIB_TESTS
    "Choose if RiVLib tests should be built"
    ON "BUILD_PLUGIN_RIVLIB; WITH_TESTS" OFF)
cmake_dependent_option(BUILD_RDBLIB_TESTS
    "Choose if rdblib tests should be built"
    ON "BUILD_PLUGIN_RDBLIB; WITH_TESTS" OFF)
cmake_dependent_option(BUILD_PIPELINE_TESTS
    "Choose if pipeline tests should be built"
    OFF "WITH_TESTS" OFF)
cmake_dependent_option(BUILD_I3S_TESTS
    "Choose if I3S tests should be built"
    OFF "WITH_TESTS" OFF)
