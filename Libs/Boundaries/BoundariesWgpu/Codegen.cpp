#include "Codegen.hpp"

#include <format>

#include <Lattice/Tools/Exception.hpp>

namespace Boundaries::Wgpu {

namespace {

using Error = Lattice::Exception<Program>;

/// WGSL of a primitive's distance in local coordinates `l` with parameters `q`.
std::string primitive(Op op, const std::string& l, const std::string& q) {
    switch (op) {
    case Op::Sphere: return std::format("length({}) - {}[0].x", l, q);
    case Op::Box: return std::format("sdBox({}, {}).w", l, q);
    case Op::Capsule: return std::format("sdCapsule({}, {}).w", l, q);
    case Op::Cylinder: return std::format("sdCylinder({}, {}).w", l, q);
    case Op::HalfSpace: return std::format("sdHalfSpace({}, {}).w", l, q);
    case Op::InfiniteCylinder: return std::format("sdInfiniteCylinder({}, {}).w", l, q);
    default: throw Error("operation {} is not a primitive", uint32_t(op));
    }
}

}

std::string generateDistance(const Program& program, std::string_view name) {
    if (program.code.empty())
        throw Error("geometry program is empty");
    std::string out = std::format("fn {}(p: vec3<f32>) -> f32 {{\n    let h = vec4<f32>(p, 1.0);\n", name);
    std::vector<std::string> stack;
    for (size_t i = 0; i < program.code.size(); ++i) {
        const Op op = program.code[i].op;
        const std::string value = std::format("d{}", i);
        const std::string instruction = std::format("geometryCode[{}u]", i);
        if (isPrimitive(op)) {
            out += std::format("    let i{0} = {1};\n    let {2} = i{0}.scale * ({3});\n", i, instruction, value,
                primitive(op, std::format("geometryLocal(i{}, h)", i), std::format("i{}.params", i)));
        } else if (isBinary(op)) {
            if (stack.size() < 2)
                throw Error("instruction {} needs two operands", i);
            const std::string b = stack.back();
            stack.pop_back();
            const std::string a = stack.back();
            stack.pop_back();
            std::string expression;
            switch (op) {
            case Op::Union: expression = std::format("min({}, {})", a, b); break;
            case Op::Intersect: expression = std::format("max({}, {})", a, b); break;
            case Op::Subtract: expression = std::format("max({}, -{})", a, b); break;
            default: expression = std::format("geometrySmoothUnion({}, {}, {}.params[0].x)", a, b, instruction); break;
            }
            out += std::format("    let {} = {};\n", value, expression);
        } else {
            if (stack.empty())
                throw Error("instruction {} needs an operand", i);
            const std::string a = stack.back();
            stack.pop_back();
            const std::string expression = op == Op::Round
                ? std::format("{} - {}.params[0].x", a, instruction)
                : std::format("abs({}) - {}.params[0].x", a, instruction);
            out += std::format("    let {} = {};\n", value, expression);
        }
        stack.push_back(value);
    }
    if (stack.size() != 1)
        throw Error("geometry program leaves {} values instead of one", stack.size());
    out += std::format("    return -{};\n}}\n", stack.back());
    return out;
}

std::string interpretedDistance(const Program&, std::string_view name) {
    return std::format("fn {}(p: vec3<f32>) -> f32 {{ return geometryDistance(p); }}\n", name);
}

std::vector<Op> structure(const Program& program) {
    std::vector<Op> ops;
    ops.reserve(program.code.size());
    for (const Instruction& instruction : program.code)
        ops.push_back(instruction.op);
    return ops;
}

}
