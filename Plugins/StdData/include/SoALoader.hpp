#pragma once

#include "SoA.hpp"
#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/Exception.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <Lattice/Tools/Logger.hpp>
#include "StdIo/include/LoaderAPI.hpp"


class SoALoader final : public LoaderAPI {
    static constexpr std::string_view tag = "SoALoader";

public:
    void configure(Lattice::Node& branch) {
        branch_ = &branch;
    }

    std::string_view section() const override {
        return "SoAData";
    }

    void load(const Value* data) override {
        if (!data || !data->is<Table>())
            return;

        if (!branch_.exists())
            throw Lattice::Exception(tag, "loader is not configured");

        const auto& table = std::get<Table>(*data);

        const auto targetIt = table.find("target");
        const auto columnsIt = table.find("columns");
        const auto rowsIt = table.find("rows");

        if (targetIt == table.end() || !targetIt->second.is<std::string>())
            throw Lattice::Exception(tag, "SoAData.target is missing");

        if (columnsIt == table.end() || !columnsIt->second.is<Array>())
            throw Lattice::Exception(tag, "SoAData.columns is missing");

        if (rowsIt == table.end() || !rowsIt->second.is<Array>())
            throw Lattice::Exception(tag, "SoAData.rows is missing");

        const auto& target = std::get<std::string>(targetIt->second);
        const auto& columnNames = std::get<Array>(columnsIt->second);
        const auto& rowData = std::get<Array>(rowsIt->second);

        if (columnNames.empty())
            return;

        StdData::SoA* soa = resolveTarget(target);

        for (const auto& value : columnNames) {
            if (!value.is<std::string>())
                throw Lattice::Exception(tag, "column name must be a string");

            const auto& name = std::get<std::string>(value);
            if (!soa->has(name))
                throw Lattice::Exception(tag, "column '{}' not found in '{}'", name, target);
        }

        const size_t offset = soa->size();
        soa->resize(offset + rowData.size());

        for (size_t row = 0; row < rowData.size(); ++row) {
            if (!rowData[row].is<Array>())
                throw Lattice::Exception(tag, "row {} must be an array", row);

            const auto& values = std::get<Array>(rowData[row]);

            if (values.size() != columnNames.size())
                throw Lattice::Exception(
                    tag,
                    "row {} has {} values, expected {}",
                    row,
                    values.size(),
                    columnNames.size()
                );

            for (size_t column = 0; column < columnNames.size(); ++column) {
                const auto& name = std::get<std::string>(columnNames[column]);
                soa->set(name, offset + row, values[column]);
            }
        }

        Logger::ok(tag, "loaded {} rows into '{}'", rowData.size(), target);
    }

private:
    StdData::SoA* resolveTarget(std::string_view target) const {
        Lattice::Context& ctx = branch_->requireContext();
        const Lattice::ObjectId id = ctx.active(target);

        if (!Lattice::Objects::valid(id))
            throw Lattice::Exception(tag, "SoA target '{}' is not active in context", target);

        Lattice::Node* node = ctx.objects.require(id).node;
        if (auto soa = node->find<StdData::SoA>())
            return soa.get();

        throw Lattice::Exception(tag, "no SoA buffer under target '{}'", target);
    }

    Ref<Lattice::Node> branch_;
};