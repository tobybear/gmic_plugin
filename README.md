G'MIC as a plugin for Adobe hosts (After Effects and Premiere Pro), OpenFX (Nuke, Natron, etc.) and frei0r (Kdenlive, Shotcut, etc.)

Licenses:
This project is 'dual-licensed', you have to choose one of the two licenses below to apply.
CeCILL-C: The CeCILL-C license is close to the GNU LGPL ( http://cecill.info/licences/Licence_CeCILL-C_V1-en.html )
or
CeCILL v2.0: The CeCILL license is compatible with the GNU GPL. ( http://cecill.info/licences/Licence_CeCILL_V2-en.html =

WARNING
This is a very early test release, use at your own risk! Expect crashes and weird behaviour.
Feel free to report any errors you encounter, but be aware that this is pretty much still a proof-of-concept prototype interface.

WHAT IS IT?
This is an attempt to make (part of) the functionality of the G'MIC open-source framework for image processing (http://www.gmic.eu) available in video editing hosts.
Currently supported hosts are:
  Adobe After Effects (CS6 or higher)
  Adobe Premiere Pro (CS6 or higher)
  OpenFX hosts like The Foundry's Nuke, Natron, Sony Vegas, etc.
  frei0r hosts like Kdenlive or Shotcut
- early beta, many things might not work as intended and plugin may crash the host!

Quick installation of the binaries from the "bin" folder:

1. General (both Adobe + OFX):
- Put the file from the "lib" subfolder somewhere in your path.

2. Adobe plugins (After Effects and Premiere Pro):
- Put the files from the "adobe" subfolder somewere in "c:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\".
- Run the file "make_plugins.bat" in that folder. It will create all (1000+!) Adobe plugins.

3. OFX plugins:
- Put the files from the "ofx" subfolder somewere in "c:\Program Files\Common Files\OFX\Plugins\".

4. frei0r plugin
- Put the dll and xml file in the appropriate folders of your host application.
  For Kdenlive, the dll goes to ".\lib\frei0r-1\" and the xml goes to ".\bin\data\kdenlive\effects\".


COMPILATION of the sources for Windows systems:
1. Compiling the G'MIC static lib (not needed if you use the latest libcgmicstatic.dll provided in the official G'MIC release)
1a) G'MIC main sources
Go to http://gmic.eu and download the latest sources from there.
Extract the archive and copy the contents of the contents of the "src" subfolder to ".\ThirdParty\GMIC\src"
1b) G'MIC community sources
Copy the 3 source files from the "libcgmic" folder in this repository also to ".\ThirdParty\GMIC\src"
1c) G'MIC static C lib
Open your MinGW command line on Windows, change into the ".\ThirdParty\GMIC\src" folder and compile
the static library with this command:
"make libcstatic"
It should take several minutes but should produce no errors and spit out the following files in the same folder:
- use_libcgmic_static.exe
- libcgmicstatic.dll
You can run the exe to see if it actually works, it should display an RGB polaroid on screen.

2. Compiling the G'MIC OpenFX plugin
2a) OpenFX sources
You need to get the OpenFX API files (both the include and the examples archives), which are available here:
https://sourceforge.net/projects/openfx/files/
At the time of writing, the latest versions were "openfx-examples-1.4.tar.gz" and "openfx-include-1.4.tar.gz".
Extract these to files into the folder ".\ThirdParty\OpenFX\", so that the contents of the two archives end up in the "Examples" and "include" subfolders below that.
2b) G'MIC OFX plugin
Go into the folder ".\src\GMIC_OFX" and open the "GMIC_OFX.sln" solution for Visual Studio.
The solution is for Visual Studio 2012 and has only been tested with that, but should work in newer (and possibly older) versions of Visual Studio as well.
Compile the plugin. It should create a proper OFX bundle folder structure with the file in it and also a copy of it in "C:\Program Files\Common\OFX"

3. Compiling the G'MIC After Effects plugins for Windows systems
3a) Adobe After Effects SDK sources
You need to get the Adobe After Effects SDK from here: http://www.adobe.com/devnet/aftereffects.html
The latest version as of this writing is CC 2017, although the sources should be backwards compatible even down to the CS6 SDK.
Download and extract the archive, then copy the subfolder "Examples" to ".\ThirdParty\Adobe_AfterEffects_SDK", so that all its contents end up in ".\ThirdParty\Adobe_AfterEffects_SDK\Examples\...".
3b) G'MIC AE plugin
Go into the folder ".\src\GMIC_AE\Win" and open the "gmic_ae.sln" solution for Visual Studio.
The solution is for Visual Studio 2012 and has only been tested with that, but should work in newer (and possibly older) versions of Visual Studio as well.
3c) Create plugins from the template
Go to ".\src\GMIC_AE\Win\Release". There should be two files, "gmic_ae.aex" and "gmic_ae_tool.exe".
Copy the file ".\ThirdParty\GMIC\src\gmic_stdlib.gmic" to that folder as well, then on the command line, enter the following: "gmic_ae_tool xb gmic_stdlib.gmic"
This should create a whole lot of .aex/.gmic files that should be usable from within After Effects.

INSTALLATION of the binaries
- Copy the folder "GMIC_OFX.ofx.bundle" to your preferred OpenFX plugin folder. The default is usually "c:\Program Files\Common Files\OFX\Plugins\"
- Copy the generated binary .aex files to your preferred Adobe plugin folder. The default is usually "c:\Program Files\Adobe\Common\Plug-ins\[VERSION]\MediaCore\"
- Copy the file "libcgmicstatic.dll" to any folder that is contained in your path (enter "echo %PATH%" on the command line to see the possible folders on your system)

CONTACT
Tobias Fleischer / reduxFX Productions
web: www.reduxfx.com
email: info@reduxfx.com
