#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Tools/LogMode.hpp>


struct LogInvariantFixture : Lattice::TestFixture {};


namespace {

constexpr Level kLevels[] = {
    Level::Message,
    Level::Ok,
    Level::Action,
    Level::Info,
    Level::Warning,
    Level::Error,
    Level::Exception
};

constexpr LogMode kFlags[] = {
    LogMode::Verbose,
    LogMode::OnlyWarn,
    LogMode::Clean,
    LogMode::SuppressError
};

using LogModes::KeepContext;
using LogModes::shouldKeep;
using LogModes::UnlimitedDepth;

KeepContext withLevel(KeepContext ctx, Level level) {
    ctx.level = level;
    return ctx;
}

bool keep(const KeepContext& ctx) {
    return shouldKeep(ctx);
}

LogMode without(LogMode mode, LogMode flag) {
    return mode & ~flag;
}

}

TEST(LogMode_DefaultIsCleanOnlyWarn, LogInvariantFixture,
    "Дефолтный режим — Clean | OnlyWarn.")
{
    REQUIRE(LogModes::Default == (LogMode::Clean | LogMode::OnlyWarn));
}

TEST(LogMode_OrIsCommutative, LogInvariantFixture,
    "Комбинация флагов коммутативна: порядок | не меняет shouldKeep.")
{
    const KeepContext bases[] = {
        {},
        {.success = false},
        {.depth = 3, .maxDepth = 2},
        {.isCurrentFinal = true, .isScopeFinal = true},
        {.isScopeFinal = true, .hadProblem = true},
        {.success = false, .hadProblem = true}
    };

    for (LogMode a : kFlags) {
        for (LogMode b : kFlags) {
            const LogMode ab = a | b;
            const LogMode ba = b | a;
            REQUIRE(ab == ba);

            for (const auto& base : bases) {
                for (Level level : kLevels) {
                    KeepContext ctx = withLevel(base, level);
                    ctx.mode = ab;
                    const bool left = keep(ctx);
                    ctx.mode = ba;
                    CHECK(left == keep(ctx));
                }
            }
        }
    }
}

TEST(LogMode_SuppressErrorIndependent, LogInvariantFixture,
    "SuppressError применяется независимо от остальных режимов.")
{
    const KeepContext bases[] = {
        {},
        {.success = false},
        {.depth = 4, .maxDepth = 1},
        {.isCurrentFinal = true, .isScopeFinal = true},
        {.success = false, .isCurrentFinal = true, .isScopeFinal = true},
        {.isScopeFinal = true, .hadProblem = true}
    };

    for (LogMode a : kFlags) {
        for (LogMode b : kFlags) {
            const LogMode mode = a | b;

            for (const auto& base : bases) {
                for (Level level : kLevels) {
                    KeepContext with = withLevel(base, level);
                    with.mode = mode | LogMode::SuppressError;

                    if (LogModes::isError(level)) {
                        CHECK(!keep(with));
                        continue;
                    }

                    KeepContext withoutErr = with;
                    withoutErr.mode = without(mode, LogMode::SuppressError);
                    CHECK(keep(with) == keep(withoutErr));
                }
            }
        }
    }
}

TEST(LogMode_VerboseIgnoresMaxDepth, LogInvariantFixture,
    "Verbose игнорирует maxDepth.")
{
    for (Level level : kLevels) {
        const bool unlimited = keep({
            .mode = LogMode::Verbose,
            .level = level,
            .depth = 8,
            .maxDepth = UnlimitedDepth
        });

        CHECK(keep({
            .mode = LogMode::Verbose,
            .level = level,
            .depth = 8,
            .maxDepth = 1
        }) == unlimited);

        CHECK(keep({
            .mode = LogMode::Verbose | LogMode::OnlyWarn,
            .level = level,
            .depth = 8,
            .maxDepth = 1
        }) == keep({
            .mode = LogMode::Verbose | LogMode::OnlyWarn,
            .level = level,
            .depth = 1,
            .maxDepth = UnlimitedDepth
        }));
    }
}

TEST(LogMode_MaxDepthSkipsCurrentFinal, LogInvariantFixture,
    "maxDepth применяется к Action и Warning, но не к итогу текущего scope.")
{
    const LogMode mode = LogMode::Clean | LogMode::OnlyWarn;

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Ok,
        .depth = 8,
        .maxDepth = 1,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Exception,
        .success = false,
        .depth = 8,
        .maxDepth = 1,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));

    REQUIRE(!keep({
        .mode = mode,
        .level = Level::Action,
        .depth = 2,
        .maxDepth = 2,
        .hadProblem = true
    }));

    REQUIRE(!keep({
        .mode = mode,
        .level = Level::Warning,
        .depth = 2,
        .maxDepth = 2
    }));

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Action,
        .depth = 1,
        .maxDepth = 2,
        .hadProblem = true
    }));

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Warning,
        .depth = 1,
        .maxDepth = 2
    }));

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Error,
        .depth = 8,
        .maxDepth = 1
    }));
}

