# FaceLapse

Create a timelapse of your daily selfies. Facelapse transforms each frame so your eyes are in the same position.

## Build
Compile main.cpp in c++11. Include json.hpp from https://github.com/nlohmann/json and bMath.hpp from https://github.com/Indeximal/sfmlTest.
Link to sfml-graphics, sfml-window and sfml-system.

## Usage
`facelapse [-d <datafile>] [-r <outputwidth> <outputheigt> | -R <720p=hd|1080p=fullhd] -o <outputfolder> frame0 frame1 ... frameN`

or: 

`facelapse -c -d <datafile> [-o <outputfolder>] [newFrame1 newFrame2 ... newFrameN]`

### Arguments

`-d <datafile`: some json file to store eye-coordinates, as well as other information, for later use.

`-r <width> <height>`: Output resolution in pixels.

`-R <Resolution>`: Use 720p = hd or 1080p = fullhd as the output resolution.

`-o <outputfolder>`: Folder where to put frames in the format: frame00000.png

`-c`: Continue like last time, need a json-datafila to work. 
Will include all previous frames saved in the datafile as well as any other given frames.
The resolution, the output folder and the final eye position will be the same as last time.
If a outputfolder is given it will instead output to this folder and start with frame00000.png,
otherwise it will continue with the counting.

`frame` A picture in a sfml supported format. (png and jpeg work for sure)
