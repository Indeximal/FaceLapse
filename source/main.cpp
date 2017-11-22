#include <iostream>
#include <fstream>

#include <SFML/Graphics.hpp>
#include "json.hpp"

#include "bMath.hpp"

#define ASSERT(exp, msg) if (!(exp)) {\
    std::cerr << msg << std::endl;\
    return -1;}


#define WINDOWHEIGHT 720

using json = nlohmann::json;

namespace b2d {
    void to_json(json& j, const b2d::Vector2& v) {
        j = json{v.x, v.y};
    }

    void from_json(const json& j, b2d::Vector2& v) {
        v.x = j.at(0).get<float>();
        v.y = j.at(1).get<float>();
    }
}

std::string dataFileName;

json jsonData = R"({
    "display": {
        "width": 1920,
        "height": 1080,
        "color": [0, 0, 0, 255]
    },
    "positioning": {
        "gap": null,
        "height": null
    },
    "output": {
        "folder": null,
        "counter": 0
    },
    "frames": {}
    })"_json;
json frameData = jsonData["frames"];

int outHeight = 0;
int outWidth = 0;
float eyeHeight = 0;
float eyeDistance = 0;
sf::Color backgroundColor = sf::Color::Black;

std::string outputFolder;
int outputCounter = 0;

std::vector<std::string> frames;

sf::Image transformFrame(std::string name) {
    // sf::Clock clock;
    sf::Image ret;
    ret.create(outWidth, outHeight);
    sf::Image orig;
    if (!orig.loadFromFile(name)) {
        std::cerr << name << "couldn't be loaded." << std::endl;
        return ret;
    }
    b2d::Vector2 rEye = frameData[name]["RightEyePos"];
    b2d::Vector2 lEye = frameData[name]["LeftEyePos"];
    float dist = (rEye - lEye).getLength();
    float angle = std::atan((rEye.y - lEye.y) / (rEye.x - lEye.x));

    b2d::Matrix3 t1 = b2d::Matrix3::translate2d(-(outWidth-eyeDistance*outWidth)/2, -outHeight*(1-eyeHeight));
    b2d::Matrix3 s0 = b2d::Matrix3::scale2d(dist / (eyeDistance*outWidth));
    b2d::Matrix3 r0 = b2d::Matrix3::rotate2d(angle);
    b2d::Matrix3 t2 = b2d::Matrix3::translate2d(rEye.x, rEye.y);
    b2d::Matrix3 invMat = t2 * r0 * s0 * t1;

    for (int x = 0; x < outWidth; x++){
        for (int y = 0; y < outHeight; y++){
            b2d::Vector2 pos(x, y);
            b2d::Vector2 pxl = invMat * pos;
            sf::Color col = backgroundColor;
            if (pxl.x < orig.getSize().x && pxl.x >= 0 && pxl.y < orig.getSize().y && pxl.y >= 0) {
                col = orig.getPixel((int)pxl.x, (int)pxl.y);
            }
            ret.setPixel(x, y, col);
        }
    }
    // std::cout << clock.getElapsedTime().asMilliseconds() << std::endl;
    return ret;
}

bool demandEyePositioning() {
    float windowScale = (float) WINDOWHEIGHT / outHeight;
    sf::RenderWindow window(sf::VideoMode(outWidth * windowScale, WINDOWHEIGHT), "Face Lapse Utility");
    window.setFramerateLimit(60);

    sf::Texture tex;
    tex.loadFromImage(transformFrame(frames[0])); // hardcode 1st frame 

    sf::Sprite photo;
    photo.setTexture(tex);
    photo.setOrigin(window.getSize().x / 2 / windowScale, (1 - eyeHeight) * window.getSize().y / windowScale);
    // std::cout << (window.getSize().x / 2) << "|" << ((1 - eyeHeight) * window.getSize().y) << std::endl;
    photo.setPosition(window.getSize().x / 2, (1 - eyeHeight) * window.getSize().y);
    photo.setScale(windowScale, windowScale);

    bool save = false;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed :
                window.close();
                break;
            case sf::Event::KeyPressed :
                if (event.key.code == sf::Keyboard::Escape) window.close();
                if (event.key.code == sf::Keyboard::Return) {
                    save = true;
                    window.close();
                }
                break;
            
            case sf::Event::MouseMoved :
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
                    float s = std::abs((float)event.mouseMove.x / window.getSize().x - 0.5) / eyeDistance * 2;
                    photo.setScale(windowScale*s, windowScale*s);
                    float d = event.mouseMove.y;
                    photo.setPosition(photo.getPosition().x, d);
                }
                break;
            case sf::Event::MouseButtonReleased : {
                if (event.mouseButton.button != sf::Mouse::Button::Left){
                    break;
                }
                float s = std::abs((float)event.mouseButton.x / window.getSize().x - 0.5) / eyeDistance * 2;
                photo.setScale(windowScale*s, windowScale*s);
                float d = event.mouseButton.y;
                photo.setPosition(photo.getPosition().x, d);
                break;
            }
            default:
                break;
            }
        }

        window.clear(backgroundColor);
        window.draw(photo);

        window.display();
    }
    if (save) {
        eyeHeight = 1 - photo.getPosition().y / window.getSize().y;
        eyeDistance *= photo.getScale().x / windowScale;
    }
    return save;
}

