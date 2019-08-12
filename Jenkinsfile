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
                            image 'pelardon.aeronavics.com:8084/qgc_android'
                            args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
                        }
                    }

                    steps {
                        sh 'git fetch --tags'
                        sh 'echo $PATH'
                        sh 'echo $CCACHE_BASEDIR'
                        withCredentials(bindings: [file(credentialsId: 'AndroidReleaseKey', variable: 'ANDROID_KEYSTORE')]) {
                            sh 'cp $ANDROID_KEYSTORE ${WORKSPACE}/android/android_release.keystore.h'
                        }

                        sh './tools/update_android_version.sh;'
                        sh 'export'
                        sh 'ccache -z'
                        sh 'git submodule deinit -f .'
                        sh 'git clean -ff -x -d .'
                        sh 'git submodule update --init --recursive --force'
                        sh 'wget --quiet http://192.168.2.144:8086/nexus/repository/gstreamer-android-qgroundcontrol/gstreamer/gstreamer-1.0-android-universal-1.14.4.tar.bz2'
                        sh 'tar jxf gstreamer-1.0-android-universal-1.14.4.tar.bz2 -C ${WORKSPACE}' 
                        withCredentials([string(credentialsId: 'ANDROID_STOREPASS', variable: 'ANDROID_STOREPASS')]) {
                            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=installer CONFIG+=${QGC_CONFIG}'
                            sh 'cd build; make -j`nproc --all`'
                        }
                        sh 'ccache -s'
                    }
                    post {
                        success {
                            stash(includes: 'build/**/*.apk', name: 'android_binary')
                            archiveArtifacts artifacts: 'build/**/*.apk'

                        }
                        cleanup {
                            sh 'rm -r ${WORKSPACE}/build || true'
                            sh 'rm -r ${WORKSPACE}/gstreamer* || true'
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
                            image 'pelardon.aeronavics.com:8084/qgc_linux'
                            args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
                        }
                    }
                    steps {
                        sh 'git fetch --tags'
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
                        success {
                            stash(includes: 'build/**/*.AppImage', name: 'linux_appimage')
                            stash(includes: 'build/**/*.deb', name: 'linux_deb')
                            archiveArtifacts artifacts: 'build/**/*.AppImage'
                            archiveArtifacts artifacts: 'build/**/*.deb'
                        }
                        cleanup {
                            sh 'rm -r ${WORKSPACE}/build || true'
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
                        bat 'git fetch --tags'
                        bat "call vcvarsall.bat"
                        withCredentials(bindings: [file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
                            sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
                        }
                        bat "call winbuild.bat"
                    }
                    post {
                        success {
                            stash(includes: 'build/release/*installer*.exe', name: 'windows_binary')
                            archiveArtifacts artifacts: 'build/release/*installer*.exe'
                        }
                    }
                }
            }
        }
        stage('deploy stable') {
	        agent {
	        	node{
	        		label "linux"
	        	}
	        }
            environment {
                QGC_REGISTRY_CREDS = credentials('qgc_uploader')
            }
            when {
                branch 'prod'
            }
            steps {
                script {
                    env.QGC_CONFIG = 'release'
                    env.VERSION_NAME = getVersion()
                    env.HASH_NAME = getHash()
                    env.TAG_NAME = getTag()
                }

                sh "wget --user=${QGC_REGISTRY_CREDS_USR} --password=${QGC_REGISTRY_CREDS_PSW} http://pelardon.aeronavics.com:8086/nexus/repository/nexus_tools/remove_latest.py"
                sh "python3 remove_latest.py qgroundcontrol ${QGC_REGISTRY_CREDS_USR} ${QGC_REGISTRY_CREDS_PSW} false"
                sh "rm remove_latest.py || true"

                unstash 'linux_appimage'
                unstash 'linux_deb'
                unstash 'android_binary'
                nexusArtifactUploader(credentialsId: 'qgc_uploader', groupId: 'stable', nexusUrl: 'pelardon.aeronavics.com:8086/nexus', nexusVersion: 'nexus3', protocol: 'http', repository: 'qgroundcontrol', version: 'latest', artifacts: [
                [artifactId: 'QGroundControl', classifier: "${env.VERSION_NAME}", file: 'build/release/package/QGroundControl.AppImage', type: 'AppImage'],
                [artifactId: 'QGroundControl', classifier: "${env.VERSION_NAME}", file: 'build/release/package/QGroundControl.deb', type: 'DEB'],
                [artifactId: 'QGroundControl', classifier: "${env.VERSION_NAME}", file: 'build/release/package/QGroundControl.apk', type: 'apk']
                ])
                nexusArtifactUploader(credentialsId: 'qgc_uploader', groupId: 'stable', nexusUrl: 'pelardon.aeronavics.com:8086/nexus', nexusVersion: 'nexus3', protocol: 'http', repository: 'qgroundcontrol', version: "${env.TAG_NAME}", artifacts: [
                [artifactId: 'QGroundControl', classifier: "${env.HASH_NAME}", file: 'build/release/package/QGroundControl.AppImage', type: 'AppImage'],
                [artifactId: 'QGroundControl', classifier: "${env.HASH_NAME}", file: 'build/release/package/QGroundControl.deb', type: 'DEB'],
                [artifactId: 'QGroundControl', classifier: "${env.HASH_NAME}", file: 'build/release/package/QGroundControl.apk', type: 'apk']
                ])
            }
        }
        stage('deploy unstable') {
	        agent {
	        	node{
	        		label "linux"
	        	}
	        }
            environment {
                QGC_REGISTRY_CREDS = credentials('qgc_uploader')
            }
            when {
                branch 'work_on_pipeline'
            }
            steps {
                script {
                    env.QGC_CONFIG = 'release'
                    env.VERSION_NAME = getVersion()
                    env.HASH_NAME = getHash()
                    env.TAG_NAME = getTag()
                }

                sh "wget --user=${QGC_REGISTRY_CREDS_USR} --password=${QGC_REGISTRY_CREDS_PSW} http://pelardon.aeronavics.com:8086/nexus/repository/nexus_tools/remove_latest.py"
                sh "python3 remove_latest.py qgroundcontrol ${QGC_REGISTRY_CREDS_USR} ${QGC_REGISTRY_CREDS_PSW} true"
                sh "rm remove_latest.py || true"

                unstash 'linux_appimage'
                unstash 'linux_deb'
                unstash 'android_binary'
                nexusArtifactUploader(credentialsId: 'qgc_uploader', groupId: 'unstable', nexusUrl: 'pelardon.aeronavics.com:8086/nexus', nexusVersion: 'nexus3', protocol: 'http', repository: 'qgroundcontrol', version: "${env.VERSION_NAME}", artifacts: [
                [artifactId: 'QGroundControl', classifier: "${env.HASH_NAME}", file: 'build/release/package/QGroundControl.AppImage', type: 'AppImage'],
                [artifactId: 'QGroundControl', classifier: "${env.HASH_NAME}", file: 'build/release/package/QGroundControl.deb', type: 'DEB'],
                [artifactId: 'QGroundControl', classifier: "${env.HASH_NAME}", file: 'build/release/package/QGroundControl.apk', type: 'apk']
                ])
                nexusArtifactUploader(credentialsId: 'qgc_uploader', groupId: 'unstable', nexusUrl: 'pelardon.aeronavics.com:8086/nexus', nexusVersion: 'nexus3', protocol: 'http', repository: 'qgroundcontrol', version: 'latest', artifacts: [
                [artifactId: 'QGroundControl', classifier: "${env.VERSION_NAME}", file: 'build/release/package/QGroundControl.AppImage', type: 'AppImage'],
                [artifactId: 'QGroundControl', classifier: "${env.VERSION_NAME}", file: 'build/release/package/QGroundControl.deb', type: 'DEB'],
                [artifactId: 'QGroundControl', classifier: "${env.VERSION_NAME}", file: 'build/release/package/QGroundControl.apk', type: 'apk']
                                ])
            }
        }
    }
    environment {
        CCACHE_CPP2 = '1'
        CCACHE_DIR = '/tmp/ccache'
        QT_FATAL_WARNINGS = '1'
    }
}

def getTag()
{
    tags = sh(returnStdout: true, script: "git tag").trim()
    print tags
    return tags
}

def getVersion()
{
    tags = sh(returnStdout: true, script: "git describe --tags --abbrev=7").trim()
    print tags
    return tags
}

def getHash()
{
    hash = sh(returnStdout: true, script: "git rev-parse --short HEAD").trim()
    print hash
    return hash
}
