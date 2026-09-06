#pragma once

#include <format>
#include <memory>
#include <string>
#include <vector>

#include <Lattice/Tools/Text.hpp>
#include <Lattice/Tools/Logger.hpp>


namespace Logger {

class Tree {
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
        friend class Tree;

        std::string name_;
        std::vector<std::unique_ptr<TreeNode>> children_;
    };

    explicit Tree(std::string_view name)
        : root_(std::string(name)) {}

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

    void print() const {
        Logger::message("<b><w>{}<//>", root_.name_);
        printTreeNode(root_, "");
        Logger::blank();
    }

private:
    static void printTreeNode(const TreeNode& node, const std::string& prefix) {
        for (size_t i = 0; i < node.children_.size(); ++i) {
            const auto& child = node.children_[i];
            const bool last = i + 1 == node.children_.size();

            Logger::message(
                "{}{}─ <w>{}</>",
                prefix,
                last ? "└" : "├",
                child->name_
            );

            printTreeNode(
                *child,
                prefix + (last ? "   " : "│  ")
            );
        }
    }

    TreeNode root_;
    std::vector<TreeNode*> parents_;
};

}