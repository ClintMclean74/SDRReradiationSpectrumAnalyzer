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
	
	e.g., SDRSpectrumAnalyzerOpenGL.exe -a -s 430000000 -e 470000000