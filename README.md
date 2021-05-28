The application is in the build\ folder.

Start the reradiation spectrum analyzer with launch.bat.

To connect a compatible device using GNU Radio first launch the flowgraph GNURadioDeviceFlowgraph.grc (in the "GNURadioDeviceFlowgraph\" folder) then the launch.bat files or SDRSpectrumAnalyzerOpenGL.exe

Run SDRSpectrumAnalyzerOpenGL.exe for these usage instructions and edit launch.bat accordingly.

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
	
	SHIFT and mouse moves graphs
	CTRL and mouse rotates graphs
	
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

https://drive.google.com/open?id=1Sc6_Tbxux-O5aAFY-gAXCkrgMpNiRkjv&fbclid=IwAR2ktx2dDmgUX8CeoNUnBmmzYwdr6OMHwQbpX10gseqPXcWQTDCFnjCIk3k

The original code system is here:

https://github.com/ClintMclean74/SDRSpectrumAnalyzer



Installation Instructions
-------------------------

You need to install the rtl sdr dongle according to this guide:

http://www.rtl-sdr.com/tag/zadig/



If you need to buy the dongle I would also recommend

www.rtl-sdr.com



Additional Information
----------------------

You should also read this research paper/thesis for more details on how to use the rtl sdr spectrum analyzer and how I detected the signal that I mentioned:

https://drive.google.com/open?id=13wX8O4iqdy88WMnNP0QvVjPmfYBROFxf

 
The code uses the librtlsdr library to transfer data from the device:

https://github.com/steve-m/librtlsdr

The description for this project is here:

http://osmocom.org/projects/rtl-sdr/wiki

The libusb library code is also used to communicate with the device using USB:

https://github.com/libusb/libusb

So the code for libusb-1.0.dll and rtlsdr.dll is available from there if you require it.

