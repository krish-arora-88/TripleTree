#include "triplefill/animation.hpp"
#include "triplefill/fill.hpp"
#include "triplefill/image.hpp"
#include "triplefill/pixel.hpp"
#include "triplefill/point.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Args {
    std::string input;
    std::string output;
    triplefill::Point seed{0, 0};
    double tolerance          = 0.1;
    int    frame_freq         = 1000;
    triplefill::Algorithm algo = triplefill::Algorithm::BFS;
    std::string picker_name   = "solid";

    triplefill::RGBA color{255, 0, 0, 255};
    triplefill::RGBA color1{255, 0, 0, 255};
    triplefill::RGBA color2{0, 0, 255, 255};
    unsigned stripe_width     = 10;
    int      bright           = 40;
    unsigned border_width     = 3;
    triplefill::RGBA border_color{0, 0, 0, 255};
};

void print_usage(const char* prog) {
    std::cerr
        << "Usage: " << prog << " [OPTIONS]\n\n"
        << "  --input <path.png>         Input PNG image\n"
        << "  --output <path.png|gif>    Output file (.png or .gif)\n"
        << "  --seed <x,y>              Seed pixel coordinates\n"
        << "  --tolerance <double>       Colour tolerance (default 0.1)\n"
        << "  --frame-freq <int>         Frame capture frequency (default 1000)\n"
        << "  --algo <bfs|dfs>           Fill algorithm (default bfs)\n"
        << "  --picker <solid|stripe|quarter|border>\n"
        << "\n  Picker parameters:\n"
        << "    solid:   --color <r,g,b,a>\n"
        << "    stripe:  --stripe-width N --color1 <r,g,b,a> --color2 <r,g,b,a>\n"
        << "    quarter: --color <r,g,b,a> --bright N\n"
        << "    border:  --color <r,g,b,a> --border-width N --border-color <r,g,b,a>\n"
        << "\n  --help                     Show this message\n";
}

triplefill::RGBA parse_rgba(const std::string& s) {
    int r = 0, g = 0, b = 0, a = 255;
    char sep;
    std::istringstream ss(s);
    ss >> r >> sep >> g >> sep >> b;
    if (ss.peek() == ',') ss >> sep >> a;
    return {static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(g),
            static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(a)};
}

triplefill::Point parse_point(const std::string& s) {
    int x = 0, y = 0;
    char sep;
    std::istringstream ss(s);
    ss >> x >> sep >> y;
    return {x, y};
}

bool parse_args(int argc, char* argv[], Args& args) {
    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Missing value for " << arg << "\n";
                return {};
            }
            return argv[++i];
        };

        if (arg == "--help")          { print_usage(argv[0]); return false; }
        else if (arg == "--input")    args.input         = next();
        else if (arg == "--output")   args.output        = next();
        else if (arg == "--seed")     args.seed          = parse_point(next());
        else if (arg == "--tolerance") args.tolerance     = std::stod(next());
        else if (arg == "--frame-freq") args.frame_freq   = std::stoi(next());
        else if (arg == "--algo") {
            auto v = next();
            args.algo = (v == "dfs") ? triplefill::Algorithm::DFS
                                     : triplefill::Algorithm::BFS;
        }
        else if (arg == "--picker")       args.picker_name  = next();
        else if (arg == "--color")        args.color        = parse_rgba(next());
        else if (arg == "--color1")       args.color1       = parse_rgba(next());
        else if (arg == "--color2")       args.color2       = parse_rgba(next());
        else if (arg == "--stripe-width") args.stripe_width = static_cast<unsigned>(std::stoi(next()));
        else if (arg == "--bright")       args.bright       = std::stoi(next());
        else if (arg == "--border-width") args.border_width = static_cast<unsigned>(std::stoi(next()));
        else if (arg == "--border-color") args.border_color = parse_rgba(next());
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    Args args;
    if (!parse_args(argc, argv, args))
        return 1;

    if (args.input.empty() || args.output.empty()) {
        std::cerr << "Error: --input and --output are required.\n";
        print_usage(argv[0]);
        return 1;
    }

    try {
        auto img = triplefill::load_png(args.input);
        std::cerr << "Loaded " << img.width() << "x" << img.height()
                  << " image from " << args.input << "\n";

        triplefill::PickerConfig picker;
        if (args.picker_name == "solid") {
            picker = triplefill::SolidPicker{args.color};
        } else if (args.picker_name == "stripe") {
            picker = triplefill::StripePicker{args.color1, args.color2,
                                              args.stripe_width};
        } else if (args.picker_name == "quarter") {
            picker = triplefill::QuarterPicker{args.color, args.bright};
        } else if (args.picker_name == "border") {
            picker = triplefill::BorderPicker{args.color, args.border_color,
                                              args.border_width};
        } else {
            std::cerr << "Unknown picker: " << args.picker_name << "\n";
            return 1;
        }

        triplefill::FillConfig cfg{
            .seed       = args.seed,
            .tolerance  = args.tolerance,
            .frame_freq = args.frame_freq,
            .algorithm  = args.algo,
            .picker     = picker,
        };

        std::cerr << "Running flood fill ("
                  << (args.algo == triplefill::Algorithm::BFS ? "BFS" : "DFS")
                  << ") from (" << args.seed.x << "," << args.seed.y << ")...\n";

        auto anim = triplefill::flood_fill(img, cfg);

        std::cerr << "Fill complete: " << anim.size() << " frames captured.\n";

        // Create output directory if needed
        auto parent = std::filesystem::path(args.output).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);

        auto ext = std::filesystem::path(args.output).extension().string();
        if (ext == ".gif") {
            anim.write_gif(args.output);
            std::cerr << "Wrote animated GIF to " << args.output << "\n";
        } else {
            anim.write_last_png(args.output);
            std::cerr << "Wrote final PNG to " << args.output << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
