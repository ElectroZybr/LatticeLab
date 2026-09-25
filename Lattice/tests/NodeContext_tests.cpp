#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>

namespace Lattice {

TEST(NodeContext_CreateRole, RuntimeFixture,
    "Повторный запрос одной роли должен возвращать тот же RoleId.")
{
    const RoleId a = fixture.run_ctx.nodes.context.role("Component");
    const RoleId b = fixture.run_ctx.nodes.context.role("Component");

    REQUIRE(a == b);
    REQUIRE(fixture.run_ctx.nodes.context.findRole("Component") == a);
}

TEST(NodeContext_CreateScope, RuntimeFixture,
    "Повторное создание scope для одной ноды должно возвращать тот же ContextScopeId.")
{
    const ContextScopeId a = fixture.run_ctx.nodes.context.createScope(fixture.root);
    const ContextScopeId b = fixture.run_ctx.nodes.context.createScope(fixture.root);

    REQUIRE(a == b);
    REQUIRE(fixture.run_ctx.nodes.context.findScope(fixture.root) == a);
}

TEST(NodeContext_RootScope, RuntimeFixture,
    "Scope корневой ноды должен становиться root scope.")
{
    const ContextScopeId scope = fixture.run_ctx.nodes.context.createScope(fixture.root);

    REQUIRE(fixture.run_ctx.nodes.context.root() == scope);
}

TEST(NodeContext_SetGet, RuntimeFixture,
    "set должен записывать target роли в scope.")
{
    const NodeId target = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.createScope(fixture.root);
    const RoleId role = fixture.run_ctx.nodes.context.role("Component");

    fixture.run_ctx.nodes.context.addCandidate(scope, role, target);
    fixture.run_ctx.nodes.context.set(scope, role, target);

    REQUIRE(fixture.run_ctx.nodes.context.resolveInfo(scope, role).target == target);
}

TEST(NodeContext_SetUpdatesEntry, RuntimeFixture,
    "Повторный set одной роли должен заменять target, а не создавать вторую запись.")
{
    const NodeId a = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    const NodeId b = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(fixture.root);
    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    fixture.run_ctx.nodes.context.set(scope, role, b);

    REQUIRE(fixture.run_ctx.nodes.context.resolveInfo(scope, role).target == b);
    REQUIRE(fixture.run_ctx.nodes.context.scope(scope)->roles.size() == 1);
}

TEST(NodeContext_ResolveLocal, RuntimeFixture,
    "Несколько локальных кандидатов без выбора должны быть неоднозначны.")
{
    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(branch);
    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    REQUIRE(fixture.run_ctx.nodes.context.resolveInfo(scope, role).state == ContextResolutionState::Ambiguous);
}

TEST(NodeContext_ResolveInherited, RuntimeFixture,
    "Неоднозначность родительского scope должна наследоваться.")
{
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const ContextScopeId branchScope = fixture.run_ctx.nodes.context.createScope(branch);
    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    REQUIRE(fixture.run_ctx.nodes.context.resolveInfo(branchScope, role).state == ContextResolutionState::Ambiguous);
}

TEST(NodeContext_ResetRestoresInheritance, RuntimeFixture,
    "reset должен удалить локальный override и восстановить наследование.")
{
    const NodeId rootA = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");
    const ContextScopeId rootScope = fixture.run_ctx.nodes.context.findScope(fixture.root);
    fixture.run_ctx.nodes.context.set(rootScope, role, rootA);

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const NodeId branchA = fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(branch);
    fixture.run_ctx.nodes.context.set(scope, role, branchA);

    fixture.run_ctx.nodes.context.reset(scope, role);

    REQUIRE(fixture.run_ctx.nodes.context.resolve(scope, role) == rootA);
}

TEST(NodeContext_LocalScopeInheritsParentAmbiguity, RuntimeFixture,
    "Пустой локальный scope должен наследовать неоднозначность родителя.")
{
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const ContextScopeId scope = fixture.run_ctx.nodes.context.createScope(branch);
    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    REQUIRE(fixture.run_ctx.nodes.context.resolveInfo(scope, role).state == ContextResolutionState::Ambiguous);
}

TEST(NodeContext_ParentScope, RuntimeFixture,
    "parentScope должен пропускать ноды без scope и находить ближайший родительский scope.")
{
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const NodeId folder = fixture.run_ctx.nodes.factory.folder(fixture.root, "folder");
    const NodeId branch = fixture.run_ctx.nodes.factory.folder(folder, "branch");

    const ContextScopeId rootScope = fixture.run_ctx.nodes.context.findScope(fixture.root);
    const ContextScopeId branchScope = fixture.run_ctx.nodes.context.createScope(branch);

    REQUIRE(fixture.run_ctx.nodes.context.parentScope(branchScope) == rootScope);
}

TEST(NodeContext_GlobalResolveRoot, RuntimeFixture,
    "Глобальный resolve должен сообщать неоднозначность root scope.")
{
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    REQUIRE(fixture.run_ctx.nodes.context.resolveInfo(role).state == ContextResolutionState::Ambiguous);
}

TEST(NodeContext_ActiveOverlay, RuntimeFixture,
    "Активный scope должен перекрывать root scope.")
{
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const NodeId branchA = fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(branch);
    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    fixture.run_ctx.nodes.context.set(scope, role, branchA);
    fixture.run_ctx.nodes.context.activate(scope);

    REQUIRE(fixture.run_ctx.nodes.context.resolve(role) == branchA);
}

TEST(NodeContext_ActivateTwice, RuntimeFixture,
    "Повторная активация одного scope не должна дублировать его.")
{
    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(branch);

    fixture.run_ctx.nodes.context.activate(scope);
    fixture.run_ctx.nodes.context.activate(scope);

    REQUIRE(fixture.run_ctx.nodes.context.activeScopes().size() == 1);
    REQUIRE(fixture.run_ctx.nodes.context.activeScopes()[0] == scope);
}

TEST(NodeContext_FactoryAccumulatesCandidates, RuntimeFixture,
    "Создание одинаковых компонентов должно сохранять полный набор кандидатов.")
{
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "2");

