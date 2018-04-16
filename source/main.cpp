#include <iostream>
#include <fstream>
#include <thread>

#include <SFML/Graphics.hpp>
#include "json.hpp"
using json = nlohmann::json;

#include "bMath.hpp"

#define ASSERT(exp, msg) if (!(exp)) { std::cerr << msg << std::endl; return -1;}

#define WINDOWHEIGHT 720


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
float eyeHeight = 0.5;
float eyeDistance = 0.1;
sf::Color backgroundColor = sf::Color::Black;

std::string outputFolder;
int outputCounter = 0;

std::vector<std::string> frames;

sf::RenderWindow window(sf::VideoMode(1280, 720), "Face Lapse Utility");

sf::Image transformFrameGL(std::string frameName) {
    sf::RenderTexture renderTex;
    renderTex.create(outWidth, outHeight);
    renderTex.clear(backgroundColor);

    sf::Texture tex;
    tex.loadFromFile(frameName);

    sf::Sprite sprite(tex);

    b2d::Vector2 rEye = frameData[frameName]["RightEyePos"];
    b2d::Vector2 lEye = frameData[frameName]["LeftEyePos"];
    float dist = (rEye - lEye).getLength();
    float angle = std::atan2(lEye.y - rEye.y, lEye.x - rEye.x);

    sprite.setOrigin(rEye.x, rEye.y);
    sprite.scale((eyeDistance*outWidth) / dist, -(eyeDistance*outWidth) / dist);
    sprite.rotate(angle * 360 / 6.2831f);
    sprite.move((outWidth-eyeDistance*outWidth)/2, outHeight * eyeHeight);

    renderTex.draw(sprite);

    return renderTex.getTexture().copyToImage();
}

/*sf::Image transformFrame(std::string name) {
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
    float angle = std::atan2(lEye.y - rEye.y, lEye.x - rEye.x);

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
}*/
enum ReturnStatus {
    Saved,
    Canceled
};

void hideWindow() {
    window.setVisible(false);
    sf::Event event;
    while (window.pollEvent(event));
}

