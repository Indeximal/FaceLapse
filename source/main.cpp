#include <iostream>
#include <fstream>
#include <thread>
#include <cmath>

#include <SFML/Graphics.hpp>
#include "json.hpp"
using json = nlohmann::json;

#define ASSERT(exp, msg) if (!(exp)) { std::cerr << msg << std::endl; return -1;}
#define WINDOWHEIGHT 720

namespace facelapse {
    namespace jsonKeys {
        namespace settings {
            const std::string width = "output_width";
            const std::string height = "output_heigth";
            const std::string bgColor = "background_color";
            const std::string eyeHeight = "eye_height";
            const std::string eyeSpacing = "eye_spacing";
        }
        const std::string outputsettings = "output_settings";
        const std::string coordinates = "coordinate_pairs";
        const std::string version = "version";
    }

    struct CoordinatePair {
        float rX, rY;
        float lX, lY;

        CoordinatePair(float rx = -1, float ry = -1, float lx = -1, float ly = -1) 
            : rX(rx), rY(ry), lX(lx), lY(ly)
        { }

        float dist() {
            return std::sqrt((rX - lX) * (rX - lX) + (rY - lY) * (rY - lY));
        }
        bool isComplete() {
            return rX > 0 && rY > 0 && lX > 0 && lY > 0;
        }
    };
    void to_json(json& j, const CoordinatePair& coords) {
        j = json { coords.rX, coords.rY, coords.lX, coords.lY };
    }
    void from_json(const json& j, CoordinatePair& coords) {
        if (j.is_null()) {
            coords = CoordinatePair();
        } else {
            coords = CoordinatePair(j[0], j[1], j[2], j[3]);
        }
    }

    struct OutputSettings {
        int width;
        int height;
        sf::Color bgColor;
        float eyeHeight;
        float eyeSpacing;

        OutputSettings(int w = 1920, int h = 1080, sf::Color col = sf::Color::Black, float eH = 0.5, float eS = 0.1)
            : width(w), height(h), bgColor(col), eyeHeight(eH), eyeSpacing(eS)
        { }
    };
    void to_json(json& j, const OutputSettings& obj) {
        j = json { {jsonKeys::settings::width, obj.width}, {jsonKeys::settings::height, obj.height}, 
        {jsonKeys::settings::bgColor, {obj.bgColor.r, obj.bgColor.g, obj.bgColor.b, obj.bgColor.a }},
        {jsonKeys::settings::eyeHeight, obj.eyeHeight}, {jsonKeys::settings::eyeSpacing, obj.eyeSpacing} };
    }
    void from_json(const json& j, OutputSettings& obj) {
        obj.width = j.at(jsonKeys::settings::width).get<int>();
        obj.height = j.at(jsonKeys::settings::height).get<int>();
        auto cArr = j.at(jsonKeys::settings::bgColor);
        obj.bgColor = sf::Color(cArr.at(0), cArr.at(1), cArr.at(2), cArr.at(3));
        obj.eyeHeight = j.at(jsonKeys::settings::eyeHeight).get<float>();
        obj.eyeSpacing = j.at(jsonKeys::settings::eyeSpacing).get<float>();
    }

    enum ReturnStatus {
        Saved,
        Canceled
    };


    json jsonData;
    json coordinatePairs;
    std::string dataFileName;

    OutputSettings outSettings;

    std::vector<std::string> frames;


    sf::RenderWindow window;

    void hideWindow() {
        window.setVisible(false);
        sf::Event event;
        while (window.pollEvent(event));
    }

    sf::Transform calculateTransform(CoordinatePair cp, OutputSettings out) {
        float dist = cp.dist();
        float angle = std::atan2(cp.lY - cp.rY, cp.lX - cp.rX);

        float rPosX = out.width * (1 - out.eyeSpacing) / 2;
        float rPosY = out.height * out.eyeHeight;

        sf::Transform t;
        t.translate(rPosX-cp.rX, rPosY-cp.rY);
        t.rotate(-angle * 360 / 6.2831f, cp.rX, cp.rY);
        float scale = (out.eyeSpacing * out.width) / dist;
        t.scale(scale, scale, cp.rX, cp.rY);
        return t;
    }

