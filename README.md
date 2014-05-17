## About

b² is a [jinja](http://jinja.pocoo.org/)-inspired template engine which uses [LLVM](http://llvm.org/) to generate native code.

## Building the code

### Mac OS X

Requirements:
 - `brew install cmake`
 - `brew install llvm`
 - `brew install bison`

Build steps:

```bash
mkdir build && cd build
cmake .. -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison -DLLVM_CONFIG_EXECUTABLE=/usr/local/opt/llvm/bin/llvm-config
make
make install
```

### Linux

Requirements:
 - `aptitude install cmake`
 - `aptitude install llvm`

TODO

## Running tests

```bash
tests/runner.py build/
```

### Generate Xcode project

```bash
mkdir xcode && cd xcode
cmake .. -G Xcode
open b2.xcodeproj/
```

### Unofficial Homebrew formula

```ruby
require 'formula'

class B2 < Formula
  head 'https://github.com/mcuelenaere/b2'
  version '0.0.1'

  option 'with-php-bindings', 'Enable PHP bindings.'

  depends_on 'cmake' => :build
  depends_on 'bison' => :build

  # Standard packages
  depends_on 'llvm'
  depends_on 'php55' if build.with? 'php-bindings'

  def install
    args = [
      "-DBISON_EXECUTABLE=#{Formula['bison'].opt_prefix}/bin/bison",
      "-DLLVM_CONFIG_EXECUTABLE=#{Formula['llvm'].opt_prefix}/bin/llvm-config"
    ] + std_cmake_args

    if build.with? 'php-bindings'
      args << "-DPHP_CONFIG_EXECUTABLE=#{Formula['php55'].opt_prefix}/bin/php-config"
    end

    mkdir "build"
    cd "build" do
      system "cmake", "..", *args
      system "make"
      system "make install"
    end

    if build.with? 'php-bindings'
      # TODO: install php.ini
    end
  end
end
```