TEST(LogMode_CleanDropsInternals, LogInvariantFixture,
    "Clean без других флагов оставляет только итог scope.")
{
    for (Level level : kLevels) {
        CHECK(!keep({
            .mode = LogMode::Clean,
            .level = level
        }));
    }

    REQUIRE(keep({
        .mode = LogMode::Clean,
        .level = Level::Ok,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));

    REQUIRE(!keep({
        .mode = LogMode::Clean,
        .level = Level::Error,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));

    REQUIRE(keep({
        .mode = LogMode::Clean,
        .level = Level::Exception,
        .success = false,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));

    REQUIRE(keep({
        .mode = LogMode::Clean,
        .level = Level::Error,
        .success = false,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));
}

TEST(LogMode_CleanSuppressErrorOnlyOk, LogInvariantFixture,
    "Clean | SuppressError оставляет только Ok; Error и Exception удаляются.")
{
    const LogMode mode = LogMode::Clean | LogMode::SuppressError;

    for (Level level : kLevels) {
        CHECK(!keep({.mode = mode, .level = level}));
        CHECK(!keep({
            .mode = mode,
            .level = level,
            .success = false,
            .isCurrentFinal = true,
            .isScopeFinal = true
        }));
    }

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Ok,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));
}

TEST(LogMode_OnlyWarnSuppressError, LogInvariantFixture,
    "OnlyWarn | SuppressError оставляет Action, Warning, Ok; ошибки удаляет.")
{
    const LogMode mode = LogMode::OnlyWarn | LogMode::SuppressError;

    REQUIRE(keep({.mode = mode, .level = Level::Action}));
    REQUIRE(keep({.mode = mode, .level = Level::Warning}));
    REQUIRE(keep({.mode = mode, .level = Level::Ok}));
    REQUIRE(!keep({.mode = mode, .level = Level::Info}));
    REQUIRE(!keep({.mode = mode, .level = Level::Message}));
    REQUIRE(!keep({.mode = mode, .level = Level::Error}));
    REQUIRE(!keep({.mode = mode, .level = Level::Exception}));

    REQUIRE(!keep({
        .mode = mode,
        .level = Level::Error,
        .success = false,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));
}

TEST(LogMode_CleanOnlyWarnSuppressErrorRespectsDepth, LogInvariantFixture,
    "Clean | OnlyWarn | SuppressError оставляет Action, Warning, Ok с учётом maxDepth.")
{
    const LogMode mode =
        LogMode::Clean | LogMode::OnlyWarn | LogMode::SuppressError;

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Action,
        .depth = 1,
        .maxDepth = 2,
        .hadProblem = true
    }));
    REQUIRE(keep({
        .mode = mode,
        .level = Level::Warning,
        .depth = 1,
        .maxDepth = 2
    }));
    REQUIRE(keep({
        .mode = mode,
        .level = Level::Ok,
        .depth = 1,
        .maxDepth = 2,
        .isScopeFinal = true
    }));

    REQUIRE(!keep({
        .mode = mode,
        .level = Level::Action,
        .depth = 2,
        .maxDepth = 2,
        .hadProblem = true
    }));
    REQUIRE(!keep({
        .mode = mode,
        .level = Level::Warning,
        .depth = 2,
        .maxDepth = 2
    }));
    REQUIRE(!keep({
        .mode = mode,
        .level = Level::Ok,
        .depth = 2,
        .maxDepth = 2,
        .isScopeFinal = true
    }));

    REQUIRE(!keep({.mode = mode, .level = Level::Error}));
    REQUIRE(!keep({.mode = mode, .level = Level::Exception}));
    REQUIRE(!keep({.mode = mode, .level = Level::Ok}));
    REQUIRE(!keep({.mode = mode, .level = Level::Info}));

    REQUIRE(keep({
        .mode = mode,
        .level = Level::Ok,
        .depth = 8,
        .maxDepth = 1,
        .isCurrentFinal = true,
        .isScopeFinal = true
    }));
}

TEST(LogMode_VerboseSuppressError, LogInvariantFixture,
    "Verbose | SuppressError оставляет всё, кроме Error и Exception.")
{
    const LogMode mode = LogMode::Verbose | LogMode::SuppressError;

    REQUIRE(keep({.mode = mode, .level = Level::Message}));
    REQUIRE(keep({.mode = mode, .level = Level::Ok}));
    REQUIRE(keep({.mode = mode, .level = Level::Action}));
    REQUIRE(keep({.mode = mode, .level = Level::Info}));
    REQUIRE(keep({.mode = mode, .level = Level::Warning}));
    REQUIRE(!keep({.mode = mode, .level = Level::Error}));
    REQUIRE(!keep({.mode = mode, .level = Level::Exception}));
}