    sf::Image transformFrameGL(std::string frameName, OutputSettings out = outSettings) {
        sf::RenderTexture renderTex;
        renderTex.create(out.width, out.height);
        renderTex.clear(out.bgColor);

        sf::Texture tex;
        tex.loadFromFile(frameName);
        sf::Sprite sprite(tex);

        sf::Transform transform = calculateTransform(coordinatePairs[frameName], out);
        renderTex.draw(sprite, transform);

        auto img = renderTex.getTexture().copyToImage();
        img.flipVertically();
        return img;
    }

    ReturnStatus demandEyePositioning() {
        float windowScale = (float) WINDOWHEIGHT / outSettings.height;
        int wHeight = WINDOWHEIGHT;
        int wWidth = outSettings.width * windowScale;
        window.setSize(sf::Vector2u(wWidth, wHeight));

        window.setVisible(true);

        sf::Texture tex;
        tex.loadFromFile(frames[0]);

        sf::Sprite photo;
        photo.setTexture(tex);

        OutputSettings settings(wWidth, wHeight, outSettings.bgColor, outSettings.eyeHeight, outSettings.eyeSpacing);

        bool needsRepaint = true;

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
                        outSettings.eyeHeight = settings.eyeHeight;
                        outSettings.eyeSpacing = settings.eyeSpacing;
                        return Saved;
                    }
                    break;
                case sf::Event::MouseMoved : // Drag
                    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
                        settings.eyeSpacing = std::abs((float) event.mouseMove.x / window.getSize().x - 0.5) * 2;
                        settings.eyeHeight = (float) event.mouseMove.y / window.getSize().y;
                        needsRepaint = true;
                    }
                    break;
                case sf::Event::MouseButtonReleased : // Click
                    if (event.mouseButton.button == sf::Mouse::Button::Left){
                        settings.eyeSpacing = std::abs((float) event.mouseButton.x / window.getSize().x - 0.5) * 2;
                        settings.eyeHeight = (float) event.mouseButton.y / window.getSize().y;
                        needsRepaint = true;
                    }
                    break;
                default:
                    break;
                }
            }

            if (needsRepaint) {
                window.clear(outSettings.bgColor);

                sf::Transform t = calculateTransform(coordinatePairs[frames[0]], settings);
                window.draw(photo, t);
                needsRepaint = false;
                window.display();
            }
        }
        return Canceled;
    }

    std::vector<std::string> getUncompleteFrames(std::vector<std::string> frameSet){
        std::vector<std::string> ret;
        for (auto str : frameSet) {
            if (coordinatePairs.count(str) == 0) {
                ret.push_back(str);
            } else {
                CoordinatePair cp = coordinatePairs[str];
                if (!cp.isComplete())
                    ret.push_back(str);
            }
        }
        return ret;
    }

    ReturnStatus fillData(std::vector<std::string> frameSet) {
        int loadNumber = 0;
        int currentNumber = 0;
        float currentScale = 1;

        window.setVisible(true);

        sf::Texture tex;
        sf::Sprite photo;

        CoordinatePair currPair;

        bool needsRepaint = true;

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
                        coordinatePairs[frameSet[currentNumber]] = currPair;
                        return Saved;
                    }
                    if (event.key.code == sf::Keyboard::Right) {
                        coordinatePairs[frameSet[currentNumber]] = currPair;
                        loadNumber = std::min(currentNumber + 1, (int)frameSet.size() - 1);
                    }
                    if (event.key.code == sf::Keyboard::Left) {
                        coordinatePairs[frameSet[currentNumber]] = currPair;
                        loadNumber = std::max(currentNumber - 1, 0);
                    }
                    break;
                case sf::Event::MouseButtonPressed : {
                    float x = event.mouseButton.x / currentScale;
                    float y = event.mouseButton.y / currentScale;

                    if (event.mouseButton.button == sf::Mouse::Right) {
                        currPair.lX = x;
                        currPair.lY = y;
                    } else if (event.mouseButton.button == sf::Mouse::Left) {
                        currPair.rX = x;
                        currPair.rY = y;
                    }
                    needsRepaint = true;
                    break;
                }
                default:
                    break;
                }
            }

            // Load new frame if necessary
            if (loadNumber != -1) {
                if (tex.loadFromFile(frameSet[loadNumber])) {
                    photo.setTexture(tex);
                    float scaleX = (float)window.getSize().x / tex.getSize().x; // goal: > 1
                    float scaleY = (float)window.getSize().y / tex.getSize().y;
                    currentScale = std::min(scaleX, scaleY);
                    photo.setScale(currentScale, currentScale);
                    currPair = coordinatePairs[frameSet[loadNumber]];
                    currentNumber = loadNumber;
                    needsRepaint = true;
                } else {
                    std::cerr << "error loading frame " << frameSet[loadNumber] << std::endl;
                }
                loadNumber = -1;
            }

            if (needsRepaint) {
                window.clear(sf::Color::White);
                window.draw(photo);

                if (currPair.rX != -1) {
                    sf::Vector2f pos = sf::Vector2f(currPair.rX, currPair.rY) * currentScale;
                    sf::Vertex lines[] =
                    {
                        sf::Vertex(pos, sf::Color::Blue),
                        sf::Vertex(pos + sf::Vector2f(-20, -20), sf::Color::Blue),
                        sf::Vertex(pos, sf::Color::Blue),
                        sf::Vertex(pos + sf::Vector2f(-20, 20), sf::Color::Blue)
                    };

                    window.draw(lines, 4, sf::Lines);
                }
                if (currPair.lX != -1) {
                    sf::Vector2f pos = sf::Vector2f(currPair.lX, currPair.lY) * currentScale;
                    sf::Vertex lines[] =
                    {
                        sf::Vertex(pos, sf::Color::Red),
                        sf::Vertex(pos + sf::Vector2f(20, -20), sf::Color::Red),
                        sf::Vertex(pos, sf::Color::Red),
                        sf::Vertex(pos + sf::Vector2f(20, 20), sf::Color::Red)
                    };

                    window.draw(lines, 4, sf::Lines);
                }
                window.display();
                needsRepaint = false;
            }
        }
        return Canceled;
    }

    void writeData(){
        if (dataFileName != "") {
            jsonData[jsonKeys::coordinates] = coordinatePairs;
            jsonData[jsonKeys::outputsettings] = outSettings; 
            jsonData[jsonKeys::version] = 2;
            std::ofstream file(dataFileName);
            if (file.good()){
                file << jsonData;
            }
            file.close();
        }
    }

    int fmain(int argc, char* argv[]) {
        if (argc <= 1) {
            std::cout << "No arguments provided. Use " << argv[0] << " -? for help" << std::endl;
            return 0;
        }

        bool hasData = false;
        bool forceEyeWindow = false;
        bool forceAllFrames = false;
        std::string outputFolder;

        bool customColor = false;
        sf::Color backgroundColor;
        bool customResolution = false;
        int outWidth, outHeight;

        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-'){
                switch (argv[i][1]){
                    case 'e':
                        forceEyeWindow = true;
                        break;
                    case 'a':
                        forceAllFrames = true;
                        break;
                    case 'd': // datafile
                        {
                        ASSERT(argc > i + 1, "-d needs one argument. Usage: -d <datafile>")
                        dataFileName = argv[++i];
                        std::ifstream file(dataFileName);
                        if (file.good()){
                            file >> jsonData;
                            coordinatePairs = jsonData[jsonKeys::coordinates];
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
                    case 'r': // custom resolution
                        ASSERT(argc > i + 2, "-r needs two arguments. Usage: -r <outputwidth> <outputheigt>")
                        outWidth = std::stoi(argv[++i]);
                        outHeight = std::stoi(argv[++i]);
                        customResolution = true;
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
                        customResolution = true;
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
                        customColor = true;
                        break;
                        }
                    default:
                        std::cerr << "unknown option -" << argv[i][1] << std::endl;
                    case '?': // help
                        std::cout << "Usage: " << argv[0] << " [-d <file>] [-r <w> <h> | -R <720p=hd|1080p=fullhd>] [-c <r> <g> <b> <a> | -C <black|white|transparent>] [-e] [-a] [-o <folder>] frame0 frame1 ... frameN" << std::endl;
                        std::cout << "Go to https://github.com/Indeximal/FaceLapse for further information" << std::endl;
                        return 0;
                }
            } else {
                frames.push_back(std::string(argv[i]));
            }
        }

        // Load display & positioning data
        if (hasData) {
            if (jsonData.count(jsonKeys::version) == 0) {
                outSettings.height = jsonData["display"]["height"];
                outSettings.width = jsonData["display"]["width"];
                auto col = jsonData["display"]["color"];
                outSettings.bgColor = sf::Color(col[0], col[1], col[2], col[3]);
                outSettings.eyeHeight = jsonData["positioning"]["height"];
                outSettings.eyeSpacing = jsonData["positioning"]["gap"];

                for (json::iterator it = jsonData["frames"].begin(); it != jsonData["frames"].end(); ++it) {
                    json f = it.value();
                    CoordinatePair cp(f["RightEyePos"][0], f["RightEyePos"][1], f["LeftEyePos"][0], f["LeftEyePos"][1]);
                    coordinatePairs[it.key()] = cp;
                }
                jsonData = json();
            } else {
                outSettings = jsonData[jsonKeys::outputsettings];
            }
        }

        if (customColor) {
            outSettings.bgColor = backgroundColor;
        }

        if (customResolution) {
            outSettings.width = outWidth;
            outSettings.height = outHeight;
        }

        writeData();

        if (frames.empty()) {
            std::cout << "No frames to process" << std::endl;
            return 0;
        }

        window.create(sf::VideoMode(1280, 720), "Face Lapse Utility");
        window.setVisible(false);
        window.setFramerateLimit(60);

        // Eye indentification Phase
        std::vector<std::string> framesToDo = forceAllFrames ? frames : getUncompleteFrames(frames);
        if (framesToDo.size() > 0) {
            ReturnStatus result = fillData(framesToDo);
            hideWindow();
            if (result == Saved){ // if successful (Enter)
                writeData();
            } else { // canceled (ESC or close)
                std::cout << "Canceled eye indentification phase." << std::endl;
                return 0;
            }
        }

        // Only continue if all data is availiable
        if (getUncompleteFrames(frames).size() != 0) {
            std::cout << "Uncomplete dataset." << std::endl;
            return 0;
        }

        // Positioning Phase
        if (forceEyeWindow || !hasData) { // if phase 2 needed
            ReturnStatus result = demandEyePositioning();
            hideWindow();
            if (result == Saved) { // if successful (Enter)
                writeData();
            } else { // canceled (ESC or close)
                std::cout << "Canceled positioning phase." << std::endl;
                return 0;
            }
        }

        // Rendering Phase
        if (outputFolder != "") {  
            sf::Clock clock;
            float secPerFrame = 0;

            std::thread saverThread([](){});
            //sf::Image frameImage;
            
            int framesProcessed = 0;
            for (int i = 0; i < frames.size(); i++) {
                hideWindow();

                std::cout << "\r[" << i+1 << "/" << frames.size() << "]" << std::flush;
                std::string nr = std::to_string(i);
                std::string fileName = outputFolder + "frame" + std::string(5 - nr.length(), '0') + nr + ".png";

                sf::Image frameImage = transformFrameGL(frames[i]); // Transform ca 500ms

                saverThread.join(); // wait for last frame to finish saving
                saverThread = std::thread([=](){
                    bool success = frameImage.saveToFile(fileName); // save in saverThread
                    if (!success)
                        std::cerr << frames[i] << " couldn't be saved as " << fileName << std::endl; 
                });

                framesProcessed++;
                float timeLeft = clock.getElapsedTime().asSeconds() / framesProcessed * (frames.size() - i);
                std::cout << " eta: " << std::round(timeLeft) << "s   " << std::flush;
            }
            saverThread.join();

            std::cout << "\r" << frames.size() << " frames processed in " << clock.getElapsedTime().asMilliseconds() << "ms" << std::endl;
        }
        return 0;
    } // </int main()>
} // </namespace facelapse>

int main(int argc, char* argv[]) {
    return facelapse::fmain(argc, argv);
}