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
                        //sh 'apt-get -y install speech-dispatcher libgstreamer-plugins-base1.0-dev libgstreamer1.0-0:amd64 libgstreamer1.0-dev libsdl2-dev libudev-dev wget'
                        sh 'mkdir ${WORKSPACE}/gstreamer'
                        sh 'tar jxf gstreamer-1.0-android-universal-1.14.4.tar.bz2 -C /qgroundcontrol/gstreamer/'
                        sh 'echo $PATH'
						withCredentials(bindings: [file(credentialsId: 'AndroidReleaseKey', variable: 'ANDROID_KEYSTORE')]) {
							sh 'cp $ANDROID_KEYSTORE ${WORKSPACE}/android/android_release.keystore.h'
						}

                        sh './tools/update_android_version.sh;'
						sh 'export'
						sh 'ccache -z'
						sh 'git submodule deinit -f .'
						sh 'git clean -ff -x -d .'
                        sh 'git submodule update --init --recursive --force'
                        sh 'cp ${WORKSPACE}/android/strings.xml /opt/Qt/5.11.0/android_armv7/src/android/java/res/values/strings.xml'
                        withCredentials([string(credentialsId: 'ANDROID_STOREPASS', variable: 'ANDROID_STOREPASS')]) {
                            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=installer CONFIG+=${QGC_CONFIG}'
                            sh 'cd build; make -j`nproc --all`'
                        }
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
