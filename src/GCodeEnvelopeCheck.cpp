#include "GCodeEnvelopeCheck.h"

#include "ofUtils.h"
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

namespace grbl {

static bool inEnvelope(const Envelope& e, double x, double y, double z) {
    return (x >= e.minX && x <= e.maxX) && (y >= e.minY && y <= e.maxY) &&
           (z >= e.minZ && z <= e.maxZ);
}

static void stripGCodeComments(std::string& s) {
    std::size_t i = 0;
    while (i < s.size()) {
        if (s[i] == '(') {
            std::size_t j = s.find(')', i + 1);
            if (j == std::string::npos) {
                s.erase(i);
                break;
            }
            s.erase(i, j - i + 1);
            continue;
        }
        if (s[i] == ';') {
            s.erase(i);
            break;
        }
        ++i;
    }
    ofTrim(s);
}

struct Word {
    char   c = 0;
    double v = 0.0;
};

static bool parseLineWords(const std::string& uline, std::vector<Word>& out) {
    out.clear();
    for (std::size_t p = 0; p < uline.size();) {
        while (p < uline.size() && std::isspace(static_cast<unsigned char>(uline[p]))) {
            ++p;
        }
        if (p >= uline.size()) {
            break;
        }
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(uline[p])));
        if (!std::isalpha(static_cast<unsigned char>(c)) || c == '(' || c == ';') {
            break;
        }
        ++p;
        std::size_t numStart = p;
        if (p < uline.size() && (uline[p] == '+' || uline[p] == '-')) {
            ++p;
        }
        while (p < uline.size() &&
                (std::isdigit(static_cast<unsigned char>(uline[p])) || uline[p] == '.')) {
            ++p;
        }
        if (numStart == p) {
            break;
        }
        char* endp = nullptr;
        double v = std::strtod(uline.c_str() + numStart, &endp);
        if (endp != uline.c_str() + p) {
            break;
        }
        p = static_cast<std::size_t>(endp - uline.c_str());
        out.push_back(Word{ c, v });
    }
    return !out.empty();
}

static bool wordHasAxis(const std::vector<Word>& w, char axis) {
    for (const auto& x : w) {
        if (x.c == axis) {
            return true;
        }
    }
    return false;
}

static double wordValueOr(const std::vector<Word>& w, char axis, double fallback) {
    for (const auto& x : w) {
        if (x.c == axis) {
            return x.v;
        }
    }
    return fallback;
}

std::optional<std::string> checkGCodeBlockAgainstEnvelope(
    const std::string& gcode, const Envelope& env, float startX, float startY, float startZ) {
    double x        = startX, y = startY, z = startZ;
    bool   isMetric = true; // G21; G20 = inches
    bool   isAbs    = true; // G90; G91 = incremental
    // Modal motion group: 0 = G0, 1 = G1, 2 = G2, 3 = G3, -1 = none
    int modalMotion = -1;

    std::istringstream in(gcode);
    std::string        raw;
    int                lineNo = 0;
    while (std::getline(in, raw)) {
        ++lineNo;
        std::string line = ofToUpper(ofTrim(raw));
        stripGCodeComments(line);
        if (line.empty()) {
            continue;
        }

        std::vector<Word> w;
        if (!parseLineWords(line, w)) {
            continue;
        }

        // Update modals from this line
        for (const auto& wd : w) {
            if (wd.c != 'G' || wd.v < 0) {
                continue;
            }
            int gi = static_cast<int>(std::lround(wd.v));
            if (gi == 0 || gi == 1) {
                modalMotion = gi;
            } else if (gi == 2 || gi == 3) {
                modalMotion = 2; // treat arc as motion class 2
            } else if (gi == 20) {
                isMetric = false;
            } else if (gi == 21) {
                isMetric = true;
            } else if (gi == 90) {
                isAbs = true;
            } else if (gi == 91) {
                isAbs = false;
            }
        }

        bool   hasX  = wordHasAxis(w, 'X');
        bool   hasY  = wordHasAxis(w, 'Y');
        bool   hasZ  = wordHasAxis(w, 'Z');
        double tX    = hasX ? wordValueOr(w, 'X', x) : 0.0;
        double tY    = hasY ? wordValueOr(w, 'Y', y) : 0.0;
        double tZ    = hasZ ? wordValueOr(w, 'Z', z) : 0.0;
        if (!isMetric) {
            tX *= 25.4;
            tY *= 25.4;
            tZ *= 25.4;
        }

        // Motion: explicit G0/G1/G2/G3 on this line, or use modal
        int motion = -1;
        for (const auto& wd : w) {
            if (wd.c == 'G') {
                int g = static_cast<int>(std::lround(wd.v));
                if (g == 0 || g == 1) {
                    motion = g;
                    break;
                } else if (g == 2 || g == 3) {
                    motion = 2;
                    break;
                }
            }
        }
        if (motion < 0 && (hasX || hasY || hasZ)) {
            if (modalMotion == 0) {
                motion = 0;
            } else if (modalMotion == 1) {
                motion = 1;
            } else if (modalMotion == 2) {
                motion = 2;
            }
        }

        if (motion < 0 || (motion != 0 && motion != 1 && motion != 2)) {
            // Non-motion line: still update if pure G21/G90/ etc. (already done)
            continue;
        }
        if (!(hasX || hasY || hasZ)) {
            continue;
        }

        double nx = x, ny = y, nz = z;
        if (isAbs) {
            if (hasX) {
                nx = tX;
            }
            if (hasY) {
                ny = tY;
            }
            if (hasZ) {
                nz = tZ;
            }
        } else {
            if (hasX) {
                nx = x + tX;
            }
            if (hasY) {
                ny = y + tY;
            }
            if (hasZ) {
                nz = z + tZ;
            }
        }

        if (!inEnvelope(env, nx, ny, nz)) {
            std::ostringstream o;
            o << "G-code would leave the software work envelope (line " << lineNo
              << ", end X" << nx << " Y" << ny << " Z" << nz << " vs. min ("
              << env.minX << ", " << env.minY << ", " << env.minZ << ") max ("
              << env.maxX << ", " << env.maxY << ", " << env.maxZ << "))";
            return o.str();
        }

        x = nx;
        y = ny;
        z = nz;
    }
    return std::nullopt;
}

} // namespace grbl
