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
					}
					agent {
						docker {
							image 'mavlink/qgc-build-android:2019-02-03'
							args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw -u root:root'
						}
					}

					steps {
                        sh 'wget --quiet https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/gstreamer-1.0-android-universal-1.14.4.tar.bz2'
                        sh 'apt update'
                        sh 'apt install -y bzip2'
                        sh 'tar jxf gstreamer-1.0-android-universal-1.14.4.tar.bz2 -C .'
                        sh 'wget --quiet https://s3-us-west-2.amazonaws.com/qgroundcontrol/dependencies/Qt5.11.0-android_armv7-min.tar.bz2'
                        sh 'tar jxf Qt5.11.0-android_armv7-min.tar.bz2 -C /tmp'
                        sh 'wget --quiet https://dl.google.com/android/repository/android-ndk-r16b-linux-x86_64.zip'
                        sh 'unzip android-ndk-r16b-linux-x86_64.zip > /dev/null'
                        sh 'export ANDROID_NDK_ROOT=`pwd`/android-ndk-r16b'
                        sh 'export ANDROID_SDK_ROOT=/usr/local/android-sdk'
                        sh 'export PATH=/tmp/Qt5.11-android_armv7/5.11.0/android_armv7/bin:`pwd`/android-ndk-r16b:$PATH'
                        sh 'echo $PATH'
                        sh './tools/update_android_version.sh;'
						sh 'export'
						sh 'ccache -z'
						sh 'git submodule deinit -f .'
						sh 'git clean -ff -x -d .'
						sh 'git submodule update --init --recursive --force'
						sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=installer CONFIG+=${QGC_CONFIG}'
						sh 'cd build; make -j`nproc --all`'
						sh 'ls -al build/'
						sh 'ls -al build/release/'
						sh 'ccache -s'
					}
					post {
						always {
							archiveArtifacts artifacts: 'build/release/package/*.apk'
						}
						cleanup {
							sh 'git clean -ff -x -d .'
						}
					}
				}


				stage('Linux Release') {
					environment {
						CCACHE_BASEDIR = "${env.WORKSPACE}"
						QGC_CONFIG = 'release'
						QMAKE_VER = "5.11.0/gcc_64/bin/qmake"
					}
					agent {
						docker {
							image 'mavlink/qgc-build-linux:2019-02-03'
							args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
						}
					}
					steps {
						sh 'export'
						sh 'ccache -z'
						sh 'git submodule deinit -f .'
						sh 'git clean -ff -x -d .'
						sh 'git submodule update --init --recursive --force'
						withCredentials([file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
							sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
						}
						sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=installer CONFIG+=${QGC_CONFIG}'
						sh 'cd build; make -j`nproc --all`'
						sh 'ccache -s'
                        sh './deploy/create_linux_appimage.sh . build/release build/release/package;'
					}
					post {
						always {
							archiveArtifacts artifacts: 'build/**/*.AppImage'
						}
						cleanup {
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
					}
					steps {
						echo "Test"
						bat "call vcvarsall.bat"
						withCredentials(bindings: [file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
							sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
						}
						bat "call winbuild.bat"
					}
					post {
						always {
							archiveArtifacts artifacts: 'build/release/*installer*.exe'
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
	options {
		buildDiscarder(logRotator(numToKeepStr: '10', artifactDaysToKeepStr: '30'))
			timeout(time: 60, unit: 'MINUTES')
	}
}
