FastPreview
===
A PicaView replacement.

Manual
--
See [Manual.html](manual.html).

Build Instructions
--
* Get MSVC12 (2013) - Not sure if the Express version suffices.
* Optionally get the Intel C++ compiler 14
* Optionally get [Microsoft Visual Studio Installer Projects Preview](http://visualstudiogallery.msdn.microsoft.com/9abe329c-9bba-44a1-be59-0fbf6151054d)
* Open the solution.
* Select one of the following configurations:
	* `Debug` / `Win32` or `x64`
	* `Release` / `Win32` or `x64`
	* `WOW` / `Win32` (`fpext_WOW.dll` only)
* Build!

Alternatively batch build:

* `fastpreview`: `Release Win32` + `Release x64`
* `fpext.dll`: `WOW Win32`

After batch-building, feel free to build the `Setup` configuration to create MSIs. Also see `Setup/createsfx.py` to create the self-extracting installers.

License
--
Original code is licensed under the MIT LICENSE. Also see the `License.*.txt` files regarding licensing of third-party stuff.

Note
--
Please note that this stuff is essentially a mix of old "*let's write one of my first 
C++ applications*" cruft and some more modern stuff.
Also note that most parts of the code aren't mine; Most of FastPreview is just glue code to piece various bits together.