TEST(LogMode_VerboseOverridesClean, LogInvariantFixture,
    "Verbose перекрывает Clean и оставляет внутренние сообщения.")
{
    REQUIRE(!keep({.mode = LogMode::Clean, .level = Level::Info}));
    REQUIRE(keep({
        .mode = LogMode::Verbose | LogMode::Clean,
        .level = Level::Info
    }));
    REQUIRE(keep({
        .mode = LogMode::Verbose | LogMode::Clean,
        .level = Level::Action,
        .depth = 8,
        .maxDepth = 1
    }));
}

TEST(LogMode_CleanOnlyWarnKeepsActionOnProblem, LogInvariantFixture,
    "Clean | OnlyWarn оставляет Action только при проблеме или ошибке scope.")
{
    const LogMode mode = LogMode::Clean | LogMode::OnlyWarn;

    REQUIRE(!keep({.mode = mode, .level = Level::Action}));
    REQUIRE(keep({
        .mode = mode,
        .level = Level::Action,
        .hadProblem = true
    }));
    REQUIRE(keep({
        .mode = mode,
        .level = Level::Action,
        .success = false
    }));

    REQUIRE(!keep({.mode = mode, .level = Level::Ok}));
    REQUIRE(keep({
        .mode = mode,
        .level = Level::Ok,
        .isScopeFinal = true
    }));
    REQUIRE(keep({.mode = mode, .level = Level::Warning}));
    REQUIRE(!keep({.mode = mode, .level = Level::Info}));
}

TEST(LogMode_AnnotateWarnInClean, LogInvariantFixture,
    "Clean оставляет метку (warn) на успешном итоге, если внутри был warn/error.")
{
    using LogModes::shouldAnnotateWarn;

    REQUIRE(shouldAnnotateWarn(LogMode::Clean, true, true));
    REQUIRE(!shouldAnnotateWarn(LogMode::Clean, true, false));
    REQUIRE(!shouldAnnotateWarn(LogMode::Clean, false, true));
}

TEST(LogMode_AnnotateWarnSuppressed, LogInvariantFixture,
    "SuppressError в Clean убирает даже метку (warn).")
{
    using LogModes::shouldAnnotateWarn;

    REQUIRE(!shouldAnnotateWarn(
        LogMode::Clean | LogMode::SuppressError,
        true,
        true
    ));
}

TEST(LogMode_AnnotateWarnNotInVerboseOrOnlyWarn, LogInvariantFixture,
    "Verbose и OnlyWarn не ставят (warn): внутренности и так видны.")
{
    using LogModes::shouldAnnotateWarn;

    REQUIRE(!shouldAnnotateWarn(LogMode::Verbose, true, true));
    REQUIRE(!shouldAnnotateWarn(LogMode::OnlyWarn, true, true));
    REQUIRE(!shouldAnnotateWarn(
        LogMode::Clean | LogMode::OnlyWarn,
        true,
        true
    ));
    REQUIRE(!shouldAnnotateWarn(
        LogMode::Verbose | LogMode::Clean,
        true,
        true
    ));
}

TEST(LogMode_InheritVerboseOverridesQuiet, LogInvariantFixture,
    "Verbose от родителя перекрывает Clean | SuppressError и показывает всё.")
{
    using LogModes::inherit;

    const LogMode tests = LogMode::Clean | LogMode::SuppressError;

    REQUIRE(inherit(tests, LogMode::Verbose) == LogMode::Verbose);
    REQUIRE(
        inherit(tests, LogMode::Verbose | LogMode::OnlyWarn) ==
        (LogMode::Verbose | LogMode::OnlyWarn)
    );
}

TEST(LogMode_InheritSuppressErrorBlocksOnlyWarn, LogInvariantFixture,
    "SuppressError не наследует OnlyWarn: тесты остаются тихими.")
{
    using LogModes::inherit;

    const LogMode tests = LogMode::Clean | LogMode::SuppressError;

    REQUIRE(inherit(tests, LogModes::Default) == tests);
    REQUIRE(inherit(tests, LogMode::OnlyWarn) == tests);
    REQUIRE(inherit(tests, LogMode::Clean | LogMode::OnlyWarn) == tests);
}

TEST(LogMode_InheritOnlyWarnWithoutSuppress, LogInvariantFixture,
    "Без SuppressError OnlyWarn наследуется от родителя.")
{
    using LogModes::inherit;

    REQUIRE(
        inherit(LogMode::Clean, LogMode::OnlyWarn) ==
        (LogMode::Clean | LogMode::OnlyWarn)
    );
}
