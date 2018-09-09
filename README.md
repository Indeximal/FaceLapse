# FaceLapse

Create a timelapse of your daily selfies. Facelapse transforms each frame so your eyes are in the same position.

## Build
Compile main.cpp in c++11. Include json.hpp from https://github.com/nlohmann/json.
Link to sfml-graphics, sfml-window and sfml-system.

## Usage
### Command Line Arguments
`facelapse [-d <datafile>] [-r <outputwidth> <outputheigt> | -R <720p=hd|1080p=fullhd] [-c <r> <g> <b> <a> | -C <black|white|transparent>] [-a] [-e] [-o <outputfolder>] frames...`

`-d <datafile`: some json file to store eye-coordinates, as well as other information, for later use.

`-r <width> <height>` or `-R <Resolution>`: Set the output resolution in pixels. Availiable presets: 
720p = hd, 1080p = fullhd

`-c <r> <g> <b> <a>` or `-C <Color>`: Set the backgroundcolor for the output frames. Availiable presets: 
black, white, transparent

`-o <outputfolder>`: Folder where to put frames in the format: frame00000.png

`-a`: Force all frames to be displayed for eye coordinate editing.

`-e`: Force the window to move the eye target to pop up.

`frames...` A picture in a sfml supported format. (png and jpeg work for sure)

### Eye coordinate editing
A window will show up in which you can determine your selfies eye positions. Use left click to mark your right eye (normally on the left side of the picture) and right click to mark your left eye. 2 markers will indicate your clicks, they should point towards each other.

Press Enter to confirm. Or press Escape or close the window to discard all changes.

### Layout editing
In the next window you can choose the final position of your eye in the output. To do so click or drag your eye. Once you're done press Enter to confirm. Or press Escape or close the window to discard the changes.

### Rendering
If a output file is given the frames will be rendered into the given folder using the format: frame00000.png.

## Example
`facelapse -d datafile.json -o frames images/*`: All images in the folder 'images' will be rendered into the folder 'frames'. 

## Combining images.
This tool only transforms the frames so your eyes are always in the same position. To combine the frames to a video use tools like ffmpeg.
