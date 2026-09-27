#include <Lattice/Tools/Fixture.hpp>
#include <Lattice/Tools/Tests.hpp>
#include <Lattice/Tools/TextFormatter.hpp>
#include <Lattice/Tools/TextPattern.hpp>
#include <Lattice/Tools/Logger.hpp>

#include <vector>

namespace Lattice {

TEST(TextFormatter_RgbMarkupAndTrueColorOutput, TestFixture,
    "Hex tags should produce true-color ANSI output and survive markup conversion.") {
    const TextFormatter text("<#7aa2f7>title</>");

    REQUIRE(text.markup() == "<#7aa2f7>title</>");
    REQUIRE(text.render() == "\033[38;2;122;162;247mtitle\033[0m");
}

TEST(TextFormatter_StyleParameterCodec, RuntimeFixture,
    "TextStyle should remain an ordinary parameter through ParamAdapter.") {
    TextStyle style = TextStyle::Bold | TextStyle::rgb(0x7aa2f7);
    const ExportId id = fixture.build(fixture.root).param("style", style);

    REQUIRE(fixture.run_ctx.nodes.exports.value(id).get<std::string>() == "#7aa2f7 bold");
    fixture.run_ctx.nodes.exports.set(id, Value{"#f7768e dim"});
    REQUIRE(style == (TextStyle::Dim | TextStyle::rgb(0xf7768e)));
}

TEST(TextFormatter_SemanticThemeTags, TestFixture,
    "Semantic tags should resolve through the supplied theme.") {
    TextTheme theme = TextTheme::defaults();
    TextFormatter text("<h>Header</>", theme);

    REQUIRE(text.markup() == "<b><#7aa2f7>Header<//>");

    theme.style(theme.find("h")) = TextStyle::rgb(0x00ff00);
    text.parse("<h>Header</>", &theme);
    REQUIRE(text.markup() == "<#00ff00>Header</>");
}

TEST(TextFormatter_UsesSystemThemeByDefault, TestFixture,
    "Semantic tags should have immutable defaults when no CLI theme is supplied.") {
    REQUIRE(TextFormatter("<h>Header</>").markup() == "<b><#7aa2f7>Header<//>");
    REQUIRE(TextFormatter("<a>Accent</>").markup() == "<#bb9af7>Accent</>");
    REQUIRE(TextFormatter("<ok>Success</>").markup() == "<#28d08a>Success</>");
    REQUIRE(TextFormatter("<wrn>Warning</>").markup() == "<#ffff55>Warning</>");
    REQUIRE(TextFormatter("<err>Error</>").markup() == "<#da6a6a>Error</>");
    REQUIRE(TextFormatter("<mut>Secondary</>").markup() == "<#aaaaaa>Secondary</>");
    REQUIRE(TextFormatter("<mut2>Metadata</>").markup() == "<#555555>Metadata</>");
}

TEST(TextPattern_ResolvesNamesOnceAndReadsCurrentStyle, TestFixture,
    "TextPattern should retain semantic ids while observing changed theme values.") {
    TextTheme theme = TextTheme::defaults();
    TextPattern pattern("<b><h>Header<//>", theme);

    REQUIRE(pattern.plain() == "Header");
    REQUIRE(pattern.format().markup() == "<b><#7aa2f7>Header<//>");

    theme.style(theme.find("h")) = TextStyle::rgb(0x00ff00);
    REQUIRE(pattern.format().markup() == "<b><#00ff00>Header<//>");
}

TEST(TextFormatter_UnclosedTagDoesNotAbortFormatting, TestFixture,
    "Malformed markup should be rendered and diagnosed instead of throwing.") {
    const TextFormatter text("first line\n<h>unclosed");

    REQUIRE(text.plain() == "first line\nunclosed");
    REQUIRE(text.markup() == "first line\n<b><#7aa2f7>unclosed<//>");
    REQUIRE(text.diagnostics().size() == 1);
    REQUIRE(text.diagnostics().front().lineNumber == 2);
    REQUIRE(text.diagnostics().front().column == 1);
    REQUIRE(text.diagnostics().front().line == "<h>unclosed");
}

TEST(TextFormatter_LoggerReportsDiagnostics, TestFixture,
    "Logger should turn formatter diagnostics into ordinary warning events.") {
    const TextFormatter text("<h>unclosed");
    std::vector<LogEvent> events;
    const auto sink = LogSystem::addSink([&events](const LogEvent& event) {
        events.push_back(event);
    });

    Logger::message(text);
    LogSystem::removeSink(sink);

    REQUIRE(events.size() == 2);
    REQUIRE(events[0].level == Level::Warning);
    REQUIRE(events[0].text.plain().find("TextFormatter at 1:1") != std::string::npos);
    REQUIRE(events[0].text.plain().find("<h>unclosed") != std::string::npos);
    REQUIRE(events[1].level == Level::Message);
}

}
