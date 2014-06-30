#!/usr/bin/env python

import os
import re
import sys

from distutils.spawn import find_executable
from itertools import chain
from glob import iglob
from shutil import rmtree
from subprocess import check_output, CalledProcessError, STDOUT
from tempfile import mkdtemp
from unittest import TestSuite, TextTestRunner, TestCase


def read_file(filename):
    with open(filename, "r") as f:
        return f.read()


def write_file(filename, contents):
    with open(filename, "w") as f:
        f.write(contents)


class AbstractTestCase(TestCase):
    def __init__(self, filename):
        super(AbstractTestCase, self).__init__(methodName='test')

        self.filename = filename
        self.maxDiff = None

    def __str__(self):
        return "%s (%s.%s)" % (self.filename, self.__class__.__module__, self.__class__.__name__)

    def _parse_file(self):
        self.parts = {
            'FILES': {},
            'ARGUMENTS': [],
            'EXPECTED_RETCODE': 0
        }

        file_contents = read_file(self.filename)
        parts = re.split(r"^--([^-]+)--\n", file_contents, flags=re.MULTILINE)
        for i in range(1, len(parts), 2):
            name, contents = parts[i], parts[i+1]

            if name == "TEMPLATE":
                self.parts['FILES']['main.tpl'] = contents
            elif name.startswith("FILE["):
                filename = re.match(r"FILE\[([^\[]+)\]", name).group(1)
                self.parts['FILES'][filename] = contents
            elif name == "ARGUMENTS":
                self.parts['ARGUMENTS'] = contents.strip().split(" ")
            else:
                self.parts[name] = contents

    def _setup_tempdir_and_extract_files(self):
        class Resource():
            temp_dir = None

            def __init__(self, files):
                self.files = files

            def __enter__(self):
                # create temporary directory
                self.temp_dir = mkdtemp()

                # setup files
                for filename, contents in self.files.iteritems():
                    write_file(os.path.join(self.temp_dir, filename), contents)

                return self.temp_dir

            def __exit__(self, exc_type, exc_val, exc_tb):
                # remove temporary directory including all its contents
                rmtree(self.temp_dir)
                self.temp_dir = None

        return Resource(self.parts['FILES'])

    def _assert_process_output_as_expected(self, args, allowSkippingTests=True):
        try:
            result = check_output(args, stderr=STDOUT)
        except CalledProcessError as e:
            assert_msg = "Command %s returned exit status %d, expected %d. Output: %s" % (e.cmd, e.returncode, self.parts['EXPECTED_RETCODE'], e.output)
            self.assertEqual(self.parts['EXPECTED_RETCODE'], e.returncode, msg=assert_msg)

        if allowSkippingTests and result.strip().startswith("SKIP_TEST"):
            self.skipTest(result.strip().replace("SKIP_TEST: ", ""))

        self.assertMultiLineEqual(self.parts['EXPECTED'], result)

    def test(self):
        raise NotImplementedError()


class ParserTestCase(AbstractTestCase):
    def __init__(self, filename, ast_print):
        super(ParserTestCase, self).__init__(filename)
        self.ast_print = ast_print

    def test(self):
        self._parse_file()

        self.assertIn('EXPECTED', self.parts)
        self.assertIn('main.tpl', self.parts['FILES'])

        with self._setup_tempdir_and_extract_files() as temp_dir:
            self._assert_process_output_as_expected([self.ast_print, "-t", temp_dir] + self.parts['ARGUMENTS'] + [os.path.join(temp_dir, "main.tpl")])


class PHPBindingTestCase(AbstractTestCase):
    def __init__(self, filename, php_binary, php_extension):
        super(PHPBindingTestCase, self).__init__(filename)
        self.php_binary = php_binary
        self.php_extension = php_extension

    def test(self):
        self._parse_file()

        self.assertIn('EXPECTED', self.parts)
        self.assertIn('main.tpl', self.parts['FILES'])
        self.assertIn('main.php', self.parts['FILES'])

        with self._setup_tempdir_and_extract_files() as temp_dir:
            self._assert_process_output_as_expected([self.php_binary, "-d", "extension=" + self.php_extension, "-f", os.path.join(temp_dir, "main.php")] + self.parts['ARGUMENTS'])


class JSPrecompilerTestCase(AbstractTestCase):
    def __init__(self, filename, js_precompiler):
        super(JSPrecompilerTestCase, self).__init__(filename)
        self.js_precompiler = js_precompiler

    def test(self):
        self._parse_file()

        self.assertIn('EXPECTED', self.parts)
        self.assertIn('main.tpl', self.parts['FILES'])

        with self._setup_tempdir_and_extract_files() as temp_dir:
            self._assert_process_output_as_expected([self.js_precompiler, "-t", temp_dir] + self.parts['ARGUMENTS'] + [os.path.join(temp_dir, "main.tpl")])


def find_file(filenames, directory):
    for filename in filenames:
        path = os.path.join(directory, filename)
        if os.path.isfile(path):
            return path

    raise RuntimeError('Couldn\'t find any of %s in "%s"!' % (str(filenames), directory))


def create_parser_testcases(build_path):
    ast_print = find_executable('ast_print', os.path.join(build_path, 'src', 'ast_print'))
    for filename in iglob(os.path.join(os.path.dirname(__file__), "parser", "*.test")):
        yield ParserTestCase(filename, ast_print)


def create_php_testcases(build_path):
    php_extension = find_file(('libb2_php.dylib', 'libb2_php.so', 'b2_php.dll'), os.path.join(build_path, 'src', 'php'))
    php_binary = find_executable('php')
    for filename in iglob(os.path.join(os.path.dirname(__file__), "php_binding", "*.test")):
        yield PHPBindingTestCase(filename, php_extension=php_extension, php_binary=php_binary)


def create_js_precompiler_testcases(build_path):
    js_precompiler = find_executable('b2-js-precompiler', os.path.join(build_path, 'src', 'js_precompiler'))
    for filename in iglob(os.path.join(os.path.dirname(__file__), "js_precompiler", "*.test")):
        yield JSPrecompilerTestCase(filename, js_precompiler)


def main(args):
    if len(args) != 1:
        raise RuntimeError("usage: runner.py <build_path>")

    build_path = args[0]

    suite = TestSuite()
    for testcase in chain(create_parser_testcases(build_path), create_php_testcases(build_path), create_js_precompiler_testcases(build_path)):
        suite.addTest(testcase)

    runner = TextTestRunner()
    result = runner.run(suite)

    return 0 if result.wasSuccessful() else 1

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