    const ContextScopeId scope = fixture.run_ctx.nodes.context.findScope(fixture.root);
    const RoleId role = fixture.run_ctx.nodes.context.findRole("Component");

    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "3");

    const auto result = fixture.run_ctx.nodes.context.resolveInfo(scope, role);
    REQUIRE(result.state == ContextResolutionState::Ambiguous);
    REQUIRE(result.candidates.size() == 3);
}

TEST(NodeContext_MultipleActiveScopes, RuntimeFixture,
    "Несколько untyped scope могут быть активны одновременно.")
{
    auto& context = fixture.run_ctx.nodes.context;

    const NodeId a = fixture.run_ctx.nodes.factory.folder(fixture.root, "a");
    const NodeId b = fixture.run_ctx.nodes.factory.folder(fixture.root, "b");

    const ContextScopeId sa = context.createScope(a);
    const ContextScopeId sb = context.createScope(b);

    context.activate(sa);
    context.activate(sb);

    REQUIRE(context.activeScopes().size() == 2);
    REQUIRE(context.activeScopes()[0] == sa);
    REQUIRE(context.activeScopes()[1] == sb);
}

TEST(NodeContext_OverlayOrder, RuntimeFixture,
    "Последний активированный scope должен перекрывать предыдущие.")
{
    auto& context = fixture.run_ctx.nodes.context;

    const NodeId rootA = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root2");

    const NodeId a = fixture.run_ctx.nodes.factory.folder(fixture.root, "a");
    const NodeId a1 = fixture.run_ctx.nodes.factory.component(a, "Component", "1");
    fixture.run_ctx.nodes.factory.component(a, "Component", "2");

    const NodeId b = fixture.run_ctx.nodes.factory.folder(fixture.root, "b");
    const NodeId b1 = fixture.run_ctx.nodes.factory.component(b, "Component", "1");
    fixture.run_ctx.nodes.factory.component(b, "Component", "2");

    const RoleId role = context.findRole("Component");
    const ContextScopeId rootScope = context.findScope(fixture.root);
    const ContextScopeId sa = context.findScope(a);
    const ContextScopeId sb = context.findScope(b);

    context.set(rootScope, role, rootA);
    context.set(sa, role, a1);
    context.set(sb, role, b1);

    REQUIRE(context.resolve(role) == rootA);

    context.activate(sa);
    REQUIRE(context.resolve(role) == a1);

    context.activate(sb);
    REQUIRE(context.resolve(role) == b1);
}

