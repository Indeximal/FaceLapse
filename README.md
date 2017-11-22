# FaceLapse

Create a timelapse of your daily selfies. Facelapse transforms each frame so your eyes are in the same position.

## Build
Compile main.cpp in c++11. Include json.hpp from https://github.com/nlohmann/json and bMath.hpp from https://github.com/Indeximal/sfmlTest.
Link to sfml-graphics, sfml-window and sfml-system.

## Usage
### Command Line Arguments
`facelapse [-d <datafile>] [-r <outputwidth> <outputheigt> | -R <720p=hd|1080p=fullhd] [-c <r> <g> <b> <a> | -C <black|white|transparent>] [-a] [-e] [-o <outputfolder> | -O] frames...`

`-d <datafile`: some json file to store eye-coordinates, as well as other information, for later use.

`-r <width> <height>`: Output resolution in pixels.

`-R <Resolution>`: Use 720p = hd or 1080p = fullhd as the output resolution.

`-c <r> <g> <b> <a>`: Backgroundcolor in the output frames.

`-C <Color>`: Use black, white or transparent as a backgroundcolor.

`-o <outputfolder>`: Folder where to put frames in the format: frame00000.png

`-O`: Output to the same folder as last time, starting from the previous last frame.

`-a`: Force all frames to be displayed for eye coordinate editing.

`-e`: Force the window to move the eye target to pop up.

`frame` A picture in a sfml supported format. (png and jpeg work for sure)
