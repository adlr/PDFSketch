# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

#
# GNU Make based build file.  For details on GNU Make see:
# http://www.gnu.org/software/make/manual/make.html
#

#
# Get pepper directory for toolchain and includes.
#
# If NACL_SDK_ROOT is not set, then assume it can be found three directories up.
#
THIS_MAKEFILE := $(abspath $(lastword $(MAKEFILE_LIST)))
NACL_SDK_ROOT ?= $(abspath $(dir $(THIS_MAKEFILE))../..)


# Project Build flags
WARNINGS := -Wno-long-long -Wall -Wswitch -Werror
CXXFLAGS := -pthread -std=gnu++11 -stdlib=libc++ $(WARNINGS)

#
# Compute tool paths
#
GETOS := python $(NACL_SDK_ROOT)/tools/getos.py
OSHELPERS = python $(NACL_SDK_ROOT)/tools/oshelpers.py
OSNAME := $(shell $(GETOS))
RM := $(OSHELPERS) rm

PNACL_TC_PATH := $(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_pnacl)
X86_64_TC_PATH := $(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_x86_newlib)
PNACL_CXX_SUFFIX=clang++
X86_64_CXX_SUFFIX=g++
PNACL_PREFIX=pnacl-
X86_64_PREFIX=x86_64-nacl-

TC_PATH=$(PNACL_TC_PATH)
CXX_SUFFIX=$(PNACL_CXX_SUFFIX)
PREFIX=$(PNACL_PREFIX)

PKG_CONFIG_PATH=${TC_PATH}/usr/lib/pkgconfig


CXX := $(TC_PATH)/bin/$(PREFIX)$(CXX_SUFFIX)
FINALIZE := $(TC_PATH)/bin/$(PREFIX)finalize
CXXFLAGS += -I$(NACL_SDK_ROOT)/include $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags cairo protobuf)
LDFLAGS := -L$(NACL_SDK_ROOT)/lib/pnacl/Release -lnacl_io -lppapi -lppapi_cpp $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs poppler-cpp poppler cairo fontconfig pixman-1 freetype2 protobuf) -lz -lexpat -ltar

#
# Disable DOS PATH warning when using Cygwin based tools Windows
#
CYGWIN ?= nodosfilewarning
export CYGWIN

BCOBJECTS=\
	pdfsketch.bc
PEXE=pdfsketch.pexe

OBJECTS=\
	pdfsketch.o \
	view.o \
	root_view.o \
	page_view.o \
	scroll_bar_view.o \
	scroll_view.o \
	document_view.o \
	rectangle.o \
	graphic.o \
	text_area.o \
	toolbox.o \
	undo_manager.o \
	checkmark.o \
	document.pb.o \
	file_io.o \
	graphic_factory.o \
	circle.o \
	squiggle.o

DISTFILES=\
	$(PEXE) \
	system.tar \
	manifest.json \
	index.js \
	main.js \
	style.css

CROSFONTSTARBALL=croscorefonts-1.23.0.tar.gz

# Declare the ALL target first, to make the 'all' target the default build
all: $(PEXE)

clean:
	rm -f $(PEXE) $(OBJECTS) $(BCOBJECTS)

$(OBJECTS): document.pb.cc

%.pb.cc: %.proto
	protoc --cpp_out=. document.proto

%.o: %.cc
	$(CXX) -c -o $@ $< -O2 $(CXXFLAGS)

pdfsketch.bc: $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) -O2 $(CXXFLAGS) $(LDFLAGS)

$(PEXE): $(BCOBJECTS)
	$(FINALIZE) -o $@ $(BCOBJECTS)

$(CROSFONTSTARBALL):
	wget http://commondatastorage.googleapis.com/chromeos-localmirror/distfiles/croscorefonts-1.23.0.tar.gz

system: $(CROSFONTSTARBALL)
	mkdir -p system/usr/share/fonts
	mkdir -p system/etc
	ln -s $(NACL_SDK_ROOT)/toolchain/linux_pnacl/usr/etc/fonts system/etc/fonts
	tar xzvf $< -C system
	mv system/croscorefonts-* system/usr/share/fonts/croscore

system.tar: system
	cp local.conf system/etc/fonts/
	tar cvhf system.tar -C system .

dist.zip: $(DISTFILES)
	rm -rf dist
	mkdir dist
	cp $(DISTFILES) dist
	cd dist && zip ../$@ * && cd ..

#
# Makefile target to run the SDK's simple HTTP server and serve this example.
#
HTTPD_PY := python $(NACL_SDK_ROOT)/tools/httpd.py

.PHONY: serve
serve: all
	$(HTTPD_PY) -C $(CURDIR)
