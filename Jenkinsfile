def buildWinMinGW() {
    fileOperations([folderCreateOperation('bld'), folderDeleteOperation('sta'), folderCreateOperation('sta')])
    cmakeBuild(
            sourceDir: '.',
            buildDir: 'bld',
            buildType: 'RelWithDebInfo',
            installation: 'MinGW64',
            steps: [[withCmake: true]])
    sh("cp --target-directory=sta /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll")
    sh("cp --target-directory=sta /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll")
    sh("cp --target-directory=sta /usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll")
    sh("cp --target-directory=sta bld/test0.exe")
    fileOperations([fileZipOperation('sta')])
    stash(includes: 'sta.zip', name: 'MinGWBuildStash')
    archiveArtifacts('sta.zip')
}

def testWinMinGW() {
    fileOperations([folderDeleteOperation('sta')])    
	unstash 'MinGWBuildStash'
	fileOperations([fileUnZipOperation(filePath: 'sta.zip', targetLocation: '')])
	bat('sta\\test0.exe --output_format=XML --log_level=test_suite --report_level=no --result_code=no > test0.xml')
	xunit([BoostTest(
            pattern: 'test*.xml',
            stopProcessingIfError: true,
            deleteOutputFiles: true,
            failIfNotNew: true,
            skipNoTestFiles: false)])
}

def buildAndTestNix() {
    fileOperations([folderCreateOperation('bld2')])
    cmakeBuild(
            sourceDir: '.',
            buildDir: 'bld2',
            buildType: 'RelWithDebInfo',
            installation: 'MinGW64',
            steps: [[withCmake: true]])
    sh('./bld2/test0 --output_format=XML --log_level=test_suite --report_level=no --result_code=no > test0.xml')
	xunit([BoostTest(
            pattern: 'test*.xml',
            stopProcessingIfError: true,
            deleteOutputFiles: true,
            failIfNotNew: true,
            skipNoTestFiles: false)])
    archiveArtifacts 'bld2/test0'
}

node ('fedora && linux && 64bit') {
    buildWinMinGW()
}

node ('windows && 64bit') {
    testWinMinGW()
}

node ('fedora && linux && 64bit') {
    buildAndTestNix()
}
