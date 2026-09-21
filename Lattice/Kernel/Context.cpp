#include <Lattice/Kernel/Context.hpp>
#include <Lattice/Kernel/Node.hpp>
#include <format>

namespace Lattice {

void Context::appendTree(Logger::Tree& tree) const {
    const auto label = [this](ObjectId id) -> std::string {
        if (id == InvalidObjectId) return "Empty";
        const auto* entry = objects.get(id);
        return entry && entry->node ? entry->node->stringPath() : std::format("missing #{}", id);
    };

    tree.node("<b><c>Scopes<//>", 0);
    for (FocusScopeId id = 0; id < focusScopes.size(); ++id) {
        const auto* scope = focusScopes.get(id);
        if (!scope) continue;
        std::string state;
        if (id == rootScope) state += " <m>[root]</>";
        if (std::ranges::find(activeScopes, id) != activeScopes.end()) state += " <m>[active]</>";
        const auto type = scope->type == Blueprints::InvalidId ? "untyped" : blueprints.require(scope->type).name;
        tree.node(std::format("{}{} <gr>#{} [{}]</>", label(scope->owner), state, id, type), 1);
        for (const auto& entry : scope->roles)
            tree.node(std::format("{} ➜ <gr>{}</>", roles.require(entry.role).name, label(entry.target)), 2);
    }

    tree.node("<b><c>Resolved roles<//>", 0);
    for (RoleId role = 0; role < roles.size(); ++role)
        tree.node(std::format("{} ➜ <gr>{}</>", roles.require(role).name, label(resolveFocus(InvalidFocusScopeId, role))), 1);
}

void Context::printTree() const {
    Logger::Tree tree("Focus");
    appendTree(tree);
    tree.print();
}

}
