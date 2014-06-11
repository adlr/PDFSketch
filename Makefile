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

#
# Compute tool paths
#
GETOS := python $(NACL_SDK_ROOT)/tools/getos.py
OSHELPERS = python $(NACL_SDK_ROOT)/tools/oshelpers.py
OSNAME := $(shell $(GETOS))

ifdef NATIVE

TC_PATH=/usr
PREFIX=
EXTRA_CXX_FLAGS=
EXTRA_LD_FLAGS=
CXX_PKGCONFIG=$(shell pkg-config --cflags cairo protobuf)  -I$(shell readlink -f $$HOME)/Code/poppler-0.24.5/cpp
LD_PKGCONFIG=-L$(shell readlink -f $$HOME)/Code/poppler-0.24.5/cpp/.libs -lpoppler-cpp $(shell pkg-config --libs cairo fontconfig pixman-1 freetype2 protobuf png) 
PROTOC=protoc
CXX_SUFFIX=g++
CXXFLAGS := -pthread -std=gnu++0x $(WARNINGS)
LDTAR=

else

TC_PATH=$(abspath $(NACL_SDK_ROOT)/toolchain/$(OSNAME)_pnacl)
PREFIX=pnacl-
EXTRA_CXX_FLAGS= -I$(NACL_SDK_ROOT)/include
EXTRA_LD_FLAGS=-L$(NACL_SDK_ROOT)/lib/pnacl/Release -lnacl_io -lppapi -lppapi_cpp
CXX_PKGCONFIG=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags cairo protobuf poppler poppler-cpp)
LD_PKGCONFIG=$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs poppler-cpp poppler cairo fontconfig pixman-1 freetype2 protobuf libpng)
CXX_SUFFIX=clang++
CXXFLAGS := -pthread -std=gnu++11 -stdlib=libc++ $(WARNINGS)
LDTAR=-ltar

endif

CXX := $(TC_PATH)/bin/$(PREFIX)$(CXX_SUFFIX)
PKG_CONFIG_PATH=${TC_PATH}/usr/lib/pkgconfig

FINALIZE := $(TC_PATH)/bin/$(PREFIX)finalize
CXXFLAGS += $(EXTRA_CXX_FLAGS) $(CXX_PKGCONFIG)
LDFLAGS := $(EXTRA_LD_FLAGS) $(LD_PKGCONFIG) -lz -lexpat $(LDTAR) -lpodofo -lcrypto -ljpeg

BCOBJECTS=\
	pdfsketch.bc
PEXE=pdfsketch.pexe
NEXE=pdfsketch_x86_64.nexe
NEXES=\
	pdfsketch_x86_32.nexe \
	pdfsketch_x86_64.nexe \
	pdfsketch_arm_32.nexe

TEST_EXE=test

OBJECTS=\
	view.o \
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

NACL_OBJECTS=\
	pdfsketch.o \
	root_view.o

TEST_OBJECTS=\
	test_main.o

DISTFILES=\
	$(NEXES) \
	system.tar \
	manifest.json \
	index.html \
	index.js \
	main.js \
	style.css \
	logo16.png \
	logo128.png \
	pdfsketch_nexe.nmf

CROSFONTSTARBALL=croscorefonts-1.23.0.tar.gz

# Declare the ALL target first, to make the 'all' target the default build
all: $(PEXE)

clean:
	rm -f $(PEXE) $(OBJECTS) $(NACL_OBJECTS) $(TEST_OBJECTS) $(BCOBJECTS) *.pb.*

$(OBJECTS): document.pb.cc

%.pb.cc: %.proto
	protoc --cpp_out=. document.proto

%.o: %.cc
	$(CXX) -c -o $@ $< -O2 $(CXXFLAGS)

pdfsketch.bc: $(OBJECTS) $(NACL_OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(NACL_OBJECTS) -O2 $(CXXFLAGS) $(LDFLAGS)

$(TEST_EXE): $(OBJECTS) $(TEST_OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(TEST_OBJECTS) -O2 $(CXXFLAGS) $(LDFLAGS)

$(PEXE): $(BCOBJECTS)
	$(FINALIZE) -o $@ $(BCOBJECTS)

$(NEXE): $(PEXE)
	$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-translate --allow-llvm-bitcode-input $< -arch x86-64 -o $@

pdfsketch_x86_32.nexe: $(PEXE)
	$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-translate --allow-llvm-bitcode-input $< -arch x86-32 -o $@

pdfsketch_arm_32.nexe: $(PEXE)
	$(NACL_SDK_ROOT)/toolchain/linux_pnacl/bin/pnacl-translate --allow-llvm-bitcode-input $< -arch arm -o $@

$(CROSFONTSTARBALL):
	wget http://commondatastorage.googleapis.com/chromeos-localmirror/distfiles/croscorefonts-1.23.0.tar.gz

system: $(CROSFONTSTARBALL)
	mkdir -p system/usr/share/fonts
	mkdir -p system/etc
	cp -R $(NACL_SDK_ROOT)/toolchain/linux_pnacl/usr/etc/fonts system/etc/
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

