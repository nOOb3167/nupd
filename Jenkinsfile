pipeline {
	agent none
	stages {
		stage('win and nix build and test') {
			parallel {
				stage('win') {
					stages {
						stage('build') {
							agent { label 'fedora && linux && 64bit' }
							steps {
								script {
									sh '[ -f /etc/fedora-release ]'
									def extraCMakeArgs = '-DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-mingw64.cmake'
									dir("bld") {
										cmakeBuild buildDir: '.', buildType: 'Release', installation: 'MinGW64', sourceDir: '..', steps: [[withCmake: true]], cmakeArgs: "${extraCMakeArgs}"
										sh 'cp --target-directory=. /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll'
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
						stage('test') {
							agent { label 'windows && 64bit' }
							options { skipDefaultCheckout true }
							steps {
								dir("sta") {
									deleteDir()
									unstash 'MinGWBuildStash'
									unzip dir: '', zipFile: 'test0.zip', glob: ''
									bat label: '', script: 'test0.exe --output_format=XML --log_level=test_suite --report_level=no --result_code=no > test0.xml'
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
				stage('nix') {
					agent { label 'fedora && linux && 64bit' }
					stages {
						stage('build') {
							steps {
								script {
									sh '[ -f /etc/fedora-release ]'
									dir("bld2") {
										cmakeBuild buildDir: '.', buildType: 'Release', installation: 'MinGW64', sourceDir: '..', steps: [[withCmake: true]]
										archiveArtifacts 'test0'
									}
								}
							}
						}
						stage('test') {
							options { skipDefaultCheckout true }
							steps {
								dir("sta2") {
									deleteDir()
									sh 'cp --target-directory=. ../bld2/test0'
									sh label: '', script: 'test0 --output_format=XML --log_level=test_suite --report_level=no --result_code=no > test0.xml'
								}
							}
							post {
								always {
									dir("sta2") {
										xunit([BoostTest(deleteOutputFiles: true, failIfNotNew: true, pattern: 'test0.xml', skipNoTestFiles: false, stopProcessingIfError: true)])
									}
								}
							}
						}
					}
				}
			}
		}
	}
}
