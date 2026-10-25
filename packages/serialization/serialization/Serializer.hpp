#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <Lattice/kernel/NodeSystem.hpp>
#include <stdio/IOSubsystem.hpp>

namespace Serialization {

/**
 * Applies a partial runtime tree and stores explicitly assigned parameter values.
 * This infrastructure object uses NodeSystem directly; it retains no NodeBuild.
 * File parsing/writing belongs to the supplied StdIo subsystem.
 */
class Serializer {
public:
    Serializer(Lattice::NodeSystem& nodes, Lattice::NodeId root, IOSubsystem& io);
    ~Serializer();
    Serializer(const Serializer&) = delete;
    Serializer& operator=(const Serializer&) = delete;
    void load(std::optional<std::string> path);
    void save(std::optional<std::string> path);
    std::string status() const;

private:
    Lattice::NodeSystem& nodes_;
    Lattice::NodeId root_;
    uint64_t generation_;
    IOSubsystem& io_;
    std::filesystem::path path_;
    std::optional<Document> document_;

    uint64_t observer_;
    std::unordered_map<Lattice::ExportId, Lattice::Value> overrides_;
    std::unordered_map<Lattice::NodeId, uint64_t> declarations_;

    void changed(Lattice::ExportId id, Lattice::ParamChange change);
    Lattice::Object layer(Lattice::NodeId owner);
    std::string key(Lattice::NodeId owner) const;
    bool declared(Lattice::NodeId owner) const;
    void checkRoot() const;
    void visit(Lattice::NodeId owner, Lattice::Object& config, const std::string& path);
    Lattice::NodeId child(Lattice::NodeId parent, std::string_view key, const std::string& path);
};
}
