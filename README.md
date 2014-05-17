PDFSketch
=========

PDFSketch is currently alpha quality software: it is not feature complete and will probably crash.
However, if you use it carefully, it can function.

PDFSketch is a PDF annotator/editor. It can be used to mark up a PDF file with text, circles,
freehand squiggles, and checkmarks.

To install the developer preview, visit the Chrome Web Store and install it:

The top row of buttons is actions to perform. The next row of buttons it the current tool you
have selected.

License
-------

This project is dual license BSD and GPL v2. Keep in mind that Poppler is GPL v2, so this
codebase is effectively GPL v2 unless you get a non-GPL license from the Poppler team.

Building
--------

The core of the app is written in C++, using poppler to render 

If on Chome OS device, get crouton going:

    sudo sh -e ~/Downloads/crouton -t xfce
    sudo enter-chroot

Make sure you have the right tools:

    sudo apt-get install git make lib32stdc++6 g++ zip

Install NaCl SDK from https://developer.chrome.com/native-client/sdk/download . use pepper_33

    unzip nacl_sdk.zip
    cd nacl_sdk
    ./naclsdk install pepper_33

Check out modified naclports from https://github.com/adlr/naclports (use branch ‘pdfsketch’)

set env variables: (you probably want to make a script so you don’t need to redo these each time).
Fix the paths to match your environment.

    export NACL_SDK_ROOT=$(readlink -f ~/Downloads/nacl_sdk/pepper_33)
    export NACL_ARCH=pnacl

Build PDFSketch dependencies:

    cd naclports
    make cairo
    make poppler
    make protobuf
    make libtar

build protocol compiler locally:

    cd somewhere
    tar xzvf .../path/to/naclports/out/tarballs/protobuf*.tar.gz
    cd protobuf*
    ./configure && make
    sudo make install

Check out PDFSketch and build it:

    # Note, I had to modify LD_LIBRARY_PATH to make protoc find its required libraries
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib make dist.zip

You can then navigate Chrome to chrome://extensions and load this as an unpacked extension.
