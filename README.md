The applications are in the SDRReradiation\bin folder with debug and release builds for Windows and Linux.

Start the reradiation spectrum analyzer with the launch.bat files for Windows or the launch.sh files for Linux, set the Linux launch files and SDRReradiation to execute by right clicking the file then "Properties->Permissions->Allow executing the file".

To connect a compatible device using GNU Radio first launch the flowgraph GNURadioDeviceFlowgraph.grc (in the "GNURadioDeviceFlowgraph\" folder) and then the launch files.

Run SDRSpectrumAnalyzerOpenGL.exe for these usage instructions and edit the launch files accordingly.

Usage: SDRSpectrumAnalyzerOpenGL [-a] [-m] [-f] [-s] [-e] [-S]
	-a: Automatically detect reradiated frequency ranges
	-m: Scanning Mode [normal, sessions]
	-f: Required frames for sessions
	-s: Start frequency
	-e: End frequency
	-S: Sound cues for detecting increasing signal strengths	
	-rg: show the averaged previous results graphs using the saved data in the "results\" folder (more data produces brighter lines)
	
	In Program Keys:
	
	1: Toggle data graph
	2: Toggle correlation graph (only for synchronizable devices)
	3: Toggle devices fft graphs' graphics (only for synchronizable devices)
	4: Toggle fft graph graphics (off/2D/3D)
	5: Toggle strength graph graphics (off/2D/3D)
	6: Toggle average fft graph graphics (off/2D/3D)
	7: Toggle average strength graph graphics (off/2D/3D)
	8: Toggle full spectrum range graph graphics (off/2D/3D)
	
	SHIFT 1: Set view to data graph
	SHIFT 2: Set view to correlation graph (only for synchronizable devices)
	SHIFT 3: Toggle view of devices fft graphs or waterfall of FFT (only for synchronizable devices)
	SHIFT 4: Toggle view of fft graph or waterfall of FFT
	SHIFT 5: Set view to strength graph
	SHIFT 6: Toggle view of average fft graph or waterfall of average FFT
	SHIFT 7: Set view to average strength graph
	SHIFT 8: Toggle view of full spectrum range graph or waterfall of spectrum
	SHIFT 9: Toggle the results graphs (Spectrumboard Graph, All Sessions SpectrumRange Graph and the Leaderboard Graph)
	SHIFT S: Set view to current session's strongest reradiated frequencies
	SHIFT L: Set view to leaderBoard for strongest reradiated frequencies of all sessions
	SHIFT C: Set view to all graphs
	
	Center mouse button and mouse moves graphs
	Right mouse button and mouse rotates graphs
	
	Left mouse button and drag on graph selects frequency range and zooms in	
	Right mouse button on graph zooms out
	
	Mouse wheel zooms in/out graphics
	
	n: Record near series data (for non-automated detection)
	f: Record far series data (for non-automated detection)

	l: Toggle Graph Labels
	
	SHIFT N: Clear near series data
	SHIFT F: Clear far series data

	,: Cycle transitions back
	.: Cycle transitions forward
	
	R: Check Reradiated Frequencies. Sets the SDR to the reradiated frequency so that the effect on strength levels of moving nearer and further from the antenna can be determined
	
	<: Cycle reradiated frequencies back
	>: Cycle reradiated frequencies forward
	
	e.g., SDRSpectrumAnalyzerOpenGL.exe -a -s 430000000 -e 470000000



User Guide
----------

Here's the user guide for the slower, original code system, for ideas on how to use this to detect your resonant, reradiated frequency ranges:

https://drive.google.com/file/d/1GixkhAa6bBUuEGLTBrWXw7OFkZQBzuxV/view?usp=share_link

The original code system is here:

https://github.com/ClintMclean74/SDRSpectrumAnalyzer



Installation Instructions
-------------------------

Skywave linux already has drivers and other applications installed for sdrs, although if using Windows or other Linux versions and if the rtl sdr isn't already working with other applications then you need to also install the rtl sdr dongle according to this guide:

http://www.rtl-sdr.com/tag/zadig/

If you need to buy the dongle I would also recommend

www.rtl-sdr.com


Linux Installation for building code (tested on Ubuntu)
-------------------------------------------------------

Prerequisites:
--------------
Install codeblocks:

sudo add-apt-repository universe

sudo apt update

sudo apt install codeblocks

If g++ command not found error then install gcc compiler:

sudo apt-get update

sudo apt-get upgrade

sudo apt-get install build-essential

Install OpenGL development code, GNU Radio, sdr drivers...etc:

sudo apt-get install libgl-dev

sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev

sudo apt-get install libxss-dev

sudo apt-get install gnuradio

sudo apt install gr-osmosdr

sudo apt-get install libusb-1.0-0-dev


Building the Code Instructions:
---------------------------

First make sure that SDRReradiation is the activated project for building, should be in bold (if not right-click SDRReradiation in the Project Manager and select "Activate project"). Then check that Build->Select Target->Debug_Linux64 or Build->Select Target->Release_Linux64 is set. If an error occurs for the Release build then try the Debug Build.

When you first run the program from codeblocks (if you don't rebuild it), it could give a permission denied error, because it's using the previously already generated file in bin/Release_Linux64 or Debug_linux64. These files need to have their permissions changed to allow executing (right-click file, properties->permissions->Allow executing file as program), although when you rebuild them these permissions should be automatically set. If you want to run these files from the command line or file manager then you need to also set these permissions for the launch.sh files (edit these files for frequency range and other settings).



Additional Information
----------------------

You should also read this research paper/thesis for more details on how to use the sdr spectrum analyzer and how I detected reradiated frequency ranges that could be used to cause biological effects:

https://drive.google.com/file/d/1Kyl0xJq4ndh9y6mQPjucHfqSpRKAETyK/view?usp=share_link


The code uses librtlsdr library code to transfer data from the device:

https://github.com/steve-m/librtlsdr

The description for this project is here:

http://osmocom.org/projects/rtl-sdr/wiki

The libusb library code is also used to communicate with the device using USB:

https://github.com/libusb/libusb
