node('fedora && linux && 64bit') {
	stage('stage0') {
		sh '[ -f /etc/fedora-release ]'
		def extraCMakeArgs = '-DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-mingw64.cmake'
		dir("src") {
			git 'https://github.com/nOOb3167/nupd.git'
		}
		cmakeBuild buildDir: 'bld', buildType: 'Release', installation: 'MinGW64', sourceDir: 'src', steps: [[withCmake: true]], cmakeArgs: "${extraCMakeArgs}"
		dir("bld") {
			sh 'cp --target-directory=. /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll'
			sh 'tar -czvf test0.tgz test0.exe *.dll'
			archiveArtifacts 'test0.tgz'
		}
	}
}
