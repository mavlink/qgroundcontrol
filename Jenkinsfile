pipeline {
	agent none
	stages {
		stage('build') {
			parallel {

				stage('Android Release') {
					environment {
						CCACHE_BASEDIR = "${env.WORKSPACE}"
						QGC_CONFIG = 'release'
						QMAKE_VER = "5.11.0/android_armv7/bin/qmake"
						GSTREAMER_ROOT_ANDROID = "/qgroundcontrol/gstreamerr"
		                VERSION_NAME = getVersion()
					}
					agent {
						docker {
							image 'pelardon.aeronavics.com:8084/qgc_android'
							args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
						}
					}

					steps {
						sh 'git fetch --tags'
                        sh 'echo $PATH'
                        sh 'echo $CCACHE_BASEDIR'
                        sh 'echo Version ${VERSION_NAME}'
						withCredentials(bindings: [file(credentialsId: 'AndroidReleaseKey', variable: 'ANDROID_KEYSTORE')]) {
							sh 'cp $ANDROID_KEYSTORE ${WORKSPACE}/android/android_release.keystore.h'
						}

                        sh './tools/update_android_version.sh;'
						sh 'export'
						sh 'ccache -z'
						sh 'git submodule deinit -f .'
						sh 'git clean -ff -x -d .'
                        sh 'git submodule update --init --recursive --force'
                        sh 'wget --quiet http://192.168.2.144:8086/repository/gstreamer-android-qgroundcontrol/gstreamer/gstreamer-1.0-android-universal-1.14.4.tar.bz2'
                        sh 'tar jxf gstreamer-1.0-android-universal-1.14.4.tar.bz2 -C ${WORKSPACE}' 
                        withCredentials([string(credentialsId: 'ANDROID_STOREPASS', variable: 'ANDROID_STOREPASS')]) {
                            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=installer CONFIG+=${QGC_CONFIG}'
                            sh 'cd build; make -j`nproc --all`'
                        }
						sh 'ccache -s'
					}
					post {
						always {
                            archiveArtifacts artifacts: 'build/release/package/*.apk'
                            nexusArtifactUploader(
                                credentialsId: 'qgc_uploader',
                                groupId: 'qgroundcontrol',
                                nexusUrl: 'pelardon.aeronavics.com:8086',
                                nexusVersion: 'nexus3',
                                protocol: 'http',
                                repository: 'qgroundcontrol',
                                version: "${VERSION_NAME}",
                                artifacts: [
                                    [artifactId: 'QGroundControl', classifier: "${env.GIT_COMMIT}", file: 'build/release/package/QGroundControl.apk', type: 'apk']
                                ]
                                )
                        }
						cleanup {
						    sh 'rm -r ${WORKSPACE}/build'
						    sh 'rm -r ${WORKSPACE}/gstreamer*'
							sh 'git clean -ff -x -d .'
						}
					}
				}


				stage('Linux Release') {
					environment {
						CCACHE_BASEDIR = "${env.WORKSPACE}"
						QGC_CONFIG = 'release'
						QMAKE_VER = "5.11.0/gcc_64/bin/qmake"
		                VERSION_NAME = getVersion()
					}
					agent {
						docker {
							image 'pelardon.aeronavics.com:8084/qgc_linux'
							args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
						}
					}
					steps {
						sh 'git fetch --tags'
                        sh 'echo Version ${VERSION_NAME}'
						sh 'export'
						sh 'ccache -z'
						sh 'git submodule deinit -f .'
						sh 'git clean -ff -x -d .'
						sh 'git submodule update --init --recursive --force'
						withCredentials([file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
							sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
						}
						sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=installer CONFIG+=${QGC_CONFIG} -spec linux-g++-64'
						sh 'cd build; make -j`nproc --all`'
						sh 'ccache -s'
                        sh './deploy/create_linux_appimage.sh ${WORKSPACE} ${WORKSPACE}/build/release ${WORKSPACE}/build/release/package'
                        sh './deploy/create_linux_deb.sh ${WORKSPACE}/build/release/QGroundControl ${WORKSPACE}/deploy/control ${WORKSPACE}/build/release/package'
					}
					post {
						always {
							archiveArtifacts artifacts: 'build/**/*.AppImage'
							archiveArtifacts artifacts: 'build/**/*.deb'
                            nexusArtifactUploader(
                                credentialsId: 'qgc_uploader',
                                groupId: 'qgroundcontrol',
                                nexusUrl: 'pelardon.aeronavics.com:8086',
                                nexusVersion: 'nexus3',
                                protocol: 'http',
                                repository: 'qgroundcontrol',
                                version: "${env.VERSION_NAME}",
                                artifacts: [
                                    [artifactId: 'QGroundControl', classifier: "${env.GIT_COMMIT}", file: 'build/release/package/QGroundControl.AppImage', type: 'AppImage'],
                                    [artifactId: 'QGroundControl', classifier: "${env.GIT_COMMIT}", file: 'build/release/package/QGroundControl.deb', type: 'DEB'],
                                ]
                                )
						}
						cleanup {
						    sh 'rm -r ${WORKSPACE}/build'
							sh 'git clean -ff -x -d .'
						}
					}
				}



				stage('Windows Release') {
					agent {
						node {
							label 'windows'
						}

					}
					environment {
						QGC_CONFIG = 'release'
						QMAKE_VER = '5.11.0/gcc_64/bin/qmake'
		                VERSION_NAME = getVersion()
					}
					steps {
						bat 'git fetch --tags'
						bat "call vcvarsall.bat"
						withCredentials(bindings: [file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
							sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
						}
						bat "call winbuild.bat"
					}
					post {
						always {
							archiveArtifacts artifacts: 'build/release/*installer*.exe'
                            nexusArtifactUploader(
                                credentialsId: 'qgc_uploader',
                                groupId: 'qgroundcontrol',
                                nexusUrl: 'pelardon.aeronavics.com:8086',
                                nexusVersion: 'nexus3',
                                protocol: 'http',
                                repository: 'qgroundcontrol',
                                version: "${env.VERSION_NAME}",
                                artifacts: [
                                    [artifactId: 'QGroundControl', classifier: "${env.GIT_COMMIT}", file: 'build/release/QGroundControl-installer.exe', type: 'exe'],
                                ]
                                )
						}
					}
				}
			}
		}
	}
	environment {
		CCACHE_CPP2 = '1'
		CCACHE_DIR = '/tmp/ccache'
		QT_FATAL_WARNINGS = '1'
	}


}

def getVersion()
{
    tags = sh(returnStdout: true, script: "git describe --tags --abbrev=7").trim()
    print tags
    return tags
}
