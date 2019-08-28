pipeline {
  agent none
  stages {

    stage('build') {
      parallel {

        /*stage('Android Release') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'release'
            QMAKE_VER = "5.11.0/android_armv7/bin/qmake"
          }
          agent {
            docker {
              image 'mavlink/qgc-build-android:2019-02-03'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`nproc --all`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('Linux Debug') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'debug'
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
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`nproc --all`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('Linux Debug (cmake)') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Debug'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "gcc_64"
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
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make linux'
            //sh 'make linux check' // TODO: needs Xvfb or similar
            sh 'ccache -s'
            sh 'make distclean'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }
        */
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
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`nproc --all`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }
        /*
        stage('Linux Release (cmake)') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Release'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "gcc_64"
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
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make linux'
            sh 'ccache -s'
            sh 'make distclean'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('OSX Debug') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'debug'
            QMAKE_VER = "5.11.0/clang_64/bin/qmake"
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
            sh 'cd build; make -j`sysctl -n hw.ncpu`'
            sh 'ccache -s'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('OSX Debug (cmake)') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Debug'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "clang_64"
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make mac'
            sh 'ccache -s'
            sh 'make distclean'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('OSX Release') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'installer'
            QMAKE_VER = "5.11.0/clang_64/bin/qmake"
          }
          stages {
            stage('Clean Checkout') {
              steps {
                sh 'export'
                sh 'ccache -z'
                sh 'git submodule deinit -f .'
                sh 'git clean -ff -x -d .'
                sh 'git submodule update --init --recursive --force'
              }
            }

            stage('Add Airmap API key') {
              steps {
                withCredentials([file(credentialsId: 'QGC_Airmap_api_key', variable: 'AIRMAP_API_HEADER')]) {
                  sh 'cp $AIRMAP_API_HEADER ${WORKSPACE}/src/Airmap/Airmap_api_key.h'
                }
              }
              when {
                anyOf {
                  branch 'master';
                  branch 'Stable_*'
                }
              }
            }

            stage('Build OSX Release') {
              steps {
                sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG} CONFIG+=WarningsAsErrorsOn'
                sh 'cd build; make -j`sysctl -n hw.ncpu`'
                */
                //archiveArtifacts(artifacts: 'build/**/*.dmg', fingerprint: true)
                /*
                sh 'ccache -s'
              }
            }
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }

        stage('OSX Release (cmake)') {
          agent {
            node {
              label 'mac'
            }
          }
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            CMAKE_BUILD_TYPE = 'Release'
            QT_VERSION = "5.11.0"
            QT_MKSPEC = "clang_64"
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'make distclean'
            sh 'make submodulesclean'
            sh 'make mac'
            sh 'ccache -s'
            sh 'make distclean'
          }
          post {
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }
        */


        stage('Custom CentOS Release') {
          environment {
            CCACHE_BASEDIR = "${env.WORKSPACE}"
            QGC_CONFIG = 'release'
            QMAKE_VER = "5.12.4/gcc_64/bin/qmake"

            QGC_CUSTOM_APP_NAME = "Custom QGC"
            QGC_CUSTOM_GENERIC_NAME = "Custom Ground Station"
            QGC_CUSTOM_BINARY_NAME = "CustomQGC"
            QGC_CUSTOM_LINUX_START_SH = "${env.WORKSPACE}/custom/deploy/qgroundcontrol-start.sh"
            QGC_CUSTOM_APP_ICON = "${env.WORKSPACE}/resources/icons/qgroundcontrol.png"
            QGC_CUSTOM_APP_ICON_NAME = "qgroundcontrol"
          }
          agent {
            docker {
              alwaysPull true
              label 'docker'
              image 'stefandunca/qgc:centos-5.12.4'
              args '-v ${CCACHE_DIR}:${CCACHE_DIR}:rw --privileged --cap-add SYS_ADMIN --device /dev/fuse'
            }
          }
          steps {
            sh 'export'
            sh 'ccache -z'
            sh 'git submodule sync && git submodule deinit -f .'
            sh 'git clean -ff -x -d .'
            sh 'git submodule update --init --recursive --force'
            sh 'ln -s custom-example custom'
            sh 'mkdir build; cd build; ${QT_PATH}/${QMAKE_VER} -r ${WORKSPACE}/qgroundcontrol.pro CONFIG+=${QGC_CONFIG}'
            sh 'cd build; make -j`nproc --all`'
            // Create AppImage
            sh 'deploy/create_linux_appimage.sh ${WORKSPACE}/ ${WORKSPACE}/build/release/'
            sh 'chmod +x *.AppImage'

            // Cache build files
            sh 'ccache -s'
          }
          post {
            always {
                archiveArtifacts artifacts: 'build/release/**/*', onlyIfSuccessful: true
                archiveArtifacts artifacts: '*.AppImage'
            }
            cleanup {
              sh 'git clean -ff -x -d .'
            }
          }
        }


        stage('Windows Release') {
          environment {
            QGC_CONFIG = 'release installer separate_debug_info force_debug_info qtquickcompiler'
          }
          agent {
            node {
              label 'windows'
            }
          }
          steps {
            bat 'git submodule sync && git submodule deinit -f .'
            bat 'git clean -ff -x -d .'
            bat 'git submodule update --init --recursive --force'
            bat '.\\tools\\build\\build_windows.bat release build'
            bat 'copy /Y .\\build\\release\\*-installer.exe .\\'
          }
          post {
            always {
                archiveArtifacts artifacts: 'build/release/**/*', onlyIfSuccessful: true
                archiveArtifacts artifacts: '*-installer.exe'
            }
            cleanup {
              //bat 'git clean -ff -x -e build-dev -d .'
              bat 'git clean -ff -x -d .'
            }
          }
        }
      } // parallel
    } // stage('build')
  } // stages

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
