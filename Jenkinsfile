import java.nio.file.Paths

def _mkXUnitArgs() {
    return [BoostTest(deleteOutputFiles: true, failIfNotNew: true, pattern: 'test*.xml', skipNoTestFiles: false, stopProcessingIfError: true)]
}

def _mkCMakeArgs(srcPath, bldPath, def extraCMakeArgsStr = null) {
    def args = [buildDir: bldPath.toString(), buildType: 'RelWithDebInfo', installation: 'MinGW64', sourceDir: srcPath.toString(), steps: [[withCmake: true]]]
    extraCMakeArgsStr ?: (args['cmakeArgs'] = extraCMakeArgsStr)
    return args
}

def _copyFiles(dstPath, srcPathList) {
    pathStr = ''
    srcPathList.each { pathStr += /"${it.toString()}" / }
    sh "cp --target-directory=${dstPath.toString()} ${pathStr}"
}

def buildWinMinGW() {
    def workspacePath = Paths.get(env.WORKSPACE)
    def bldPath = workspacePath.resolve('bld')
    def staPath = workspacePath.resolve('sta')

    def exePath = bldPath.resolve('test0.exe')
    def dllPathList = [
        Paths.get('/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcc_s_seh-1.dll'),
        Paths.get('/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libstdc++-6.dll'),
        Paths.get('/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libwinpthread-1.dll')]
    
    fileOperations([folderCreateOperation('bld'), folderDeleteOperation('sta'), folderCreateOperation('sta')])

    cmakeBuild(_mkCMakeArgs(workspacePath, bldPath, '-DCMAKE_TOOLCHAIN_FILE=/usr/share/mingw/toolchain-mingw64.cmake'))
    
    _copyFiles(staPath, dllPathList)
    _copyFiles(staPath, [exePath])
    fileOperations([fileZipOperation(staPath.toString())])
    
    stash includes: 'sta.zip', name: 'MinGWBuildStash'
    archiveArtifacts 'sta.zip'
}

def testWinMinGW() {
    fileOperations([folderDeleteOperation('sta')])
    
	unstash 'MinGWBuildStash'
	
	fileOperations([fileUnZipOperation(filePath: 'sta.zip', targetLocation: '')])
	bat label: '', script: 'sta\\test0.exe --output_format=XML --log_level=test_suite --report_level=no --result_code=no > test0.xml'
	xunit(_mkXUnitArgs())
}

def buildAndTestNix() {
    def workspacePath = Paths.get(env.WORKSPACE)
    def bldPath = workspacePath.resolve('bld2')
    
    fileOperations([folderCreateOperation('bld2')])
    
    cmakeBuild(_mkCMakeArgs(workspacePath, bldPath))
    
    sh script: './bld2/test0 --output_format=XML --log_level=test_suite --report_level=no --result_code=no > test0.xml'
    xunit(_mkXUnitArgs())
    
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
