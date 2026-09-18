import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('generator', ROOT / 'tools/generate_plugin.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class GeneratorTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.plugin = self.root / 'plugin'
        self.plugin.mkdir()
        self.output = self.root / 'generated.cpp'
        self.flags = ['-std=c++20', '-I' + str(ROOT), '-I' + str(self.root)]

    def generate(self, code):
        (self.plugin / 'types.hpp').write_text('#include <Lattice/Kernel/Component.hpp>\n' + code)
        return generator.generate(self.plugin, self.output, self.flags)

    def test_graph_and_foreign_types(self):
        (self.root / 'foreign.hpp').write_text('''#pragma once
#include <Lattice/Kernel/Component.hpp>
namespace External { struct API : Lattice::Component {}; }
''')
        types = self.generate('''
#include "foreign.hpp"
namespace Local {
struct Helper {};
struct ZBase : Lattice::Component {};
using Alias = External::API;
struct Derived : ZBase, Alias {};
}
''')
        self.assertEqual(types, ['Local::ZBase', 'Local::Derived'])
        code = self.output.read_text()
        self.assertIn('types.add<::Local::Derived, ::Local::ZBase, ::External::API>', code)
        self.assertNotIn('types.add("External::API"', code)
        self.assertNotIn('Helper', code)
        timestamp = self.output.stat().st_mtime_ns
        generator.generate(self.plugin, self.output, self.flags)
        self.assertEqual(timestamp, self.output.stat().st_mtime_ns)

    def test_only_graph_registration(self):
        self.generate('namespace Test { struct Base : Lattice::Component {}; struct Child : Base {}; struct Helper {}; }')
        code = self.output.read_text()
        self.assertIn('plugin_register_blueprints(Lattice::Blueprints& types)', code)
        self.assertNotIn('plugin_register(', code)
        self.assertNotIn('plugin_shutdown', code)
        self.assertNotIn('Node.hpp', code)
        self.assertNotIn('Helper', code)

    def test_multiple_bases(self):
        self.generate('struct A : Lattice::Component {}; struct B : Lattice::Component {}; struct C : A, B {};')
        self.assertIn('types.add<::C, ::A, ::B>("C");', self.output.read_text())

    def test_private_marker_is_error(self):
        with self.assertRaisesRegex(ValueError, 'public'):
            self.generate('class Hidden : Lattice::Component {};')

    def test_templates_are_not_silently_registered(self):
        with self.assertRaisesRegex(ValueError, 'templates'):
            self.generate('template<class T> struct Generic : Lattice::Component {};')

    def test_invalid_cpp_preserves_previous_output(self):
        self.output.write_text('previous output')
        with self.assertRaises(ValueError):
            self.generate('this is invalid C++;')
        self.assertEqual(self.output.read_text(), 'previous output')


if __name__ == '__main__':
    unittest.main()
