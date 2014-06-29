## About

b² is a [jinja](http://jinja.pocoo.org/)-inspired template engine which uses [LLVM](http://llvm.org/) to generate native code.

## Building the code

### CMake options

 - `-DWITH_PHP_BINDINGS=OFF` disable PHP bindings (enabled by default)

### Mac OS X

Requirements:
 - `brew install cmake llvm bison`
 - `brew install php55` (for PHP bindings)

Build steps:

```bash
mkdir build && cd build
cmake .. -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison -DLLVM_CONFIG_EXECUTABLE=/usr/local/opt/llvm/bin/llvm-config
make
make install
```

### Linux

Requirements:
 - `apt-get install bison cmake flex llvm-dev`
 - `apt-get install php5-dev` (for PHP bindings)

```bash
mkdir build && cd build
cmake ..
make
make install
```

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
      args << "-DWITH_PHP_BINDINGS=ON -DPHP_CONFIG_EXECUTABLE=#{Formula['php55'].opt_prefix}/bin/php-config"
    else
      args << "-DWITH_PHP_BINDINGS=OFF"
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
