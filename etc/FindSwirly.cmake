# - Find swirly
# Find the native SWIRLY headers and libraries.
#
#  SWIRLY_INCLUDE_DIRS - where to find swirly/ash/Defs.hpp, etc.
#  SWIRLY_LIBRARIES    - List of libraries when using swirly.
#  SWIRLY_FOUND        - True if swirly found.

#=============================================================================
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2011 Kitware, Inc., Insight Software Consortium
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ------------------------------------------------------------------------------
#
# The above copyright and license notice applies to distributions of
# CMake in source and binary form.  Some source files contain additional
# notices of original copyright by their contributors; see each source
# for details.  Third-party software packages supplied with CMake under
# compatible licenses provide their own copyright notices documented in
# corresponding subdirectories.
#
# ------------------------------------------------------------------------------
#
# CMake was initially developed by Kitware with the following sponsorship:
#
#  * National Library of Medicine at the National Institutes of Health
#    as part of the Insight Segmentation and Registration Toolkit (ITK).
#
#  * US National Labs (Los Alamos, Livermore, Sandia) ASC Parallel
#    Visualization Initiative.
#
#  * National Alliance for Medical Image Computing (NAMIC) is funded by the
#    National Institutes of Health through the NIH Roadmap for Medical Research,
#    Grant U54 EB005149.
#
#  * Kitware, Inc.
#=============================================================================

# Look for the header file.
FIND_PATH(SWIRLY_INCLUDE_DIR NAMES swirly/ash/Defs.hpp
  HINTS "$ENV{SWIRLY_HOME}/include")

# Look for the libraries.
FIND_LIBRARY(SWIRLY_TEA_LIBRARY NAMES swirly_tea
  HINTS "$ENV{SWIRLY_HOME}/lib")
FIND_LIBRARY(SWIRLY_FIR_LIBRARY NAMES swirly_fir
  HINTS "$ENV{SWIRLY_HOME}/lib")
FIND_LIBRARY(SWIRLY_FIG_LIBRARY NAMES swirly_fig
  HINTS "$ENV{SWIRLY_HOME}/lib")
FIND_LIBRARY(SWIRLY_ELM_LIBRARY NAMES swirly_elm
  HINTS "$ENV{SWIRLY_HOME}/lib")
FIND_LIBRARY(SWIRLY_ASH_LIBRARY NAMES swirly_ash
  HINTS "$ENV{SWIRLY_HOME}/lib")

# Handle the QUIETLY and REQUIRED arguments and set SWIRLY_FOUND to TRUE if all listed variables are TRUE. 
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SWIRLY DEFAULT_MSG
  SWIRLY_INCLUDE_DIR
  SWIRLY_TEA_LIBRARY
  SWIRLY_FIR_LIBRARY
  SWIRLY_FIG_LIBRARY
  SWIRLY_ELM_LIBRARY
  SWIRLY_ASH_LIBRARY)

# Copy the results to the output variables.
IF(SWIRLY_FOUND)
  SET(SWIRLY_INCLUDE_DIRS ${SWIRLY_INCLUDE_DIR})
  SET(SWIRLY_LIBRARIES ${SWIRLY_FIR_LIBRARY} ${SWIRLY_FIG_LIBRARY} ${SWIRLY_ELM_LIBRARY} ${SWIRLY_ASH_LIBRARY})
ENDIF(SWIRLY_FOUND)

MARK_AS_ADVANCED(SWIRLY_INCLUDE_DIR SWIRLY_TEA_LIBRARY SWIRLY_FIR_LIBRARY SWIRLY_FIG_LIBRARY SWIRLY_ELM_LIBRARY SWIRLY_ASH_LIBRARY)
