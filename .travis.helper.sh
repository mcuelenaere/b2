#!/bin/bash -x

case "$1" in
	before_install)
		case "${TRAVIS_OS_NAME}" in
			linux)
				sudo add-apt-repository "deb mirror://mirrors.ubuntu.com/mirrors.txt trusty main restricted universe multiverse" -y
				sudo apt-get update -q
				;;
			osx)
				brew update
				brew tap homebrew/dupes
				brew tap homebrew/php
				;;
		esac
	;;
	install)
		case "${TRAVIS_OS_NAME}" in
			linux)
				sudo apt-get install -y cmake llvm llvm-dev php5-cli php5-dev bison flex
			;;
			osx)
				brew install --verbose cmake llvm bison php55
			;;
		esac
	;;
	before_script)
		case "${TRAVIS_OS_NAME}" in
			osx)
				CMAKE_ARGS="-DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison -DLLVM_CONFIG_EXECUTABLE=/usr/local/opt/llvm/bin/llvm-config"
			;;
			*)
				CMAKE_ARGS=""
			;;
		esac

		mkdir build && cd build
		cmake .. ${CMAKE_ARGS}
		make
	;;
esac
