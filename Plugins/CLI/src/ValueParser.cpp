#include <CLI/include/ValueParser.hpp>

#include <charconv>
#include <cctype>
#include <cstdlib>
#include <string>

namespace CLIPlugin {

std::string_view trim(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.remove_suffix(1);
    return value;
}

std::optional<Lattice::Value> parseValue(
    std::string_view source,
    const Lattice::Value& current
) {
    source = trim(source);
    if (current.is<std::string>())
        return Lattice::Value{std::string(source)};

    if (current.is<bool>()) {
        if (source == "true" || source == "on" || source == "1")
            return Lattice::Value{true};
        if (source == "false" || source == "off" || source == "0")
            return Lattice::Value{false};
        return std::nullopt;
    }

    if (current.is<int64_t>()) {
        int64_t value = 0;
        const auto [end, error] = std::from_chars(
            source.data(),
            source.data() + source.size(),
            value
        );
        if (error == std::errc{} && end == source.data() + source.size())
            return Lattice::Value{value};
        return std::nullopt;
    }

    if (current.is<double>()) {
        std::string owned(source);
        char* end = nullptr;
        const double value = std::strtod(owned.c_str(), &end);
        if (end == owned.c_str() + owned.size())
            return Lattice::Value{value};
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> parseArguments(std::string_view source) {
    std::vector<std::string> result;
    size_t position = 0;

    while (position < source.size()) {
        while (position < source.size() && std::isspace(static_cast<unsigned char>(source[position])))
            ++position;
        if (position == source.size())
            break;

        std::string argument;
        char quote = 0;

        while (position < source.size()) {
            const char value = source[position++];

            if (quote != 0) {
                if (value == quote) {
                    quote = 0;
                } else if (value == '\\' && position < source.size()) {
                    argument += source[position++];
                } else {
                    argument += value;
                }
                continue;
            }

            if (value == '\'' || value == '"') {
                quote = value;
            } else if (value == '\\' && position < source.size()) {
                argument += source[position++];
            } else if (std::isspace(static_cast<unsigned char>(value))) {
                break;
            } else {
                argument += value;
            }
        }

        if (quote != 0)
            return std::nullopt;

        result.push_back(std::move(argument));
    }

    return result;
}

Lattice::NodeId resolveTreePath(
    const Lattice::TreeView& tree,
    Lattice::NodeId current,
    std::string_view expression
) {
    if (!tree.contains(current))
        return Lattice::InvalidNodeId;

    const Lattice::NodeId root = tree.root(current);
    if (expression.empty())
        return current;

    if (expression.front() == '/') {
        current = root;
        expression.remove_prefix(1);
    }

    size_t begin = 0;
    bool first = true;

    while (begin <= expression.size()) {
        size_t end = expression.find('/', begin);
        if (end == std::string_view::npos)
            end = expression.size();

        const std::string_view segment = expression.substr(begin, end - begin);
        if (!segment.empty() && segment != ".") {
            if (segment == "..") {
                const Lattice::NodeId parent = tree.parent(current);
                if (parent != Lattice::InvalidNodeId)
                    current = parent;
            } else if (first && segment == tree.label(root)) {
                current = root;
            } else {
                Lattice::NodeId found = Lattice::InvalidNodeId;

                for (Lattice::NodeId child : tree.children(current)) {
                    if (tree.label(child) != segment)
                        continue;

                    if (found != Lattice::InvalidNodeId)
                        return Lattice::InvalidNodeId;

                    found = child;
                }

                if (found == Lattice::InvalidNodeId)
                    return Lattice::InvalidNodeId;

                current = found;
            }
        }

        first = false;
        if (end == expression.size())
            break;
        begin = end + 1;
    }

    return current;
}

}