ReturnStatus demandEyePositioning() {
    float windowScale = (float) WINDOWHEIGHT / outHeight;
    window.setSize(sf::Vector2u(outWidth * windowScale, WINDOWHEIGHT));
    //sf::RenderWindow window(sf::VideoMode(outWidth * windowScale, WINDOWHEIGHT), "Face Lapse Utility");

    window.setVisible(true);

    sf::Texture tex;
    tex.loadFromImage(transformFrameGL(frames[0])); // hardcode 1st frame 

    sf::Sprite photo;
    photo.setTexture(tex);
    photo.setOrigin(window.getSize().x / 2 / windowScale, (1 - eyeHeight) * window.getSize().y / windowScale);
    photo.setPosition(window.getSize().x / 2, (1 - eyeHeight) * window.getSize().y);
    photo.setScale(windowScale, windowScale);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed : // Close
                return Canceled;
            case sf::Event::KeyPressed : // Escape and Return
                if (event.key.code == sf::Keyboard::Escape) 
                    return Canceled;
                if (event.key.code == sf::Keyboard::Return) {
                    eyeHeight = 1 - photo.getPosition().y / window.getSize().y;
                    eyeDistance *= photo.getScale().x / windowScale;
                    return Saved;
                }
                break;
            case sf::Event::MouseMoved : // Drag
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
                    float s = std::abs((float)event.mouseMove.x / window.getSize().x - 0.5) / eyeDistance * 2;
                    photo.setScale(windowScale*s, windowScale*s);
                    float d = event.mouseMove.y;
                    photo.setPosition(photo.getPosition().x, d);
                }
                break;
            case sf::Event::MouseButtonReleased : { // Click
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
    return Canceled;
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

ReturnStatus fillData(std::vector<std::string> frameSet) {
    int loadNumber = 0;
    int currentNumber = -1;
    float currentScale = 1;

    window.setVisible(true);

    sf::Texture tex;
    sf::Sprite photo;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed :
                return Canceled;
            case sf::Event::KeyPressed :
                if (event.key.code == sf::Keyboard::Escape) {
                    return Canceled;
                }
                if (event.key.code == sf::Keyboard::Return) {
                    return Saved;
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
    return Canceled;
}

bool fileExists(std::string name) {
    std::fstream file(name);
    return file.good();
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
    if (argc <= 1) {
        std::cout << "No arguments provided. Use " << argv[0] << " -? for help" << std::endl;
        return 0;
    }

    window.setVisible(false);
    window.setFramerateLimit(60);

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
                    customColor = true;
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
                    customColor = true;
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

    // Prepare display data
    if (!customColor) {
        auto col = jsonData["display"]["color"];
        backgroundColor = sf::Color(col[0], col[1], col[2], col[3]);
    }
    if (outHeight == 0) {
        outWidth = jsonData["display"]["width"];
        outHeight = jsonData["display"]["height"];
    }
    jsonData["display"] = {{"width", outWidth}, {"height", outHeight}, {"color", {backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a}}}; 
    writeData();

    // Prepare output data
    if (continueOutput) {
        outputFolder = jsonData["output"]["folder"];
        outputCounter = resetCouter ? 0 : jsonData["output"]["counter"].get<int>();
    }
    
    // Prepare frames
    if (hasData) {
        std::vector<std::string> oldFrames;
        int badFrames = 0;
        std::string firstBad = ""; 
        for (auto it = frameData.begin(); it != frameData.end(); ++it){
            std::string name = it.key();
            // checks if file exists
            if (!fileExists(name)) {
                if (firstBad == "") {
                    firstBad = name;
                } else {
                    badFrames++;
                }
            } else {
                // Only add if not given -> dont add twice
                if (std::find(frames.begin(), frames.end(), name) == frames.end()){
                    oldFrames.push_back(it.key());
                }
            }
        }
        if (firstBad != "") {
            std::cerr << firstBad << " and " << badFrames << (badFrames == 1 ? " other frame " : " other frames ") << "weren't found!" << std::endl;
        } else {
            std::cout << oldFrames.size() << " old frame" << (oldFrames.size()==1?"":"s") << " successfully loaded." << std::endl;
        }
        frames.insert(frames.begin(), oldFrames.begin(), oldFrames.end());
    }

    // Eye indentification Phase
    std::vector<std::string> framesToDo = forceAllFrames ? frames : getUncompleteFrames(frames);
    if (framesToDo.size() > 0) {
        ReturnStatus result = fillData(framesToDo);
        hideWindow();
        if (result == Saved){ // if successful (Enter)
            writeData(); // save eye coordinates
        } else { // Cancel Phase 1
            std::cout << "Canceled eye indentification phase." << std::endl;
            return 0;
        }
    }

    // Only continue if all data is availiable
    if (getUncompleteFrames(frames).size() != 0) {
        std::cout << "Uncomplete dataset." << std::endl;
        return 0;
    }

    // Load eyepositioning if availiable
    if (!jsonData["positioning"]["gap"].is_null()) {
        eyeHeight = jsonData["positioning"]["height"];
        eyeDistance = jsonData["positioning"]["gap"];
    }

    // Positioning Phase
    if (forceEyeWindow || jsonData["positioning"]["gap"].is_null()) { // if phase 2 needed
        ReturnStatus result = demandEyePositioning();
        hideWindow();
        if (result == Saved) { // if successful (Enter)
            jsonData["positioning"] = {{"gap", eyeDistance}, {"height", eyeHeight}};
            writeData(); // save eye positioning
        } else { // cancel phase 2
            std::cout << "Canceled positioning phase." << std::endl;
            return 0;
        }
    }

    // Rendering Phase
    if (outputFolder != "") {
        
        sf::Clock clock;
        float secPerFrame = 0;

        bool saveSuccess = true;
        std::thread saverThread([](){});
        //sf::Image frameImage;
        
        int framesProcessed = 0;
        for (; outputCounter < frames.size(); outputCounter++) {
            std::cout << "\r[" << outputCounter+1 << "/" << frames.size() << "]" << std::flush;
            std::string nr = std::to_string(outputCounter);
            std::string fileName = outputFolder + "frame" + std::string(5 - nr.length(), '0') + nr + ".png";

            sf::Image frameImage = transformFrameGL(frames[outputCounter]); // Transform

            saverThread.join(); // wait for last frame to finish saving
            saverThread = std::thread([=](){
                bool success = frameImage.saveToFile(fileName); // save in saverThread
                if (!success)
                    std::cerr << frames[outputCounter] << " couldn't be saved as " << fileName << std::endl; 
            });

            framesProcessed++;
            float timeLeft = clock.getElapsedTime().asSeconds() / framesProcessed * (frames.size() - outputCounter);
            std::cout << " eta: " << std::round(timeLeft) << "s   " << std::flush;
        }
        saverThread.join();

        std::cout << std::endl << "Done, " << frames.size() << " frames processed in " << clock.getElapsedTime().asMilliseconds() << "ms" << std::endl;
        
        jsonData["output"]["folder"] = outputFolder;
        jsonData["output"]["counter"] = outputCounter;
        writeData(); // write output data
    }
}