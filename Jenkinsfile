pipeline {
	agent none
	stages {
		stage('stage0') {
			agent { label 'fedora && linux && 64bit' }
			steps {
				checkout scm
				script {
					sh '[ -f /etc/fedora-release ]'
					def extraCMakeArgs = '-DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-mingw64.cmake'
					cmakeBuild buildDir: 'bld', buildType: 'Release', installation: 'MinGW64', sourceDir: '.', steps: [[withCmake: true]], cmakeArgs: "${extraCMakeArgs}"
					dir("bld") {
						sh 'cp --target-directory=. /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll'
						//sh 'tar -czvf test0.tgz test0.exe *.dll'
						//zip dir: '', zipFile: 'test0.zip', glob: 'test0.exe , *.dll'
					}
					dir("sta") {
						deleteDir()
						zip dir: '../bld', zipFile: 'test0.zip', glob: 'test0.exe , *.dll'
						stash includes: 'test0.zip', name: 'MinGWBuildStash'
						archiveArtifacts 'test0.zip'
					}
				}
			}
		}
		stage('stage1') {
			agent { label 'windows && 64bit' }
			options {
				skipDefaultCheckout true
			}
			steps {
				dir("sta") {
					deleteDir()
					unstash 'MinGWBuildStash'
					unzip dir: '', zipFile: 'test0.zip', glob: ''
					bat label: '', script: 'test0.exe --output_format=XML --log_level=test_suite --report_level=no > test0.xml'
				}
			}
			post {
				always {
					dir("sta") {
						xunit([BoostTest(deleteOutputFiles: true, failIfNotNew: true, pattern: 'test0.xml', skipNoTestFiles: false, stopProcessingIfError: true)])
					}
				}
			}
		}
	}
}
