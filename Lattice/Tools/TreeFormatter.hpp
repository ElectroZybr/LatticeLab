#pragma once

#include <format>
#include <memory>
#include <string>
#include <vector>

#include <Lattice/Tools/TextFormatter.hpp>


namespace Lattice {

struct TreeGlyphs {
    std::string_view branch;
    std::string_view lastBranch;
    std::string_view vertical;
    std::string_view horizontal;
};

namespace TreeStyles {

inline constexpr TreeGlyphs Modern{"├", "└", "│", "─"};
inline constexpr TreeGlyphs Ascii{"+", "\\", "|", "-"};

}

struct TreeFormatStyle {
    TreeGlyphs glyphs = TreeStyles::Modern;
    TextStyle root = TextStyle::Bold | TextStyle::rgb(0x5555ff);
    TextStyle node = TextStyle::None;
    TextStyle lines = TextStyle::rgb(0x555555);
    size_t indentation = 2;
};

class TreeFormatter {
public:
    class TreeNode {
    public:
        explicit TreeNode(std::string name)
            : name_(std::move(name)) {}

        TreeNode& branch(std::string_view name) {
            children_.push_back(std::make_unique<TreeNode>(std::string(name)));
            return *children_.back();
        }

        void node(std::string_view name) {
            children_.push_back(std::make_unique<TreeNode>(std::string(name)));
        }

    private:
        friend class TreeFormatter;

        std::string name_;
        std::vector<std::unique_ptr<TreeNode>> children_;
    };

    explicit TreeFormatter(
        std::string_view name,
        TreeFormatStyle style = {}
    ) : root_(std::string(name)), style_(style) {}

    TreeFormatter(
        std::string_view name,
        const TextTheme& theme,
        TreeFormatStyle style = {}
    ) : root_(std::string(name)), style_(style), theme_(&theme) {}

    TreeNode& branch(std::string_view name) {
        return root_.branch(name);
    }

    void node(std::string_view name) {
        root_.node(name);
    }

    void node(std::string_view name, size_t depth) {
        while (parents_.size() > depth)
            parents_.pop_back();

        TreeNode* parent = parents_.empty()
            ? &root_
            : parents_.back();

        TreeNode& node = parent->branch(name);
        parents_.push_back(&node);
    }

    TextFormatter format() const {
        TextFormatter output;
        output.append(TextFormatter(root_.name_, theme_), style_.root);
        appendTreeNode(root_, "", output);
        return output;
    }

private:
    void appendTreeNode(
        const TreeNode& node,
        const std::string& prefix,
        TextFormatter& output
    ) const {
        for (size_t i = 0; i < node.children_.size(); ++i) {
            const auto& child = node.children_[i];
            const bool last = i + 1 == node.children_.size();

            output.append("\n");
            output.append(prefix, style_.lines);
            output.append(
                last ? style_.glyphs.lastBranch : style_.glyphs.branch,
                style_.lines
            );
            output.append(style_.glyphs.horizontal, style_.lines);
            output.append(" ");
            output.append(TextFormatter(child->name_, theme_), style_.node);

            appendTreeNode(
                *child,
                prefix + (last ? " " : std::string(style_.glyphs.vertical)) +
                    std::string(style_.indentation, ' '),
                output
            );
        }
    }

    TreeNode root_;
    std::vector<TreeNode*> parents_;
    TreeFormatStyle style_;
    const TextTheme* theme_ = nullptr;
};

}
