PS2Paper
========

Manage PostScript 2 paper sizes more easily.


Introduction
------------

PS2Paper is a utility to help manage the different paper sizes configured for the Acorn Level 2 PostScript printer driver supplied with RISC OS. Note that PS2Paper is **not** required when using the Level 3 PostScript driver from John Tytgat and Martin Wuerthner.

Perhaps surprisingly for a printer driver, the Level 2 PostScript driver does not directly use the paper dimensions defined in Printers. Instead, it uses the *name* of the paper to help identify a snippet of PostScript data held in a small file inside !Printers (or the user's choices inside !Boot), and includes this in a suitable place in the PostScript data being output. If this snippet contains the page dimensions, these then get passed on to whatever subsequently reads the PostScript output.

The purpose of PS2Paper is to help keep track of the different papers defined on your system and the corresponding PostScript snippets -- and to help ensure that the sizes given in those snippets actually match the sizes given for the associated paper definitions.


Building
--------

PS2Paper consists of a collection of C and un-tokenised BASIC, which must be assembled using the [SFTools build environment](https://github.com/steve-fryatt). It will be necessary to have suitable Linux system with a working installation of the [GCCSDK](http://www.riscos.info/index.php/GCCSDK) to be able to make use of this.

With a suitable build environment set up, making PS2Paper is a matter of running

	make

from the root folder of the project. This will build everything from source, and assemble a working !PS2Paper application and its associated files within the build folder. If you have access to this folder from RISC OS (either via HostFS, LanManFS, NFS, Sunfish or similar), it will be possible to run it directly once built.

To clean out all of the build files, use

	make clean

To make a release version and package it into Zip files for distribution, use

	make release

This will clean the project and re-build it all, then create a distribution archive (no source), source archive and RiscPkg package in the folder within which the project folder is located. By default the output of `git describe` is used to version the build, but a specific version can be applied by setting the `VERSION` variable -- for example

	make release VERSION=1.23


Licence
-------

PS2Paper is licensed under the EUPL, Version 1.2 only (the "Licence"); you may not use this work except in compliance with the Licence.

You may obtain a copy of the Licence at <http://joinup.ec.europa.eu/software/page/eupl>.

Unless required by applicable law or agreed to in writing, software distributed under the Licence is distributed on an "**as is**"; basis, **without warranties or conditions of any kind**, either express or implied.

See the Licence for the specific language governing permissions and limitations under the Licence.