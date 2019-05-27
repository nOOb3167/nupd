node('fedora && linux && 64bit') {
	stage('stage0') {
		sh '[ -f /etc/fedora-release ]'
		def extraCMakeArgs = '-DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-mingw64.cmake'
		dir("src") {
			git 'https://github.com/nOOb3167/nupd.git'
		}
		cmakeBuild buildDir: 'bld', buildType: 'Release', installation: 'MinGW64', sourceDir: 'src', steps: [[withCmake: true]], cmakeArgs: extraCMakeArgs
	}
}
