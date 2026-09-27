// #pragma once

// #include <Lattice/Kernel/Exception.hpp>
// #include <Lattice/Kernel/NodeViews.hpp>
// #include <Lattice/Tools/Logger.hpp>

// #include "StdIo/include/LoaderAPI.hpp"
// #include "NamedSoA.hpp"

// class NamedSoALoader final : public LoaderAPI {
//     static constexpr std::string_view tag = "NamedSoALoader";
//     ExportsView exports;

// public:
//     void configure(NodeConfigure node) {
//         exports = node.exports();
//     }

//     std::string_view section() const override { return "NamedSoA"; }

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

//         StdData::NamedSoA* soa = resolveTarget(target);

//         const auto& keyName = std::get<std::string>(columnNames[0]);

//         if (!soa->has(keyName))
//             return;

//         std::vector<bool> loadColumn(columnNames.size(), false);
//         loadColumn[0] = true;

//         bool hasDataColumn = false;

//         for (size_t column = 1; column < columnNames.size(); ++column) {
//             const auto& name = std::get<std::string>(columnNames[column]);

//             if (!soa->has(name)) {
//                 Logger::warning(tag, "column '{}' not found in '{}', skipping", name, target);
//                 continue;
//             }

//             loadColumn[column] = true;
//             hasDataColumn = true;
//         }

//         if (!hasDataColumn)
//             return;

//         soa->reserve(rowData.size());

//         size_t loaded = 0;
//         size_t added = 0;

//         for (size_t row = 0; row < rowData.size(); ++row) {
//             if (!rowData[row].is<Lattice::Array>())
//                 throw Lattice::Exception(tag, "row {} must be an array", row);

//             const auto& values = std::get<Lattice::Array>(rowData[row]);

//             if (values.size() != columnNames.size())
//                 throw Lattice::Exception(tag, "row {} has {} values, expected {}", row, values.size(), columnNames.size());

//             if (!values[0].is<std::string>())
//                 throw Lattice::Exception(tag, "row {} identifier must be a string", row);

//             const auto& key = std::get<std::string>(values[0]);
//             size_t index = soa->find(key);

//             if (!Lattice::NamedRegistry<size_t>::valid(index)) {
//                 index = soa->addRow(key);
//                 ++added;
//             }

//             for (size_t column = 0; column < columnNames.size(); ++column) {
//                 if (loadColumn[column])
//                     soa->set(std::get<std::string>(columnNames[column]), index, values[column]);
//             }

//             ++loaded;
//         }

//         Logger::ok(tag, "loaded {} rows into '{}' ({} new)", loaded, target, added);
//     }

// private:
//     StdData::NamedSoA* resolveTarget(std::string_view target) const {
//         Lattice::Context& ctx = branch_->requireContext();
//         const Lattice::ObjectId id = ctx.resolveFocus(Lattice::InvalidFocusScopeId, ctx.findRole(target));

//         if (id == Lattice::InvalidObjectId)
//             throw Lattice::Exception(tag, "NamedSoA target '{}' is not active in context", target);

//         Lattice::Node* node = ctx.objects.require(id).node;

//         if (auto* soa = node->get<StdData::NamedSoA>())
//             return soa;
//         if (auto soa = node->find<StdData::NamedSoA>())
//             return soa.get();

//         throw Lattice::Exception(tag, "no NamedSoA buffer under target '{}'", target);
//     }

//     Ref<Lattice::Node> branch_;
// };