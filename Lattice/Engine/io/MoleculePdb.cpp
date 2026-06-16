#include "MoleculePdb.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "Lattice/Engine/physics/Atom/AtomData.h"

namespace Lattice {
namespace {

std::string trim(std::string_view value) {
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return std::string(value.substr(begin, end - begin));
}

std::string uppercase(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string normalizeElementSymbol(std::string_view value) {
    value = trim(value);
    if (value.empty()) {
        return {};
    }

    std::string letters;
    letters.reserve(value.size());
    for (char ch : value) {
        if (std::isalpha(static_cast<unsigned char>(ch)) != 0) {
            letters.push_back(ch);
        }
    }

    if (letters.empty()) {
        return letters;
    }

    letters = uppercase(letters);
    if (letters.size() == 1) {
        return letters;
    }

    letters.resize(2);
    letters[1] = static_cast<char>(std::tolower(static_cast<unsigned char>(letters[1])));
    return letters;
}

AtomData::Type parseAtomTypeFromSymbol(const std::string& rawSymbol) {
    const std::string symbol = normalizeElementSymbol(rawSymbol);
    for (size_t i = 0; i < static_cast<size_t>(AtomData::Type::COUNT); ++i) {
        const AtomData::Type type = static_cast<AtomData::Type>(i);
        if (AtomData::symbol(type) == symbol) {
            return type;
        }
    }

    throw std::runtime_error("MoleculePdb: unknown atom type '" + rawSymbol + "'");
}

std::string extractElementSymbol(const std::string& line) {
    if (line.size() >= 78) {
        const std::string element = normalizeElementSymbol(std::string_view(line).substr(76, 2));
        if (!element.empty()) {
            return element;
        }
    }

    if (line.size() >= 16) {
        return normalizeElementSymbol(std::string_view(line).substr(12, 4));
    }

    throw std::runtime_error("MoleculePdb: failed to extract atom element");
}

int parseIntField(const std::string& line, size_t offset, size_t width) {
    return std::stoi(trim(std::string_view(line).substr(offset, width)));
}

float parseFloatField(const std::string& line, size_t offset, size_t width) {
    return std::stof(trim(std::string_view(line).substr(offset, width)));
}

glm::vec3 parseCoordinates(const std::string& line) {
    if (line.size() >= 54) {
        const std::string xField = trim(std::string_view(line).substr(30, 8));
        const std::string yField = trim(std::string_view(line).substr(38, 8));
        const std::string zField = trim(std::string_view(line).substr(46, 8));
        const bool fixedWidthLooksValid =
            !xField.empty() && !yField.empty() && !zField.empty() && xField.find(' ') == std::string::npos && yField.find(' ') == std::string::npos &&
            zField.find(' ') == std::string::npos;
        if (fixedWidthLooksValid) {
            return glm::vec3(std::stof(xField), std::stof(yField), std::stof(zField));
        }
    }

    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 8) {
        throw std::runtime_error("MoleculePdb: failed to parse atom coordinates");
    }

    return glm::vec3(std::stof(tokens[5]), std::stof(tokens[6]), std::stof(tokens[7]));
}

MoleculeTemplate loadTemplateImpl(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        throw std::runtime_error("MoleculePdb: failed to open file '" + path.string() + "'");
    }

    MoleculeTemplate molecule;
    std::unordered_map<int, uint32_t> atomIndexBySerial;

    std::string line;
    while (std::getline(input, line)) {
        if (line.size() < 6) {
            continue;
        }

        const std::string record = trim(std::string_view(line).substr(0, 6));
        if (record == "ATOM" || record == "HETATM") {
            if (line.size() < 54) {
                continue;
            }

            const int serial = parseIntField(line, 6, 5);
            const AtomData::Type type = parseAtomTypeFromSymbol(extractElementSymbol(line));
            const glm::vec3 coords = parseCoordinates(line);

            atomIndexBySerial.emplace(serial, static_cast<uint32_t>(molecule.atoms.size()));
            molecule.atoms.push_back({type, coords});
            continue;
        }

        if (record == "CONECT") {
            if (line.size() < 11) {
                continue;
            }

            const int sourceSerial = parseIntField(line, 6, 5);
            const auto sourceIt = atomIndexBySerial.find(sourceSerial);
            if (sourceIt == atomIndexBySerial.end()) {
                continue;
            }

            for (size_t offset = 11; offset + 5 <= line.size(); offset += 5) {
                const std::string targetField = trim(std::string_view(line).substr(offset, 5));
                if (targetField.empty()) {
                    continue;
                }

                const int targetSerial = std::stoi(targetField);
                const auto targetIt = atomIndexBySerial.find(targetSerial);
                if (targetIt == atomIndexBySerial.end()) {
                    continue;
                }

                const uint32_t atomA = sourceIt->second;
                const uint32_t atomB = targetIt->second;
                if (atomA == atomB) {
                    continue;
                }

                const auto [minIt, maxIt] = std::minmax(atomA, atomB);
                const bool exists = std::any_of(molecule.bonds.begin(), molecule.bonds.end(), [&](const MoleculeBond& bond) {
                    return bond.atomA == minIt && bond.atomB == maxIt;
                });
                if (!exists) {
                    molecule.bonds.push_back({minIt, maxIt});
                }
            }
        }
    }

    if (molecule.atoms.empty()) {
        throw std::runtime_error("MoleculePdb: no atoms found in '" + path.string() + "'");
    }

    glm::vec3 centroid(0.0f);
    for (const MoleculeAtom& atom : molecule.atoms) {
        centroid += atom.localPos;
    }
    centroid /= static_cast<float>(molecule.atoms.size());

    for (MoleculeAtom& atom : molecule.atoms) {
        atom.localPos -= centroid;
    }

    return molecule;
}

} // namespace

MoleculeTemplate MoleculePdb::loadTemplate(const std::filesystem::path& path) { return loadTemplateImpl(path); }

} // namespace Lattice