TEST(NodeContext_DeactivateRestoresPreviousOverlay, RuntimeFixture,
    "Деактивация верхнего overlay должна восстановить предыдущий.")
{
    auto& context = fixture.run_ctx.nodes.context;

    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root2");

    const NodeId a = fixture.run_ctx.nodes.factory.folder(fixture.root, "a");
    const NodeId a1 = fixture.run_ctx.nodes.factory.component(a, "Component", "1");
    fixture.run_ctx.nodes.factory.component(a, "Component", "2");

    const NodeId b = fixture.run_ctx.nodes.factory.folder(fixture.root, "b");
    const NodeId b1 = fixture.run_ctx.nodes.factory.component(b, "Component", "1");
    fixture.run_ctx.nodes.factory.component(b, "Component", "2");

    const RoleId role = context.findRole("Component");
    const ContextScopeId sa = context.findScope(a);
    const ContextScopeId sb = context.findScope(b);

    context.set(sa, role, a1);
    context.set(sb, role, b1);

    context.activate(sa);
    context.activate(sb);
    context.deactivate(sb);

    REQUIRE(context.resolve(role) == a1);
}

TEST(NodeContext_DeactivateRestoresRoot, RuntimeFixture,
    "После деактивации последнего overlay должен снова использоваться root scope.")
{
    auto& context = fixture.run_ctx.nodes.context;

    const NodeId rootA = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root2");

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const NodeId branchA = fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const RoleId role = context.findRole("Component");
    const ContextScopeId rootScope = context.findScope(fixture.root);
    const ContextScopeId scope = context.findScope(branch);

    context.set(rootScope, role, rootA);
    context.set(scope, role, branchA);

    context.activate(scope);
    context.deactivate(scope);

    REQUIRE(context.resolve(role) == rootA);
}

TEST(NodeContext_ActiveSelectionOverridesPrevious, RuntimeFixture,
    "Явный выбор в активном overlay должен перекрывать root scope.")
{
    auto& context = fixture.run_ctx.nodes.context;

    const NodeId rootA = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root2");

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const NodeId branchA = fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const ContextScopeId rootScope = context.findScope(fixture.root);
    const ContextScopeId scope = context.findScope(branch);
    const RoleId role = context.findRole("Component");

    context.set(rootScope, role, rootA);
    context.set(scope, role, branchA);
    context.activate(scope);

    REQUIRE(context.resolve(role) == branchA);
}

TEST(NodeContext_DeactivateSelectionRestoresRoot, RuntimeFixture,
    "После деактивации выбранного overlay должно восстановиться значение root scope.")
{
    auto& context = fixture.run_ctx.nodes.context;

    const NodeId rootA = fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root1");
    fixture.run_ctx.nodes.factory.component(fixture.root, "Component", "root2");

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const NodeId branchA = fixture.run_ctx.nodes.factory.component(branch, "Component", "1");
    fixture.run_ctx.nodes.factory.component(branch, "Component", "2");

    const ContextScopeId rootScope = context.findScope(fixture.root);
    const ContextScopeId scope = context.findScope(branch);
    const RoleId role = context.findRole("Component");

    context.set(rootScope, role, rootA);
    context.set(scope, role, branchA);
    context.activate(scope);

    REQUIRE(context.resolve(role) == branchA);

    context.deactivate(scope);

    REQUIRE(context.resolve(role) == rootA);
}

TEST(NodeContext_DeactivateInactiveDoesNothing, RuntimeFixture,
    "Деактивация неактивного scope не должна менять состояние.")
{
    auto& context = fixture.run_ctx.nodes.context;

    const NodeId branch = fixture.run_ctx.nodes.factory.folder(fixture.root, "branch");
    const ContextScopeId scope = context.createScope(branch);

    context.deactivate(scope);

    REQUIRE(context.activeScopes().empty());
}

}
