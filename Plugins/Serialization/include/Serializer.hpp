#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <Lattice/Kernel/NodeSystem.hpp>
#include <IOSubsystem.hpp>

namespace Serialization {

/**
 * Applies a partial runtime tree and saves only parameters listed in its document.
 * This infrastructure object uses NodeSystem directly; it retains no NodeBuild.
 * File parsing/writing belongs to the supplied StdIo subsystem.
 */
class Serializer {
public:
    Serializer(Lattice::NodeSystem& nodes, Lattice::NodeId root, IOSubsystem& io);
    void load(std::optional<std::string> path);
    void save(std::optional<std::string> path);

private:
    Lattice::NodeSystem& nodes_;
    Lattice::NodeId root_;
    uint64_t generation_;
    IOSubsystem& io_;
    std::filesystem::path path_;
    std::optional<Document> document_;

    void checkRoot() const;
    void visit(Lattice::NodeId owner, Lattice::Object& config, bool loading, const std::string& path);
    Lattice::NodeId child(Lattice::NodeId parent, const Lattice::Object& config, bool create, const std::string& path);
};
}
