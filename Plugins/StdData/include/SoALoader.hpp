// #pragma once

// #include <Lattice/Kernel/Context.hpp>
// #include <Lattice/Kernel/Exception.hpp>
// #include <Lattice/Kernel/NodeViews.hpp>
// #include <Lattice/Tools/Logger.hpp>

// #include "StdIo/include/LoaderAPI.hpp"
// #include "SoA.hpp"

// class SoALoader final : public LoaderAPI {
//     static constexpr std::string_view tag = "SoALoader";
//     ExportsView exports;

// public:
//     void configure(NodeConfigure node) {
//         exports = node.exports();
//     }

//     std::string_view section() const override { return "SoA"; }

//     void load(const Lattice::Value& section) override {
//         if (!exports.exists())
//             throw Lattice::Exception(tag, "loader is not configured");

//         if (!section.is<Lattice::Object>())
//             return;

//         const auto& table = std::get<Lattice::Object>(section);

//         const auto targetIt = table.find("target");
//         const auto columnsIt = table.find("columns");
//         const auto rowsIt = table.find("rows");

//         if (targetIt == table.end() || !targetIt->second.is<std::string>())
//             throw Lattice::Exception(tag, "Dataset.target is missing");

//         if (columnsIt == table.end() || !columnsIt->second.is<Lattice::Array>())
//             throw Lattice::Exception(tag, "Dataset.columns is missing");

//         if (rowsIt == table.end() || !rowsIt->second.is<Lattice::Array>())
//             throw Lattice::Exception(tag, "Dataset.rows is missing");

//         const auto& target = std::get<std::string>(targetIt->second);
//         const auto& columnNames = std::get<Lattice::Array>(columnsIt->second);
//         const auto& rowData = std::get<Lattice::Array>(rowsIt->second);

//         if (columnNames.empty())
//             return;

//         StdData::SoA* soa = resolveTarget(target);

//         for (const auto& value : columnNames) {
//             if (!value.is<std::string>())
//                 throw Lattice::Exception(tag, "column name must be a string");

//             const auto& name = std::get<std::string>(value);
//             if (!soa->has(name))
//                 throw Lattice::Exception(tag, "column '{}' not found in '{}'", name, target);
//         }

//         const size_t offset = soa->size();

//         // for (size_t row = 0; row < rowData.size(); ++row) {
//         //     if (!rowData[row].is<Array>())
//         //         throw Lattice::Exception(tag, "row {} must be an array", row);

//         //     const auto& values = std::get<Array>(rowData[row]);

//         //     if (values.size() != columnNames.size())
//         //         throw Lattice::Exception(tag, "row {} has {} values, expected {}", row, values.size(), columnNames.size());

//         //     if (!values[0].is<std::string>())
//         //         throw Lattice::Exception(tag, "row {} target name must be a string", row);

//         //     const auto& name = std::get<std::string>(values[0]);
//         //     const auto index = soa->find(name);

//         //     if (!index) {
//         //         Logger::warning(tag, "unknown '{}' in '{}', skipping row", name, target);
//         //         continue;
//         //     }

//         //     for (size_t column = 0; column < columnNames.size(); ++column)
//         //         soa->set(std::get<std::string>(columnNames[column]), *index, values[column]);
//         // }

//         Logger::ok(tag, "loaded {} rows into '{}'", rowData.size(), target);
//     }

// private:
//     StdData::SoA* resolveTarget(std::string_view target) const {
//         Lattice::Context& ctx = branch_->requireContext();
//         const Lattice::ObjectId id = ctx.resolveFocus(Lattice::InvalidFocusScopeId, ctx.findRole(target));

//         if (id == Lattice::InvalidObjectId)
//             throw Lattice::Exception(tag, "SoA target '{}' is not active in context", target);

//         Lattice::Node* node = ctx.objects.require(id).node;
//         if (auto* soa = node->get<StdData::SoA>())
//             return soa;
//         if (auto soa = node->find<StdData::SoA>())
//             return soa.get();

//         throw Lattice::Exception(tag, "no SoA buffer under target '{}'", target);
//     }
// };