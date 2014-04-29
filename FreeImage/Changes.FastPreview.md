Changes by FastPreview
==

Mostly FreeImages is left intact. However, there are a couple of things that were changes:

* The C++ wrapper was enhanced, among other nits:
	* Namespace and drop `fip` prefix. 
	* Use `bool`.
* Lots of stuff not in use was removed, incl. different language bindings and plugins.
* Dynamic plugin loading was removed.
* Unicode fixes:
	* Source/DeprecationManager/DeprecationMgr.cpp
	* Source/FreeImage.h
	* Source/FreeImage/Plugin.cpp
* Killing the DLL Call conv (messing with defines is error prone) and `DllMain`:
	* Source/FreeImage.h
	* Source/FreeImage/FreeImage.cpp
* Using `std::unique_ptr`:
	* Source/FreeImage/BitmapAccess.cpp
	* Source/FreeImage/Halftoning.cpp
* Pointer validation:
	* Source/FreeImage/BitmapAccess.cpp
* Compiler warnings:
	* Source/FreeImage/PluginBMP.cpp
	* FreeImage/PluginPICT.cpp
* Remove OpenEXR dependecy:
	* Source/FreeImage/PluginTIFF.cpp
* Include path fix:
	* Source/FreeImage/ZLibInterface.cpp
* Make `__Swap*` compile with Intel:
	* Source/Utilities.h

Since the license seems to require marking changes by date, but does not specify the format or resolution, all changes are: 21st century A.D.