std::vector<std::string> getUncompleteFrames(std::vector<std::string> frameSet){
    std::vector<std::string> ret;
    for (auto str : frameSet) {
        if (frameData.count(str) == 0) {
            ret.push_back(str);
        } else {
            if (frameData[str].count("RightEyePos") == 0){
                ret.push_back(str);
                continue;
            } else if (!frameData[str]["RightEyePos"].is_array() || frameData[str]["RightEyePos"].size() != 2){
                ret.push_back(str);
                continue;
            }
            if (frameData[str].count("LeftEyePos") == 0){
                ret.push_back(str);
                continue;
            } else if (!frameData[str]["LeftEyePos"].is_array() || frameData[str]["LeftEyePos"].size() != 2){
                ret.push_back(str);
                continue;
            }
        }
    }
    return ret;
}

bool fillData(std::vector<std::string> frameSet) {
    int loadNumber = 0;
    int currentNumber = -1;
    float currentScale = 1;

    sf::RenderWindow window(sf::VideoMode(1280, 720), "Face Lapse Utility");
    window.setFramerateLimit(60);

    sf::Texture tex;
    sf::Sprite photo;

    bool save = false;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed :
                window.close();
                break;
            case sf::Event::KeyPressed :
                if (event.key.code == sf::Keyboard::Escape) window.close();
                if (event.key.code == sf::Keyboard::Return) {
                    save = true;
                    window.close();
                }
                if (event.key.code == sf::Keyboard::Right) {

                    loadNumber = std::min(currentNumber + 1, (int)frameSet.size() - 1);
                }

                if (event.key.code == sf::Keyboard::Left) {
                    loadNumber = std::max(currentNumber - 1, 0);
                }
                break;
            case sf::Event::MouseButtonPressed : {
                json pos = {(int) (event.mouseButton.x / currentScale), (int) (event.mouseButton.y / currentScale)};
                if (event.mouseButton.button == sf::Mouse::Right) {
                    if (frameData[frameSet[currentNumber]].is_object()){
                        frameData.at(frameSet.at(currentNumber))["LeftEyePos"] = pos;
                    } else {
                        frameData[frameSet[currentNumber]] = {{"LeftEyePos", pos}};
                    }
                } else if (event.mouseButton.button == sf::Mouse::Left) {
                    if (frameData[frameSet[currentNumber]].is_object()){
                        frameData.at(frameSet.at(currentNumber))["RightEyePos"] = pos;
                    } else {
                        frameData[frameSet[currentNumber]] = {{"RightEyePos", pos}};
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        if (loadNumber >= 0) {
            if (!tex.loadFromFile(frameSet.at(loadNumber))) {
                std::cout << "error" << std::endl;
                loadNumber = -1;
            } else {
                photo.setTexture(tex);
                float scaleX = (float)window.getSize().x / tex.getSize().x; // goal: > 1
                float scaleY = (float)window.getSize().y / tex.getSize().y;
                currentScale = std::min(scaleX, scaleY);
                photo.setScale(currentScale, currentScale);
                currentNumber = loadNumber;
                loadNumber = -1;
            }
        }

        window.clear(sf::Color::White);
        window.draw(photo);

        if (frameData[frameSet[currentNumber]].is_object()) {
            if (frameData[frameSet[currentNumber]].count("RightEyePos")) {
                auto coords = frameData[frameSet[currentNumber]]["RightEyePos"];
                sf::Vector2f pos = sf::Vector2f(coords[0], coords[1]) * currentScale;
                sf::Vertex lines[] =
                {
                    sf::Vertex(pos, sf::Color::Blue),
                    sf::Vertex(pos + sf::Vector2f(-20, -20), sf::Color::Blue),
                    sf::Vertex(pos, sf::Color::Blue),
                    sf::Vertex(pos + sf::Vector2f(-20, 20), sf::Color::Blue)
                };

                window.draw(lines, 4, sf::Lines);
            }
            if (frameData[frameSet[currentNumber]].count("LeftEyePos")) {
                auto coords = frameData[frameSet[currentNumber]]["LeftEyePos"];
                sf::Vector2f pos = sf::Vector2f(coords[0], coords[1]) * currentScale;
                sf::Vertex lines[] =
                {
                    sf::Vertex(pos, sf::Color::Red),
                    sf::Vertex(pos + sf::Vector2f(20, -20), sf::Color::Red),
                    sf::Vertex(pos, sf::Color::Red),
                    sf::Vertex(pos + sf::Vector2f(20, 20), sf::Color::Red)
                };

                window.draw(lines, 4, sf::Lines);
            }
        }

        window.display();
    }
    return save;
}

void writeData(){
    if (dataFileName != "") {
        jsonData["frames"] = frameData;   
        std::ofstream file(dataFileName);
        if (file.good()){
            file << jsonData;
        }
        file.close();
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "No arguments provided. Use " << argv[0] << " -? for help" << std::endl;
    }

    bool forceEyeWindow = false;
    bool forceAllFrames = false;
    bool continueOutput = false;
    bool resetCouter = false;
    bool customColor = false;
    bool hasData = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-'){
            switch (argv[i][1]){
                case 'e':
                    forceEyeWindow = true;
                    resetCouter = true;
                    break;
                case 'a':
                    forceAllFrames = true;
                    resetCouter = true;
                    break;
                case 'd': // datafile
                    {
                    ASSERT(argc > i + 1, "-d needs one argument. Usage: -d <datafile>")
                    dataFileName = argv[++i];
                    std::ifstream file(dataFileName);
                    if (file.good()){
                        file >> jsonData;
                        frameData = jsonData["frames"];
                        hasData = true;
                    }
                    file.close();
                    break;
                    }
                case 'o': // outputfolder
                    ASSERT(argc > i + 1, "-o needs one argument. Usage: -o <outputfolder>")
                    outputFolder = argv[++i];
                    if (outputFolder[outputFolder.length() - 1] != '/') {
                        outputFolder += "/";
                    }
                    break;
                case 'O':
                    continueOutput = true;
                    break;
                case 'r': // custom resolution
                    ASSERT(argc > i + 2, "-r needs two arguments. Usage: -r <outputwidth> <outputheigt>")
                    outWidth = std::stoi(argv[++i]);
                    outHeight = std::stoi(argv[++i]);
                    resetCouter = true;
                    break;
                case 'R': { // defalt resolutions
                    ASSERT(argc > i + 1, "-R needs one argument. Usage: -R <720p=hd|1080p=fullhd")
                    std::string res(argv[++i]);
                    if (res == "720p" || res == "hd"){
                        outWidth = 1280;
                        outHeight = 720;
                    } else if (res == "1080p" || res == "fullhd"){
                        outWidth = 1920;
                        outHeight = 1080;
                    } else {
                        std::cerr << res << " is no supported resolution. Use -r w h for a custom resolution" << std::endl;
                        break;
                    }
                    resetCouter = true;
                    break;
                    }
                case 'C': {
                    ASSERT(argc > i + 1, "-C needs one argument. Usage: -C <black|white|transparent>")
                    std::string res(argv[++i]);
                    if (res == "black"){
                        backgroundColor = sf::Color::Black;
                    } else if (res == "white") {
                        backgroundColor = sf::Color::White;
                    } else if (res == "transparent"){
                        backgroundColor = sf::Color::Transparent;
                    } else {
                        std::cerr << res << " is no supported color. Use -c r g b a for a custom color" << std::endl;
                        break;
                    }
                    resetCouter = true;
                    break;
                    }
                case 'c': {
                    ASSERT(argc > i + 4, "-c needs four arguments. Usage: -c r g b a")
                    int r = std::stoi(argv[++i]);
                    int g = std::stoi(argv[++i]);
                    int b = std::stoi(argv[++i]);
                    int a = std::stoi(argv[++i]);
                    backgroundColor = sf::Color(r, g, b, a);
                    resetCouter = true;
                    break;
                    }
                default:
                    std::cerr << "unknown option -" << argv[i][1] << std::endl;
                case '?': // help
                    std::cout << "Usage: " << argv[0] << " [-d <file>] [-r <w> <h> | -R <720p=hd|1080p=fullhd>] [-c <r> <g> <b> <a> | -C <black|white|transparent>] [-e] [-a] [-o <folder> | -O] frame0 frame1 ... frameN" << std::endl;
                    std::cout << "Go to https://github.com/Indeximal/FaceLapse for further information" << std::endl;
                    return 0;
            }
        } else {
            frames.push_back(std::string(argv[i]));
        }
    }

    if (!customColor) {
        auto col = jsonData["display"]["color"];
        backgroundColor = sf::Color(col[0], col[1], col[2], col[3]);
    }
    if (outHeight == 0) {
        outWidth = jsonData["display"]["width"];
        outHeight = jsonData["display"]["height"];
    }

    if (hasData) {
        std::cout << "Frames are sorted alphabetically, so they might not be in the original order!" << std::endl;

        std::vector<std::string> oldFrames;
        int badFrames = 0;
        std::string firstBad = ""; 
        for (auto it = frameData.begin(); it != frameData.end(); ++it){
            std::string name = it.key();
            std::fstream frameFile(name);
            if (!frameFile.good()) {
                if (firstBad == "") {
                    firstBad = name;
                } else {
                    badFrames++;
                }
            } else {
                oldFrames.push_back(it.key());
            }
        }
        if (firstBad != "") {
            std::cerr << firstBad << " and " << badFrames << (badFrames == 1 ? " other frame " : " other frames ") << "weren't found!" << std::endl;
        }
        frames.insert(frames.begin(), oldFrames.begin(), oldFrames.end());
    }

    if (continueOutput) {
        outputFolder = jsonData["output"]["folder"];
        outputCounter = resetCouter ? 0 : jsonData["output"]["counter"].get<int>();
    }

    jsonData["display"] = {{"width", outWidth}, {"height", outHeight}, {"color", {backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a}}}; 


    std::vector<std::string> framesToDo = forceAllFrames ? frames : getUncompleteFrames(frames);
    if (framesToDo.size() > 0) {
        if (fillData(framesToDo)){
            writeData(); // early write
            if (getUncompleteFrames(frames).size() != 0) {
                std::cout << "Uncomplete dataset." << std::endl;
                return 0;
            }
        } else {
            std::cout << "Canceled eye indentification phase." << std::endl;
            return 0;
        }
    }

    if (forceEyeWindow || jsonData["positioning"]["gap"].is_null()) {
        if (demandEyePositioning()) {
            jsonData["positioning"] = {{"gap", eyeDistance}, {"height", eyeHeight}};
        } else {
            std::cout << "Canceled positioning phase." << std::endl;
            return 0;
        }
    } else {
        eyeHeight = jsonData["positioning"]["height"];
        eyeDistance = jsonData["positioning"]["gap"];
    }

    if (outputFolder != "") {
        jsonData["output"]["folder"] = outputFolder;
        
        sf::Clock clock;
        int secPerFrame = 0;
        for (int i = outputCounter; i < frames.size(); i++) {
            std::cout << "\r[" << i+1 << "/" << frames.size() << "]" << std::flush;
            std::string nr = std::to_string(outputCounter++);
            std::string name = outputFolder + "frame" + std::string(5 - nr.length(), '0') + nr + ".png";

            sf::Clock c2;

            sf::Image frame = transformFrame(frames[i]);
            if (!frame.saveToFile(name)) {
                std::cerr << frames[i] << " couldn't be saved as " << name << std::endl; 
            }

            secPerFrame = c2.restart().asSeconds();
            std::cout << "~" << (frames.size() - outputCounter)*secPerFrame << " sec remaining" << std::flush;
        }
        std::cout << std::endl << "Done, " << frames.size() << " frames processed in " << clock.getElapsedTime().asMilliseconds() << "ms" << std::endl;
    }
    jsonData["output"]["counter"] = outputCounter;
    writeData();
}